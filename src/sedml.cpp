#include <string>
#include <iostream>
#include <vector>
#include <map>
#include "sedml.hpp"

LIBSEDML_CPP_NAMESPACE_USE

typedef std::pair<std::string, std::string> StringPair;
typedef std::map<std::string, std::string> StringMap;
typedef std::map<std::string, StringPair> StringPairMap;

static void printStringMap(StringMap& map)
{
    for (auto i = map.begin(); i != map.end(); ++i)
    {
        std::cout << "Namespace: " << i->first.c_str() << " ==> " << i->second.c_str() << std::endl;
    }
}

static std::string nonEssentialString()
{
    static int counter = 1000;
    std::stringstream ss;
    ss << counter++;
    std::string number(ss.str());
    std::string value("bob_");
    value += number;
    return value;
}

class MyReport
{
public:
    MyReport(const SedReport* sedReport)
    {
        sed = sedReport;
        id = sed->getId();
        for (unsigned int i = 0; i < sed->getNumDataSets(); ++i)
        {
            const SedDataSet* dataSet = sed->getDataSet(i);
            // these strings are not required in the SED-ML document, so need to handle them being absent
            std::string id = dataSet->isSetId() ? dataSet->getId() : nonEssentialString();
            std::string label = dataSet->isSetLabel() ? dataSet->getLabel() : nonEssentialString();
            dataSets[id] = StringPair(dataSet->getDataReference(), label);
        }
    }

    int resolveTasks(SedDocument* doc)
    {
        int numberOfErrors = 0;
        for (auto i = dataSets.begin(); i != dataSets.end(); ++i)
        {
            std::cout << "DataSet " << i->first.c_str() << ":" << std::endl;
            std::cout << "\tdata reference: " << i->second.first.c_str() << std::endl;
            std::cout << "\tlabel: " << i->second.second.c_str() << std::endl;
            SedDataGenerator* dg = doc->getDataGenerator(i->second.first);
            // FIXME: we're assuming, for now, that there is one variable and that variable is all we care about
            if (dg->getNumVariables() != 1)
            {
                std::cerr << "We can only handle one variable, sorry!" << std::endl;
                ++numberOfErrors;
            }
            else
            {
                StringMap namespaces;
                SedVariable* v = dg->getVariable(0);
                std::string id = v->isSetId() ? v->getId().c_str() : nonEssentialString();
                std::string target = v->getTarget();
                std::string taskRef = v->getTaskReference();
                // grab all the namespaces to use when resolving the target xpaths
                SedBase* current = v;
                while (current)
                {
                    const XMLNamespaces* nss = current->getNamespaces();
                    if (nss)
                    {
                        for (unsigned int i = 0; i < nss->getNumNamespaces(); ++i)
                        {
                            std::string prefix = nss->getPrefix(i);
                            if (namespaces.count(prefix) == 0)
                            {
                                // only want to add a namespace if its not already in there
                                namespaces[prefix] = nss->getURI(i);
                            }
                        }
                    }
                    current = current->getParentSedObject();
                }
                std::cout << "\t\tVariable " << id.c_str() << ": target=" << target.c_str()
                     << "; task=" << taskRef.c_str() << std::endl;
                printStringMap(namespaces);
            }
        }
        return numberOfErrors;
    }

    const SedReport* sed;
    std::string id;
    StringPairMap dataSets;
};

class MyReportList : public std::vector<MyReport>
{
public:
    int resolveTasks(SedDocument* doc)
    {
        int numberOfErrors = 0;
        for (auto i = begin(); i != end(); ++i)
        {
            numberOfErrors += i->resolveTasks(doc);
        }
        return numberOfErrors;
    }
};

Sedml::Sedml() : mSed(NULL), mReports(NULL)
{
}

Sedml::~Sedml()
{
    if (mSed) delete mSed;
    if (mReports) delete mReports;
}

int Sedml::parseFromString(const std::string &xmlDocument)
{
    mSed = readSedMLFromString(xmlDocument.c_str());
    int numErrors = mSed->getErrorLog()->getNumFailsWithSeverity(LIBSEDML_SEV_ERROR);
    if (numErrors > 0)
    {
        std::cerr << "Error loading SED-ML document: " << xmlDocument.c_str() << std::endl;
        std::cerr << mSed->getErrorLog()->toString();
        delete mSed;
        mSed = NULL;
        return numErrors;
    }
    return 0;
}

int Sedml::buildExecutionManifest()
{
    int numberOfErrors = 0;
    if (mReports) delete mReports;
    for (unsigned int i = 0; i < mSed->getNumOutputs(); ++i)
    {
      SedOutput* current = mSed->getOutput(i);
      switch(current->getTypeCode())
      {
        case SEDML_OUTPUT_REPORT:
        {
          if (!mReports) mReports = new MyReportList();
          mReports->push_back(MyReport(static_cast<SedReport*>(current)));
          break;
        }
        case SEDML_OUTPUT_PLOT2D:
        {
          SedPlot2D* p = static_cast<SedPlot2D*>(current);
          std::cout << "\t[unsupported] Plot2d id=" << current->getId() << " numCurves=" << p->getNumCurves() << std::endl;
          ++numberOfErrors;
          break;
        }
        case SEDML_OUTPUT_PLOT3D:
        {
          SedPlot3D* p = static_cast<SedPlot3D*>(current);
          std::cout << "\t[unsupported] Plot3d id=" << current->getId() << " numSurfaces=" << p->getNumSurfaces() << std::endl;
          ++numberOfErrors;
          break;
        }
        default:
          std::cout << "\tEncountered unknown output " << current->getId() << std::endl;
          ++numberOfErrors;
          break;
      }
    }
    if (numberOfErrors) return numberOfErrors;
    if (mReports)
    {
        numberOfErrors = mReports->resolveTasks(mSed);
    }

    return numberOfErrors;
}

int Sedml::checkBob()
{
    int numberOfErrors = 0;
    // first check we can handle the simulations required of us
    std::cout << "The document has " << mSed->getNumSimulations() << " simulation(s)." << std::endl;
    for (unsigned int i = 0; i < mSed->getNumSimulations(); ++i)
    {
      SedSimulation* current = mSed->getSimulation(i);
      switch(current->getTypeCode())
      {
         case SEDML_SIMULATION_UNIFORMTIMECOURSE:
         {
            SedUniformTimeCourse* tc = static_cast<SedUniformTimeCourse*>(current);
            const SedAlgorithm* alg = tc->getAlgorithm();
            std::string kisaoId = alg->getKisaoID();
            if (kisaoId == "KISAO:0000019")
            {
                // CVODE integration, we can handle that
                std::cout << "UniformTimecourse CVODE integration:\n";
                std::cout << "\tid = " << tc->getId() << "\n\tinterval = " << tc->getOutputStartTime() << "-"
                          << tc->getOutputEndTime() << "\n\tnumPoints = " << tc->getNumberOfPoints() << std::endl;
            }
            else if ((kisaoId == "KISAO:0000000") && alg->isSetAnnotation())
            {
                // FIXME: this all needs to be namespace aware?!

                // need to check the annotation to see what to do.
                XMLNode* annotation = alg->getAnnotation();
                if (annotation->getNumChildren() < 1)
                {
                    std::cerr << "Can't handle custom algorithm:\n";
                    std::cerr << annotation->toXMLString() << std::endl;
                    return -31;
                }
                XMLNode& csimGetSimulator = annotation->getChild(0);
                if (!csimGetSimulator.hasAttr("method"))
                {
                    std::cerr << "Missing method attribute on:\n"
                              << csimGetSimulator.toXMLString() << std::endl;
                    return -32;
                }
                std::string getSimulatorMethod = csimGetSimulator.getAttrValue("method");
                std::cout << "UniformTimecourse GET Simulator simulation:\n"
                          << "\tid = " << tc->getId() << "\n\tinterval = " << tc->getOutputStartTime() << "-"
                          << tc->getOutputEndTime() << "\n\tnumPoints = " << tc->getNumberOfPoints()
                          << "\n\tGET method = " << getSimulatorMethod.c_str() << std::endl;
            }
            break;
         }
         default:
            std::cout << "\tUncountered unknown simulation. " << current->getId() << std::endl;
            ++numberOfErrors;
            break;
      }
    }
    std::cout << std::endl;

    // check for models we can handle and keep track of them
    std::cout << "The document has " << mSed->getNumModels() << " model(s)." << std::endl;
    for (unsigned int i = 0; i < mSed->getNumModels(); ++i)
    {
      SedModel* current = mSed->getModel(i);
      std::string modelLanguage = current->getLanguage();
      std::cout << "\tModel id=" << current->getId() << " language=" << current->getLanguage() << " source=" << current->getSource() << " numChanges=" << current->getNumChanges() << std::endl;
      if (modelLanguage.find("cellml"))
      {
          std::cout << "\tWe can handle CellML models of all versions." << std::endl;
      }
      else
      {
          std::cout << "\tWe are unable to handle models of type: " << modelLanguage.c_str() << std::endl;
          ++numberOfErrors;
      }
      if (current->getNumChanges() > 0)
      {
          std::cout << "\tModel changes are currently unsupported" << std::endl;
          ++numberOfErrors;
      }
    }
    std::cout << std::endl;

    std::cout << "The document has " << mSed->getNumTasks() << " task(s)." << std::endl;
    for (unsigned int i = 0; i < mSed->getNumTasks(); ++i)
    {
        SedTask* current = mSed->getTask(i);
        SedRepeatedTask* repeat = dynamic_cast<SedRepeatedTask*>(current);
        if (repeat != NULL)
        {
            std::cout << "\tRepeated tasks not yet supported, sorry!" << std::endl;
            ++numberOfErrors;
        }
        else
            std::cout << "\tTask id=" << current->getId() << " model=" << current->getModelReference() << " sim=" << current->getSimulationReference() << std::endl;
    }
    std::cout << std::endl;

    std::cout << "The document has " << mSed->getNumDataGenerators() << " datagenerators(s)." << std::endl;
    for (unsigned int i = 0; i < mSed->getNumDataGenerators(); ++i)
    {
      SedDataGenerator* current = mSed->getDataGenerator(i);
      std::cout << "\tDG id=" << current->getId() << " math=" << SBML_formulaToString(current->getMath()) << std::endl;
      for (unsigned int j = 0; j < current->getNumVariables(); ++j)
      {
          SedVariable* v = current->getVariable(j);
          std::string id = v->isSetId() ? v->getId().c_str() : "(no id)";
          std::string target = v->isSetTarget() ? v->getTarget() : "(no target)";
          std::string taskRef = v->isSetTaskReference() ? v->getTaskReference() : "(no task reference?)";
          std::cout << "\t\tVariable " << id.c_str() << ": target=" << target.c_str()
               << "; task=" << taskRef.c_str() << std::endl;
      }
    }

    std::cout << std::endl;
    std::cout << "The document has " << mSed->getNumOutputs() << " output(s)." << std::endl;
    for (unsigned int i = 0; i < mSed->getNumOutputs(); ++i)
    {
      SedOutput* current = mSed->getOutput(i);
      switch(current->getTypeCode())
      {
        case SEDML_OUTPUT_REPORT:
        {
          SedReport* r = static_cast<SedReport*>(current);
          std::cout << "\tReport id=" << current->getId() << " numDataSets=" << r->getNumDataSets() << std::endl;
          break;
        }
        case SEDML_OUTPUT_PLOT2D:
        {
          SedPlot2D* p = static_cast<SedPlot2D*>(current);
          std::cout << "\t[unsupported] Plot2d id=" << current->getId() << " numCurves=" << p->getNumCurves() << std::endl;
          ++numberOfErrors;
          break;
        }
        case SEDML_OUTPUT_PLOT3D:
        {
          SedPlot3D* p = static_cast<SedPlot3D*>(current);
          std::cout << "\t[unsupported] Plot3d id=" << current->getId() << " numSurfaces=" << p->getNumSurfaces() << std::endl;
          ++numberOfErrors;
          break;
        }
        default:
          std::cout << "\tEncountered unknown output " << current->getId() << std::endl;
          break;
      }
    }

    if (numberOfErrors)
    {
        std::cerr << "There are things we can't handle, exiting" << std::endl;
        return -100 * numberOfErrors;
    }
    return 0;
}

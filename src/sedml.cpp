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

class MyData
{
public:
    std::string id;
    std::string label;
    std::string dataReference;
    std::string target;
    std::string taskReference;
    StringMap namespaces; // used to resolve the target XPath in the source document
};

class MyTask
{
public:
    std::string id;
    std::string name;
    std::string modelReference;
    std::string simulationReference;
};

class MyModel
{
public:
    std::string id;
    std::string name;
    std::string source;
};

class MySimulation
{
public:
    MySimulation()
    {
        mCsim = false;
        mGet = false;
    }
    void setSimulationTypeCsim()
    {
        mCsim = true;
        mGet = false;
    }
    bool isCsim() const
    {
        return mCsim;
    }

    void setSimulationTypeGet(const std::string& method)
    {
        mCsim = false;
        mGet = true;
        mMethod = method;
    }
    bool isGet() const
    {
        return mGet;
    }

    std::string id;
    double initialTime;
    double startTime;
    double endTime;
    int numberOfPoints;

private:
    // FIXME: should use enum? ok for now since there are just two options
    bool mCsim;
    bool mGet;
    std::string mMethod; // GET: open-circuit or closed-circuit?
};

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
            MyData d;
            // these strings are not required in the SED-ML document, so need to handle them being absent
            d.id = dataSet->isSetId() ? dataSet->getId() : nonEssentialString();
            d.label = dataSet->getLabel(); // empty string if no label set
            d.dataReference = dataSet->getDataReference();
            dataSets[d.id] = d;
        }
    }

    int resolveDataSets(SedDocument* doc)
    {
        int numberOfErrors = 0;
        for (auto i = dataSets.begin(); i != dataSets.end(); ++i)
        {
            std::cout << "DataSet " << i->first.c_str() << ":" << std::endl;
            MyData& d = i->second;
            std::cout << "\tdata reference: " << d.dataReference.c_str() << std::endl;
            std::cout << "\tlabel: " << (d.label == "" ? "no label" : d.label.c_str()) << std::endl;
            SedDataGenerator* dg = doc->getDataGenerator(d.dataReference);
            // FIXME: we're assuming, for now, that there is one variable and that variable is all we care about
            if (dg->getNumVariables() != 1)
            {
                std::cerr << "We can only handle one variable, sorry!" << std::endl;
                ++numberOfErrors;
            }
            else
            {
                SedVariable* v = dg->getVariable(0);
                std::string id = v->isSetId() ? v->getId().c_str() : nonEssentialString();
                d.target = v->getTarget();
                d.taskReference = v->getTaskReference();
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
                            if (d.namespaces.count(prefix) == 0)
                            {
                                // only want to add a namespace if its not already in there
                                d.namespaces[prefix] = nss->getURI(i);
                            }
                        }
                    }
                    current = current->getParentSedObject();
                }
                std::cout << "\t\tVariable " << d.id.c_str() << ": target=" << d.target.c_str()
                     << "; task=" << d.taskReference.c_str() << std::endl;
                printStringMap(d.namespaces);
            }
        }
        return numberOfErrors;
    }

    int resolveTasks(SedDocument* doc)
    {
        int numberOfErrors = 0;
        for (auto i = dataSets.begin(); i != dataSets.end(); ++i)
        {
            std::cout << "DataSet " << i->first.c_str() << ":" << std::endl;
            MyData& d = i->second;
            const SedTask* task = doc->getTask(d.taskReference);
            MyTask t;
            t.id = task->getId();
            if (tasks.count(t.id) == 0)
            {
                std::cout << "Adding task: " << t.id.c_str() << " to the execution manifest" << std::endl;
                t.name = task->getName();
                t.modelReference = task->getModelReference();
                t.simulationReference = task->getSimulationReference();
                tasks[t.id] = t;
            }
            else std::cout << "Task (" << t.id.c_str() << ") already in the execution manifest" << std::endl;
        }
        return numberOfErrors;
    }

    int resolveModels(SedDocument* doc)
    {
        int numberOfErrors = 0;
        for (auto i = tasks.begin(); i != tasks.end(); ++i)
        {
            MyTask& task = i->second;
            SedModel* model = doc->getModel(task.modelReference);
            MyModel m;
            m.id = model->getId();
            if (models.count(m.id) == 0)
            {
                std::cout << "Adding model: " << m.id.c_str() << " to the execution manifest" << std::endl;
                std::string language = model->getLanguage();
                // we can only handle CellML models.
                if (language.find("cellml"))
                {
                    m.name = model->getName(); // empty string is ok
                    // FIXME: assuming here that the source is always a cellml model, but could be a reference to another model? or would that be a modelReference?
                    m.source = model->getSource();
                    models[m.id] = m;
                }
                else
                {
                    std::cerr << "Sorry, we can only handle CellML models." << std::endl;
                    ++numberOfErrors;
                }
            }
            else std::cout << "Model (" << m.id.c_str() << ") is already in the execution manifest" << std::endl;
        }
        return numberOfErrors;
    }

    int resolveSimulations(SedDocument* doc)
    {
        int numberOfErrors = 0;
        for (auto i = tasks.begin(); i != tasks.end(); ++i)
        {
            MyTask& task = i->second;
            SedSimulation* simulation = doc->getSimulation(task.simulationReference);
            if (simulation->getTypeCode() == SEDML_SIMULATION_UNIFORMTIMECOURSE)
            {
                MySimulation s;
                s.id = simulation->getId();
                if (simulations.count(s.id) == 0)
                {
                    std::cout << "Adding simulation: " << s.id.c_str() << " to the execution manifest" << std::endl;
                    SedUniformTimeCourse* tc = static_cast<SedUniformTimeCourse*>(simulation);
                    const SedAlgorithm* alg = tc->getAlgorithm();
                    std::string kisaoId = alg->getKisaoID();
                    if (kisaoId == "KISAO:0000019")
                    {
                        // CVODE integration, we can handle that with CSim
                        s.setSimulationTypeCsim();
                        s.initialTime = tc->getInitialTime();
                        s.startTime = tc->getOutputStartTime();
                        s.endTime = tc->getOutputEndTime();
                        s.numberOfPoints = tc->getNumberOfPoints();
                    }
                    else if ((kisaoId == "KISAO:0000000") && alg->isSetAnnotation())
                    {
                        // FIXME: this all needs to be namespace aware?!
                        // need to check the annotation to see what to do.
                        XMLNode* annotation = alg->getAnnotation();
                        if (annotation->getNumChildren() != 1)
                        {
                            std::cerr << "Can't handle custom algorithm:\n";
                            std::cerr << annotation->toXMLString() << std::endl;
                            ++numberOfErrors;
                        }
                        else
                        {
                            XMLNode& csimGetSimulator = annotation->getChild(0);
                            if (!csimGetSimulator.hasAttr("method"))
                            {
                                std::cerr << "Missing method attribute on:\n"
                                          << csimGetSimulator.toXMLString() << std::endl;
                                ++numberOfErrors;
                            }
                            else
                            {
                                s.setSimulationTypeGet(csimGetSimulator.getAttrValue("method"));
                                s.initialTime = tc->getInitialTime();
                                s.startTime = tc->getOutputStartTime();
                                s.endTime = tc->getOutputEndTime();
                                s.numberOfPoints = tc->getNumberOfPoints();
                            }
                        }
                    }
                }
                else std::cout << "Simulation (" << s.id.c_str() << ") already in execution manifest." << std::endl;
            }
            else
            {
                std::cerr << "Unable to handle simulations that are not uniform time courses" << std::endl;
                ++numberOfErrors;
            }
        }
        return numberOfErrors;
    }

    const SedReport* sed;
    std::string id;
    std::map<std::string, MyData> dataSets;
    std::map<std::string, MyTask> tasks;
    std::map<std::string, MyModel> models;
    std::map<std::string, MySimulation> simulations;
};

class MyReportList : public std::vector<MyReport>
{
public:
    int resolveTasks(SedDocument* doc)
    {
        int numberOfErrors = 0;
        for (auto i = begin(); i != end(); ++i)
        {
            numberOfErrors += i->resolveDataSets(doc);
            numberOfErrors += i->resolveTasks(doc);
            numberOfErrors += i->resolveModels(doc);
            numberOfErrors += i->resolveSimulations(doc);
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

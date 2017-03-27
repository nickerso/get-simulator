#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>

#include <sbml/SBMLTypes.h>

#include <csim/model.h>

#include "utils.hpp"
#include "sedml.hpp"
#include "dataset.hpp"
#include "plotting.hpp"
#include "simulationenginecsim.hpp"
#include "simulationengineget.hpp"
#include "utilityclasses.hpp"
#include "setvaluechange.hpp"

LIBSEDML_CPP_NAMESPACE_USE

static void printStringMap(StringMap& map)
{
    for (auto i = map.begin(); i != map.end(); ++i)
    {
        std::cout << "Key: " << i->first.c_str() << " ==> " << i->second.c_str() << std::endl;
    }
}

#if 0
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
#endif

static std::map<std::string, std::string> getAllNamespaces(SedBase* current)
{
    std::map<std::string, std::string> nsList;
    // grab all the namespaces, to use when resolving target xpaths
    while (current)
    {
        const XMLNamespaces* nss = current->getNamespaces();
        if (nss)
        {
            for (int i = 0; i < nss->getNumNamespaces(); ++i)
            {
                std::string prefix = nss->getPrefix(i);
                if (nsList.count(prefix) == 0)
                {
                    // only want to add a namespace if its not already in there
                    nsList[prefix] = nss->getURI(i);
                }
            }
        }
        current = current->getParentSedObject();
    }
    return nsList;
}

class MyRange
{
public:
    std::string id;
    std::vector<double> rangeData;
};

class MyTask
{
public:
    MyTask()
    {
        isRepeatedTask = false;
    }

    /**
     * @brief Execute the given task, will ensure the source data is available in the data collection, but won't evaluate the data generator math.
     * @param models
     * @param simulations
     * @param dataSets
     * @param masterTaskId The ID of the current top-level task being executed (the parent repeated task)
     * @param changesToApply
     * @return
     */
    int execute(std::map<std::string, MyModel>& models,
                std::map<std::string, MySimulation>& simulations,
                DataCollection* dataCollection, const std::string& masterTaskId,
                bool resetModel, std::vector<MySetValueChange>& changesToApply)
    {
        int numberOfErrors = 0;
        if (isRepeatedTask) numberOfErrors = executeRepeated(models, simulations,
                                                             dataCollection, masterTaskId,
                                                             changesToApply);
        else numberOfErrors = executeSingle(models, simulations, dataCollection,
                                            masterTaskId, resetModel, changesToApply);
        return numberOfErrors;
    }

    int executeRepeated(std::map<std::string, MyModel>& models,
                        std::map<std::string, MySimulation>& simulations,
                        DataCollection* dataCollection,
                        const std::string& masterTaskId,
                        std::vector<MySetValueChange>& changesToApply)
    {
        int numberOfErrors = 0;
        // FIXME: a quick and dirty initial implementation of repeated tasks
        const MyRange& r = ranges[masterRangeId];
        int rangeIndex = 0;
        // take a copy of the set value changes that we need to make
        std::vector<MySetValueChange> localSetValueChanges(setValueChanges);
        for (double rangeValue: r.rangeData)
        {
            std::cout << "Execute repeat with master range (" << masterRangeId << ") value: " << rangeValue << std::endl;
            // update the set value changes to have the current values
            for (MySetValueChange& svc: localSetValueChanges)
            {
                const MyRange& rr = ranges[svc.rangeId];
                svc.currentRangeValue = rr.rangeData[rangeIndex];
                std::cout << "Setting range: " << svc.rangeId << "; to value: " << svc.currentRangeValue << std::endl;
            }
            // and add them to the existing set of changes
            localSetValueChanges.insert(localSetValueChanges.end(), changesToApply.begin(), changesToApply.end());
            // and then execute the sub tasks
            for (MyTask& st: subTasks) numberOfErrors +=
                    st.execute(models, simulations, dataCollection, masterTaskId,
                               resetModel, localSetValueChanges);
            ++rangeIndex;
        }
        return numberOfErrors;
    }

    int executeSingle(std::map<std::string, MyModel>& models,
                      std::map<std::string, MySimulation>& simulations,
                      DataCollection* dataCollection, const std::string& masterTaskId,
                      bool resetModel, std::vector<MySetValueChange>& changesToApply)
    {
        int numberOfErrors = 0;
        std::cout << "\n\nExecuting: " << id.c_str() << std::endl;
        std::cout << "\tsimulation = " << simulationReference.c_str() << std::endl;
        std::cout << "\tmodel = " << modelReference.c_str() << std::endl;
        std::cout << "\tnumber of changes to apply = " << changesToApply.size() << std::endl;
        const MyModel& model = models[modelReference];
        const MySimulation& simulation = simulations[simulationReference];
        std::vector<Variable*> outputVariables;
        if (simulation.isCsim())
        {
            std::cout << "\trunning simulation task using CSim..." << std::endl;
            // we need to keep track of exisiting CSim objects so that when executing repeated tasks
            // the already created and initialised CSim can be used.
            SimulationEngineCsim* csim;
            if (csimList.count(id))
            {
                csim = csimList[id];
                if (resetModel) csim->resetSimulator(true);
            }
            else
            {
                csim = new SimulationEngineCsim();
                csim->loadModel(model.modelXML);
                // flag output variables for capture
                for (auto& di: *dataCollection)
                {
                    Data& d = di.second;
                    // we only want variables relevant to this task (the master task)
                    for (auto& variables: d.variables)
                    {
                        Variable& v = variables.second;
                        if (v.taskReference == masterTaskId)
                        {
                            std::cout << "\tAdding variable: "
                                      << variables.first
                                      << "; to the outputs for this task."
                                      << std::endl;
                            // this also sets the variable's index in the output vector
                            csim->addOutputVariable(v);
                        }
                    }
                }
                // we also need to access the variables for setting changes as inputs
                for (MySetValueChange& change: changesToApply)
                {
                    if (change.modelReference == modelReference)
                    {
                        csim->addInputVariable(change);
                    }
                }
                // initialise the engine
                if (csim->instantiateSimulation() != 0)
                {
                    std::cerr << "Error instantiating the simulation?" << std::endl;
                    return -1;
                }
                csimList[id] = csim;
            }
            // apply relevant changes
            for (const MySetValueChange& change: changesToApply)
            {
                std::cout << "Change to apply: " << std::endl;
                if (change.modelReference == modelReference)
                {
                    std::cout << "Applying set value change, value: " << change.currentRangeValue << std::endl;
                    csim->applySetValueChange(change);
                }
            }
            // and grab outputs
            for (auto& di: *dataCollection)
            {
                Data& d = di.second;
                for (auto& variables: d.variables)
                {
                    Variable& v = variables.second;
                    if (v.taskReference == masterTaskId)
                    {
                        outputVariables.push_back(&v);
                    }
                }
            }
            // initialise the simulation
            csim->initialiseSimulation(simulation, simulation.initialTime,
                                       simulation.startTime);
            std::cout << "got to here 2345" << std::endl;

            std::vector<double> stepResults = csim->getOutputValues();
            // save results
            for (Variable* ov: outputVariables)
            {
                ov->data.push_back(stepResults[ov->outputIndex]);
            }
            std::cout << "Got to here 1234" << std::endl;
            double dt = (simulation.endTime - simulation.startTime) / simulation.numberOfPoints;
            double time = simulation.startTime;
            for (int i = 1; i <= simulation.numberOfPoints; ++i, time += dt)
            {
                if (csim->simulateModelOneStep(dt) == 0)
                {
                    stepResults = csim->getOutputValues();
                    // save results
                    for (Variable* ov: outputVariables)
                    {
                        ov->data.push_back(stepResults[ov->outputIndex]);
                    }
                }
                else
                {
                    std::cerr << "Error in simulation at time = " << time << std::endl;
                    break;
                }
            }
        }
        else if (simulation.isGet())
        {
            std::cout << "\trunning simulation task using GET..." << std::endl;
            SimulationEngineGet get;
            get.loadModel(model.source);
            int columnIndex = 1;
            for (auto di = dataCollection->begin(); di != dataCollection->end();
                 ++di, ++columnIndex)
            {
                const Data& d = di->second;
                // we only want variables relevant to this task
                for (auto& variables: d.variables)
                {
                    const Variable& v = variables.second;
                    if (v.taskReference == this->id)
                    {
                        //outputVariables.push_back(variables.first);
                        std::cout << "\tAdding variable: " << variables.first
                                  << "; to the outputs for this task."
                                  << std::endl;
                        get.addOutputVariable(v, columnIndex);
                    }
                }
            }
            get.initialiseSimulation();
            get.getOutputValues();
        }
        else
        {
            std::cerr << "Unknown type of simulation?" << std::endl;
            ++numberOfErrors;
        }
        return numberOfErrors;
    }

    std::string id;
    std::string name;
    std::string modelReference;
    std::string simulationReference;
    bool isRepeatedTask;
    bool resetModel;
    std::string masterRangeId;
    std::map<std::string, MyRange> ranges;
    std::vector<MyTask> subTasks;
    std::vector<MySetValueChange> setValueChanges;
    std::map<std::string, SimulationEngineCsim*> csimList;

    ~MyTask()
    {
        for (auto i = csimList.begin(); i != csimList.end(); ++i) delete i->second;
    }
};

/**
 * @brief The ExecutionManifest class for tracking the tasks that must be executed to satisfy
 * a given data collection.
 */
class ExecutionManifest
{
public:
    ExecutionManifest(const SedDocument* doc, DataCollection* dc,
                      const std::string& baseUri) : sed(doc), dataCollection(dc),
        baseUri(baseUri)
    {

    }

    int resolveTasks()
    {
        int numberOfErrors = 0;
        for (auto i = dataCollection->begin(); i != dataCollection->end(); ++i)
        {
            std::cout << "Data " << i->first.c_str() << ":" << std::endl;
            const Data& d = i->second;
            for (auto& variables: d.variables)
            {
                const Variable& v = variables.second;
                const SedTask* task = sed->getTask(v.taskReference);
                MyTask t;
                t.id = task->getId();
                if (tasks.count(t.id) == 0)
                {
                    std::cout << "Adding task: " << t.id << " to the execution manifest" << std::endl;
                    resolveTask(t, task);
                    tasks[t.id] = t;
                }
                else std::cout << "Task (" << t.id << ") already in the execution manifest" << std::endl;
            }
        }
        return numberOfErrors;
    }

    int resolveTask(MyTask& t, const SedTask* task)
    {
        int numberOfErrors = 0;
        const SedRepeatedTask* repeat = dynamic_cast<const SedRepeatedTask*>(task);
        if (repeat != NULL)
        {
            t.isRepeatedTask = true;
            numberOfErrors += resolveRepeatedTask(t, repeat);
        }
        else
        {
            t.name = task->getName();
            t.modelReference = task->getModelReference();
            t.simulationReference = task->getSimulationReference();
        }
        return numberOfErrors;
    }

    int resolveRepeatedTask(MyTask& repeat, const SedRepeatedTask* sedRepeat)
    {
        int numberOfErrors = 0;
        repeat.resetModel = sedRepeat->getResetModel();
        repeat.masterRangeId = sedRepeat->getRangeId();

        for (unsigned int i = 0; i < sedRepeat->getNumRanges(); ++i)
        {
          const SedRange* current = sedRepeat->getRange(i);
          const SedFunctionalRange* functional = dynamic_cast<const SedFunctionalRange*>(current);
          const SedVectorRange* vrange = dynamic_cast<const SedVectorRange*>(current);
          const SedUniformRange* urange = dynamic_cast<const SedUniformRange*>(current);
          if (functional != NULL)
          {
              numberOfErrors++;
              std::cerr << "Functional ranges not yet supported";
          }
          else if (vrange != NULL)
          {
              MyRange range;
              range.id = vrange->getId();
              range.rangeData = std::vector<double>(vrange->getValues());
              repeat.ranges[range.id] = range;
          }
          else if (urange != NULL)
          {
              MyRange range;
              range.id = urange->getId();
              std::string t = urange->getType();
              if (t == "log")
              {
                  numberOfErrors++;
                  std::cerr << "Uniform ranges of type 'log' are not yet supported" << std::endl;
              }
              else
              {
                  double start = urange->getStart();
                  double end = urange->getEnd();
                  double n = urange->getNumberOfPoints();
                  double step = (end - start) / n;
                  for (int i = 0; i <= n; ++i)
                  {
                      double value = start + i * step;
                      range.rangeData.push_back(value);
                  }
                  repeat.ranges[range.id] = range;
              }
          }
        }

        for (unsigned int i = 0; i < sedRepeat->getNumTaskChanges(); ++i)
        {
            const SedSetValue* current = sedRepeat->getTaskChange(i);
            // FIXME: for now assume that if a range attribute is present we simply want to assign the current range value
            // to the given target. Will ignore the math.
            MySetValueChange vc;
            vc.rangeId = current->getRange();
            vc.modelReference = current->getModelReference();
            vc.targetXpath = current->getTarget();
            // grab all the namespaces to use when resolving the target xpaths
            const SedBase* c = current;
            while (c)
            {
                const XMLNamespaces* nss = c->getNamespaces();
                if (nss)
                {
                    for (int i = 0; i < nss->getNumNamespaces(); ++i)
                    {
                        std::string prefix = nss->getPrefix(i);
                        if (vc.namespaces.count(prefix) == 0)
                        {
                            // only want to add a namespace if its not already in there
                            vc.namespaces[prefix] = nss->getURI(i);
                        }
                    }
                }
                c = c->getParentSedObject();
            }
            repeat.setValueChanges.push_back(vc);
        }
        for (unsigned int i = 0; i < sedRepeat->getNumSubTasks(); ++i)
        {
          const SedSubTask* current = sedRepeat->getSubTask(i);
          const SedTask* task = sedRepeat->getSedDocument()->getTask(current->getTask());
          MyTask st;
          st.id = task->getId();
          std::cout << "Adding sub task: " << st.id.c_str() << " to the execution manifest" << std::endl;
          resolveTask(st, task);
          if (current->isSetOrder())
          {
              std::cerr << "\n\n****** Sorry, specifying the order is not yet supported and we ignore it\n" << std::endl;
          }
          repeat.subTasks.push_back(st);
        }
        return numberOfErrors;
    }

    int resolveModel(const SedModel* model)
    {
        int numberOfErrors = 0;
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
                // check if the model refers to another model that we want to reuse
                const SedModel* sourceModel = sed->getModel(model->getSource());
                if (sourceModel)
                {
                    // is it already resolved?
                    const std::string& sourceModelId = sourceModel->getId();
                    if (models.count(sourceModelId) == 0)
                        numberOfErrors += resolveModel(sourceModel);
                    if (numberOfErrors != 0) return numberOfErrors;
                    m.source = models[sourceModelId].source;
                    m.modelXML = models[sourceModelId].modelXML;
                }
                else
                {
                    m.source = buildAbsoluteUri(model->getSource(), baseUri);
                    m.modelXML = csim::Model::serialiseUrlToString(m.source, m.source, true);
                    if (m.modelXML.length() == 0) return 1;
                }
                std::cout << "\tModel source: " << model->getSource() << std::endl;
                std::cout << "\tModel URL: " << m.source << std::endl;
                // apply any changes
                for (unsigned int i = 0; i < model->getNumChanges(); ++i)
                {
                    // IMPLEMENT THIS BIT ANDRE
                    std::cerr << "IMPLEMENT THIS BIT ANDRE" << std::endl;
                }
                models[m.id] = m;
            }
            else
            {
                std::cerr << "Sorry, we can only handle CellML models." << std::endl;
                ++numberOfErrors;
            }
        }
        else std::cout << "Model (" << m.id.c_str() << ") is already in the execution manifest" << std::endl;
        return numberOfErrors;
    }

    int resolveRepeatedTaskModels(const MyTask& task)
    {
        int numberOfErrors = 0;
        for (const MyTask& st: task.subTasks)
        {
            if (st.isRepeatedTask) numberOfErrors += resolveRepeatedTaskModels(st);
            else
            {
                // normal task
                const SedModel* model =sed->getModel(st.modelReference);
                numberOfErrors += resolveModel(model);
            }
        }
        return numberOfErrors;
    }

    int resolveModels()
    {
        int numberOfErrors = 0;
        for (auto i = tasks.begin(); i != tasks.end(); ++i)
        {
            MyTask& task = i->second;
            if (task.isRepeatedTask) numberOfErrors += resolveRepeatedTaskModels(task);
            else
            {
                const SedModel* model = sed->getModel(task.modelReference);
                numberOfErrors += resolveModel(model);
            }
        }
        return numberOfErrors;
    }

    int resolveSimulation(const SedSimulation* simulation)
    {
        int numberOfErrors = 0;
        if (simulation->getTypeCode() == SEDML_SIMULATION_UNIFORMTIMECOURSE)
        {
            MySimulation s;
            s.id = simulation->getId();
            if (simulations.count(s.id) == 0)
            {
                std::cout << "Adding simulation: " << s.id.c_str() << " to the execution manifest" << std::endl;
                const SedUniformTimeCourse* tc = static_cast<const SedUniformTimeCourse*>(simulation);
                const SedAlgorithm* alg = tc->getAlgorithm();
                std::string kisaoId = alg->getKisaoID();
                if ((kisaoId == "KISAO:0000019") || (kisaoId == "KISAO:0000030"))
                {
                    // CVODE integration, we can handle that with CSim
                    // FIXME: with CSim-v2 we can now do more, but this will do to get
                    // things working.
                    s.setSimulationTypeCsim(kisaoId);
                    s.initialTime = tc->getInitialTime();
                    s.startTime = tc->getOutputStartTime();
                    s.endTime = tc->getOutputEndTime();
                    s.numberOfPoints = tc->getNumberOfPoints();
                    // check for any parameters
                    unsigned int np = alg->getNumAlgorithmParameters();
                    for (unsigned int i = 0; i < np; ++i)
                    {
                        const SedAlgorithmParameter* ap = alg->getAlgorithmParameter(i);
                        const std::string& apki = ap->getKisaoID();
                        if (apki == "KISAO:0000211")
                        {
                            // absolute tolerance
                            s.absoluteTolerance = std::stod(ap->getValue());
                            std::cout << "resolveSimulation: setting absolute tolerance = "
                                      << s.absoluteTolerance << std::endl;
                        }
                        else if (apki == "KISAO:0000209")
                        {
                            // relative tolerance
                            s.relativeTolerance = std::stod(ap->getValue());
                            std::cout << "resolveSimulation: setting relative tolerance = "
                                      << s.relativeTolerance << std::endl;
                        }
                        else if (apki == "KISAO:0000467")
                        {
                            // maximum step size
                            s.maximumStepSize = std::stod(ap->getValue());
                            std::cout << "resolveSimulation: setting max step size = "
                                      << s.maximumStepSize << std::endl;
                        }
                        else if (apki == "KISAO:0000415")
                        {
                            // maximum number of steps
                            s.maximumNumberOfSteps = std::stod(ap->getValue());
                            std::cout << "resolveSimulation: setting max number of steps = "
                                      << s.maximumNumberOfSteps << std::endl;
                        }
                    }
                    simulations[s.id] = s;
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
                            simulations[s.id] = s;
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
        return numberOfErrors;
    }

    int resolveRepeatedTaskSimulations(const MyTask& task)
    {
        int numberOfErrors = 0;
        for (const MyTask& st: task.subTasks)
        {
            if (st.isRepeatedTask) numberOfErrors += resolveRepeatedTaskSimulations(st);
            else
            {
                // normal task
                const SedSimulation* simulation = sed->getSimulation(st.simulationReference);
                numberOfErrors += resolveSimulation(simulation);
            }
        }
        return numberOfErrors;
    }

    int resolveSimulations()
    {
        int numberOfErrors = 0;
        for (auto i = tasks.begin(); i != tasks.end(); ++i)
        {
            MyTask& task = i->second;
            if (task.isRepeatedTask)
            {
                numberOfErrors += resolveRepeatedTaskSimulations(task);
            }
            else
            {
                // normal task
                const SedSimulation* simulation =
                        sed->getSimulation(task.simulationReference);
                numberOfErrors += resolveSimulation(simulation);
            }
        }
        return numberOfErrors;
    }

    int execute()
    {
        int numberOfErrors = 0;
        for (auto i = tasks.begin(); i != tasks.end(); ++i)
        {
            MyTask& t = i->second;
            std::vector<MySetValueChange> changes;
            numberOfErrors += t.execute(models, simulations, dataCollection, t.id,
                                        false, changes);
        }
        return numberOfErrors;
    }

    int serialise(std::ostream& os)
    {
        int numberOfErrors = 0;
#ifdef FIX_SERIALISATION
        int first = 0, minDataSize = -1;
        for (const auto& ds: dataSets)
        {
            MyData d = ds.second;
            if (first) os << ",";
            else first = 1;
            os << d.label;
            int dataLength = d.data.size();
            if ((minDataSize < 0) || (dataLength < minDataSize)) minDataSize = dataLength;
        }
        os << std::endl;
        // FIXME: for now assume that all datasets have the same number of results?
        for (int i = 0; i < minDataSize; ++i)
        {
            first = 0;
            for (const auto& ds: dataSets)
            {
                MyData d = ds.second;
                if (first) os << ",";
                else first = 1;
                os << d.data[i];
            }
            os << std::endl;
        }
#endif
        return numberOfErrors;
    }

    const SedDocument* sed;
    DataCollection* dataCollection;
    const std::string& baseUri;
    std::map<std::string, MyTask> tasks;
    std::map<std::string, MyModel> models;
    std::map<std::string, MySimulation> simulations;
};

#if 0
/**
 * @brief The TaskList class, a wrapper to manage the tasks that must be
 * executed.
 */
class TaskList : public std::vector<MyReport>
{
public:
    /**
     * @brief Resolve the tasks required to compute all the data required
     * by the given data set.
     * @param doc The SED-ML document handle.
     * @param dataSet The defined data generators that must be satisfied.
     * @param baseUri The base URI used to resolve relative references.
     * @return Non-zero on error.
     */
    int resolveTasks(SedDocument* doc, DataSet& dataSet,
                     const std::string& baseUri)
    {
        int numberOfErrors = 0;
        for (auto i = begin(); i != end(); ++i)
        {
            numberOfErrors += i->resolveDataSets(doc);
            numberOfErrors += i->resolveTasks(doc);
            numberOfErrors += i->resolveModels(doc, baseUri);
            numberOfErrors += i->resolveSimulations(doc);
        }
        return numberOfErrors;
    }

    int execute()
    {
        int numberOfErrors = 0;
        for (auto i = begin(); i != end(); ++i)
        {
            numberOfErrors += i->execute();
        }
        return numberOfErrors;
    }

    int serialise(std::ostream& os)
    {
        int numberOfErrors = 0;
        for (auto i = begin(); i != end(); ++i)
        {
            numberOfErrors += i->serialise(os);
        }
        return numberOfErrors;
    }
};
#endif

Sedml::Sedml() : mSed(NULL), mDataCollection(NULL), mExecutionManifest(NULL),
    mExecutionPerformed(false), mDataComputed(false)
{
}

Sedml::~Sedml()
{
    if (mSed) delete mSed;
    if (mDataCollection) delete mDataCollection;
    if (mExecutionManifest) delete mExecutionManifest;
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
    if (mSed->getNumErrors() > 0)
    {
      std::cout << "Warnings: " << mSed->getErrorLog()->toString() << std::endl;
    }

    return 0;
}

Data Sedml::createData(const std::string &dgId)
{
    Data data;
    SedDataGenerator* dg = mSed->getDataGenerator(dgId);
    if (dg)
    {
        data.id = dgId;
        // no point continuing if no math has been set
        if (dg->isSetMath() == false) return data;
        data.math = dg->getMath();
        for (int vc=0; vc < dg->getNumVariables(); ++vc)
        {
            SedVariable* v = dg->getVariable(vc);
            Variable var;
            var.target = v->getTarget();
            var.taskReference = v->getTaskReference();
            var.namespaces = getAllNamespaces(v);
            std::cout << "\t\tVariable " << v->getId()
                      << ": target=" << var.target
                      << "; task=" << var.taskReference << std::endl;
            printStringMap(var.namespaces);
            data.variables[v->getId()] = var;
        }
        for (int pc=0; pc < dg->getNumParameters(); ++pc)
        {
            SedParameter* p = dg->getParameter(pc);
            std::cout << "\t\tParameter " << p->getId()
                      << ": value=" << p->getValue() << std::endl;
            data.parameters[p->getId()] = p->getValue();
        }
    }
    else
    {
        std::cerr << "ERROR: Unable to find data generator with ID: "
                  << dgId << std::endl;
    }
    return data;
}

int Sedml::buildExecutionManifest(const std::string& baseUri)
{
    int numberOfErrors = 0;
    if (mDataCollection) delete mDataCollection;
    if (mExecutionManifest) delete mExecutionManifest;
    // first collect all data generators referenced in all outputs, if needed
    if (mSed->getNumOutputs() == 0) return -2; // no point continuing...
    mDataCollection = new DataCollection;
    DataCollection& dc = *mDataCollection;
    for (unsigned int i = 0; i < mSed->getNumOutputs(); ++i)
    {
        SedOutput* current = mSed->getOutput(i);
        switch(current->getTypeCode())
        {
        case SEDML_OUTPUT_REPORT:
        {
            SedReport* r = static_cast<SedReport*>(current);
            for (unsigned int j = 0; j < r->getNumDataSets(); ++j)
            {
                const SedDataSet* dataSet = r->getDataSet(j);
                std::cout << "Looking for the data generator: " << dataSet->getDataReference()
                          << std::endl;
                if (dc.count(dataSet->getDataReference()) == 0)
                {
                    // assume valid SED-ML
                    dc[dataSet->getDataReference()] =
                            createData(dataSet->getDataReference());
                }
            }
            break;
        }
        case SEDML_OUTPUT_PLOT2D:
        {
            SedPlot2D* p = static_cast<SedPlot2D*>(current);
            for (unsigned int j = 0; j < p->getNumCurves(); ++j)
            {
                const SedCurve* curve = p->getCurve(j);
                if (dc.count(curve->getXDataReference()) == 0)
                    dc[curve->getXDataReference()] =
                            createData(curve->getXDataReference());
                if (dc.count(curve->getYDataReference()) == 0)
                    dc[curve->getYDataReference()] =
                            createData(curve->getYDataReference());
            }
            break;
        }
        case SEDML_OUTPUT_PLOT3D:
        {
            SedPlot3D* p = static_cast<SedPlot3D*>(current);
            for (unsigned int j = 0; j < p->getNumSurfaces(); ++j)
            {
                const SedSurface* surface = p->getSurface(j);
                if (dc.count(surface->getXDataReference()) == 0)
                    dc[surface->getXDataReference()] =
                            createData(surface->getXDataReference());
                if (dc.count(surface->getYDataReference()) == 0)
                    dc[surface->getYDataReference()] =
                            createData(surface->getYDataReference());
                if (dc.count(surface->getZDataReference()) == 0)
                    dc[surface->getZDataReference()] =
                            createData(surface->getZDataReference());
            }
            break;
        }
        default:
            std::cout << "\tEncountered unknown output " << current->getId() << std::endl;
            ++numberOfErrors;
            break;
        }
    }
    if (numberOfErrors) return numberOfErrors;
    if (mDataCollection->empty()) return -1;

    // we have some data generators so can set up the tasks to be executed
    mExecutionManifest = new ExecutionManifest(mSed, mDataCollection, baseUri);
    if (!mExecutionManifest) ++numberOfErrors;
    else
    {
        numberOfErrors += mExecutionManifest->resolveTasks();
        numberOfErrors += mExecutionManifest->resolveModels();
        numberOfErrors += mExecutionManifest->resolveSimulations();
    }

    return numberOfErrors;
}

int Sedml::execute()
{
    int numberOfErrors = 0;
    if (mExecutionManifest) numberOfErrors = mExecutionManifest->execute();
    mExecutionPerformed = true;
    return numberOfErrors;
}

int Sedml::computeData()
{
    if (!mExecutionPerformed)
    {
        std::cerr << "Can't compute data before executing the simulation experiment."
                  << std::endl;
        return -2;
    }
    if (mDataCollection->empty())
    {
        // nothing to do
        std::cout << "No data generators need computing?" << std::endl;
        return -1;
    }
    for (auto i = mDataCollection->begin(); i != mDataCollection->end(); ++i)
    {
        std::cout << "Computing Data " << i->first.c_str() << ":" << std::endl;
        Data& d = i->second;
        d.computeData();
    }
    mDataComputed = true;
    return 0;
}

int Sedml::serialiseOutputs(const std::string &baseOutputName)
{
    if (!mDataComputed)
    {
        std::cerr << "Data must be computed before outputs can be serialised."
                  << std::endl;
        return -1;
    }
    int numberOfErrors = 0;
    for (unsigned int i = 0; i < mSed->getNumOutputs(); ++i)
    {
        SedOutput* current = mSed->getOutput(i);
        switch(current->getTypeCode())
        {
        case SEDML_OUTPUT_REPORT:
        {
            SedReport* r = static_cast<SedReport*>(current);
            // collect the data to be output
            std::vector<std::string> header;
            std::vector<DataCollection::iterator> line;
            int maxDataLength = 0;
            for (unsigned int j = 0; j < r->getNumDataSets(); ++j)
            {
                const SedDataSet* dataSet = r->getDataSet(j);
                line.push_back(mDataCollection->find(dataSet->getDataReference()));
                std::string label = dataSet->getLabel();
                if (label.empty()) label = line.back()->second.id;
                header.push_back(label);
                std::cout << "Got data: " << line.back()->second.id << "; for a report." << std::endl;
                if (line.back()->second.computedData.size() > maxDataLength)
                    maxDataLength = line.back()->second.computedData.size();
                std::cout << "maxDataLength = " << maxDataLength << std::endl;
            }
            // set output target
            std::streambuf* buf;
            std::ofstream of;
            if (baseOutputName.empty())
            {
                buf = std::cout.rdbuf();
            }
            else
            {
                std::string filename = baseOutputName;
                filename += r->getId();
                filename += ".csv";
                of.open(filename);
                buf = of.rdbuf();
            }
            std::ostream out(buf);
            // header line
            for (unsigned int k=0; k<header.size(); ++k) out << header[k] << ", ";
            out << std::endl;
            for (unsigned int j=0; j<maxDataLength; ++j)
            {
                for (unsigned int k=0; k < line.size(); ++k)
                {
                    double data;
                    if (line[k]->second.computedData.size() > j)
                        data = line[k]->second.computedData[j];
                    else data = line[k]->second.computedData.back();
                    out << data << ", ";
                }
                out << std::endl;
            }
            break;
        }
        case SEDML_OUTPUT_PLOT2D:
        {
            SedPlot2D* p = static_cast<SedPlot2D*>(current);
            // collect the data to be output
            CurveData data;
            for (unsigned int j = 0; j < p->getNumCurves(); ++j)
            {
                const SedCurve* curve = p->getCurve(j);
                const std::string curveId = curve->getId();
                data[curveId] = XYpair(
                            mDataCollection->find(curve->getXDataReference()),
                            mDataCollection->find(curve->getYDataReference()));
                std::cout << "Got data: " << data[curveId].first->second.id << " vs " << data[curveId].second->second.id << "; for a 2D plot." << std::endl;
            }
            plot2d(data, baseOutputName);
            break;
        }
        case SEDML_OUTPUT_PLOT3D:
        {
#if 0
            SedPlot3D* p = static_cast<SedPlot3D*>(current);
            for (unsigned int j = 0; j < p->getNumSurfaces(); ++j)
            {
                const SedSurface* surface = p->getSurface(j);
                if (dc.count(surface->getXDataReference()) == 0)
                    dc[surface->getXDataReference()] =
                            createData(surface->getXDataReference());
                if (dc.count(surface->getYDataReference()) == 0)
                    dc[surface->getYDataReference()] =
                            createData(surface->getYDataReference());
                if (dc.count(surface->getZDataReference()) == 0)
                    dc[surface->getZDataReference()] =
                            createData(surface->getZDataReference());
            }
#endif
            break;
        }
        default:
            std::cout << "\tEncountered unknown output " << current->getId() << std::endl;
            ++numberOfErrors;
            break;
        }
    }
    return numberOfErrors;
}

#if 0
int Sedml::serialiseReports(std::ostream& os)
{
    int numberOfErrors = 0;
    if (mExecutionPerformed)
    {
        if (mReports) numberOfErrors += mReports->serialise(os);
        std::cout << "This is where the data would go...." << std::endl;
    }
    else
    {
        std::cerr << "You need to execute the simulation tasks before serialising the reports?" << std::endl;
        numberOfErrors++;
    }
    return numberOfErrors;
}
#endif

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

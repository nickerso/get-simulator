#ifndef UTILITYCLASSES_HPP
#define UTILITYCLASSES_HPP

typedef std::pair<std::string, std::string> StringPair;
typedef std::map<std::string, std::string> StringMap;
typedef std::map<std::string, StringPair> StringPairMap;

class MyModel
{
public:
    std::string id;
    std::string name;
    // this is the URL source of the actual CellML model being used.
    std::string source;
    // this is the actual model XML, which originates from the source URL
    // and is updated to reflect any changes applied to the model
    std::string modelXML;
};

class MySimulation
{
public:
    MySimulation()
    {
        mCsim = false;
        mGet = false;
        // set some reasonable defaults
        absoluteTolerance = 1.0e-6;
        relativeTolerance = 1.0e-8;
        maximumStepSize = 1.0e-2;
        maximumNumberOfSteps = 500; // CVODE default?
    }
    void setSimulationTypeCsim(const std::string& alg = "")
    {
        mCsim = true;
        mGet = false;
        mMethod = alg;
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

    std::string mMethod; // GET: open-circuit or closed-circuit? or CSim algorithm ID (KiSAO)

    double absoluteTolerance;
    double relativeTolerance;
    double maximumStepSize;
    long maximumNumberOfSteps;

private:
    // FIXME: should use enum? ok for now since there are just two options
    bool mCsim;
    bool mGet;
};


#endif // UTILITYCLASSES_HPP

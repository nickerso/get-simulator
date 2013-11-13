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

    std::string mMethod; // GET: open-circuit or closed-circuit?

private:
    // FIXME: should use enum? ok for now since there are just two options
    bool mCsim;
    bool mGet;
};


#endif // UTILITYCLASSES_HPP

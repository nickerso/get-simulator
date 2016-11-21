#ifndef DATASET_HPP
#define DATASET_HPP

#include <string>
#include <map>
#include <vector>

class MyVariable
{
public:
    std::string target;
    std::string taskReference;
    std::map<std::string, std::string> namespaces; // used to resolve the target XPath in the source document
    std::vector<double> data;
    int outputIndex; // used by the simulation engine to determine where data comes from
};

class VariableList : public std::map<std::string, MyVariable>
{
};

class ParameterList : public std::map<std::string, double>
{

};

// represents the data required for a single data generator
class MyData
{
public:
    std::string id;
    std::string label;
    std::string dataReference; // the data generator id
    VariableList variables;
    ParameterList parameters;
};

class DataSet : public std::map<std::string, MyData>
{
};

#endif // DATASET_HPP

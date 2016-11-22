#ifndef DATASET_HPP
#define DATASET_HPP

#include <string>
#include <map>
#include <vector>

class ASTNode;

class Variable
{
public:
    std::string target;
    std::string taskReference;
    std::map<std::string, std::string> namespaces; // used to resolve the target XPath in the source document
    std::vector<double> data;
    int outputIndex; // used by the simulation engine to determine where data comes from
};

class VariableList : public std::map<std::string, Variable>
{
};

class ParameterList : public std::map<std::string, double>
{

};

// represents the data required for a single data generator
class Data
{
public:
    std::string id;
    VariableList variables;
    ParameterList parameters;
    ASTNode* math;
};

class DataSet : public std::map<std::string, Data>
{
};

#endif // DATASET_HPP

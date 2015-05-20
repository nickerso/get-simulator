#ifndef DATASET_HPP
#define DATASET_HPP

#include <string>
#include <map>
#include <vector>

class MyData
{
public:
    std::string id;
    std::string label;
    std::string dataReference;
    std::string target;
    std::string taskReference;
    std::map<std::string, std::string> namespaces; // used to resolve the target XPath in the source document
    std::vector<double> data;
    int outputIndex;
};

class DataSet : public std::map<std::string, MyData>
{
};

#endif // DATASET_HPP

#ifndef SETVALUECHANGE_HPP
#define SETVALUECHANGE_HPP

#include <map>

class MySetValueChange
{
public:
    int inputIndex;
    std::string rangeId;
    std::string targetXpath;
    std::string modelReference;
    double currentRangeValue;
    std::map<std::string, std::string> namespaces; // used to resolve the target XPath in the source document
};

#endif // SETVALUECHANGE_HPP

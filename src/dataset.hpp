#ifndef DATASET_HPP
#define DATASET_HPP

#include <string>
#include <map>
#include <vector>

// from libsbml via libSEDML
namespace libsbml
{
    class ASTNode;
}

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

/**
 * @brief The Data class, providing the data required for a single SED-ML data generator.
 *
 * Here we collect all specified variables and parameters needed to evaluate the given math
 * for this data generator.
 */
class Data
{
public:
    std::string id;
    VariableList variables;
    ParameterList parameters;
    const libsbml::ASTNode* math;
    std::vector<double> computedData;

    /**
     * @brief Compute the data defined by this data generator.
     *
     * This method will first clear the @c computedData and then populate it with the
     * results of evaluating the defined math using the data from the variables and
     * parmeters.
     *
     * @return zero on success.
     */
    int computeData();
};

/**
 * @brief A collection of Data objects, indexed by their id.
 *
 * Data generators can be used multiple times in a simulation experiment. We use a data
 * collection to ensure each uniqueness of the collected Data objects, using the SED-ML
 * requirement that data generator IDs must be unique.
 */
class DataCollection : public std::map<std::string, Data>
{
};

#endif // DATASET_HPP

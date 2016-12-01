#include <iostream>
#include <string>
#include <map>
#include <cmath>

#include <sedml/SedTypes.h>

#include "dataset.hpp"

static double EvaluateSingle(const libsbml::ASTNode* node, int index,
                             const VariableList& variables,
                             const ParameterList& parameters)
{
    if (node->isReal()) return node->getReal();
    if (node->isInteger()) return node->getInteger();
    double result;
    if (node->isOperator())
    {
        switch (node->getType())
        {
        case libsbml::AST_PLUS:
            if (node->getNumChildren() == 0)
                return 0.0;
            if (node->getNumChildren() == 1)
                return EvaluateSingle(node->getLeftChild(), index, variables,
                                      parameters);
            result = EvaluateSingle(node->getChild(0), index, variables, parameters);
            for (int j = 1; j < node->getNumChildren(); ++j)
                result += EvaluateSingle(node->getChild(j), index, variables, parameters);
            return result;
        case libsbml::AST_MINUS:
            if (node->getNumChildren() == 1)
                return -EvaluateSingle(node->getLeftChild(), index,
                                       variables, parameters);
            return
                    EvaluateSingle(node->getLeftChild(), index, variables,
                                   parameters) -
                    EvaluateSingle(node->getRightChild(), index, variables,
                                   parameters);
        case libsbml::AST_DIVIDE:
            return
                    EvaluateSingle(node->getLeftChild(), index, variables,
                                   parameters) /
                    EvaluateSingle(node->getRightChild(), index, variables,
                                   parameters);
        case libsbml::AST_TIMES:
            if (node->getNumChildren() == 0)
                return 1.0;
            result = EvaluateSingle(node->getChild(0), index, variables,
                                           parameters);
            for (int j = 1; j < node->getNumChildren(); ++j)
            result *= EvaluateSingle(node->getChild(j), index, variables,
                                     parameters);
            return result;
        case libsbml::AST_POWER:
            return
                    std::pow(
                        EvaluateSingle(node->getLeftChild(), index, variables,
                                       parameters),
                        EvaluateSingle(node->getRightChild(), index,
                                       variables, parameters));
        default:
            break;
        }
    }
    else if (node->isName())
    {
        if (variables.count(node->getName()))
        {
            const Variable& variable = variables.at(std::string(node->getName()));
            return (variable.data)[index];
        }
        if (parameters.count(node->getName()))
        {
            return parameters.at(node->getName());
        }
        std::cerr << "requesting '" << node->getName() << "', which is neither a variable "
                  << "or a parameter in the data generator?" << std::endl;
    }

    // we are still here ... look for math we can handle
    switch (node->getType())
    {
    case libsbml::AST_FUNCTION_ABS:
        return std::abs(EvaluateSingle(node->getLeftChild(), index, variables, parameters));
#if 0
    case libsbml.AST_FUNCTION_ARCCOS:
      return
          Math.Acos(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                   ListOfParameters));
    case libsbml.AST_FUNCTION_ARCCOSH:
      return
          MathKGI.Acosh(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                       ListOfParameters));
    case libsbml.AST_FUNCTION_ARCCOT:
      return
          MathKGI.Acot(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                      ListOfParameters));
    case libsbml.AST_FUNCTION_ARCCOTH:
      return
          MathKGI.Acoth(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                       ListOfParameters));
    case libsbml.AST_FUNCTION_ARCCSC:
      return
          MathKGI.Acsc(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                      ListOfParameters));
    case libsbml.AST_FUNCTION_ARCCSCH:
      return
          MathKGI.Acsch(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                       ListOfParameters));
    case libsbml.AST_FUNCTION_ARCSEC:
      return
          MathKGI.Asec(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                      ListOfParameters));
    case libsbml.AST_FUNCTION_ARCSECH:
      return
          MathKGI.Asech(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                       ListOfParameters));
    case libsbml.AST_FUNCTION_ARCSIN:
      return
          Math.Asin(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                   ListOfParameters));
    case libsbml.AST_FUNCTION_ARCSINH:
      return
          MathKGI.Asinh(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                       ListOfParameters));
    case libsbml.AST_FUNCTION_ARCTAN:
      return
          Math.Atan(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                   ListOfParameters));
    case libsbml.AST_FUNCTION_ARCTANH:
      return
          MathKGI.Atanh(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                       ListOfParameters));
    case libsbml.AST_FUNCTION_CEILING:
      return
          Math.Ceiling(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                      ListOfParameters));
    case libsbml.AST_FUNCTION_COS:
      return
          Math.Cos(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                  ListOfParameters));
    case libsbml.AST_FUNCTION_COSH:
      return
          Math.Cosh(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                   ListOfParameters));
    case libsbml.AST_FUNCTION_COT:
      return
          MathKGI.Cot(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                     ListOfParameters));
    case libsbml.AST_FUNCTION_COTH:
      return
          MathKGI.Coth(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                      ListOfParameters));
    case libsbml.AST_FUNCTION_CSC:
      return
          MathKGI.Csc(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                     ListOfParameters));
    case libsbml.AST_FUNCTION_CSCH:
      return
          MathKGI.Csch(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                      ListOfParameters));
    case libsbml.AST_FUNCTION_DELAY:
      return
          MathKGI.Delay(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                       ListOfParameters),
                        EvaluateSingle(node->getRightChild(), index, sVariableIds, oVariableData,
                                       ListOfParameters));
    case libsbml.AST_FUNCTION_EXP:
      return
          Math.Exp(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                  ListOfParameters));
    case libsbml.AST_FUNCTION_FACTORIAL:
      return
          MathKGI.Factorial(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                           ListOfParameters));
    case libsbml.AST_FUNCTION_FLOOR:
      return
          Math.Floor(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                    ListOfParameters));
    case libsbml.AST_FUNCTION_LN:
      return
          Math.Log(
              EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData, ListOfParameters),
              Math.E);
    case libsbml.AST_FUNCTION_LOG:
      return
          Math.Log10(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                    ListOfParameters));
    case libsbml.AST_FUNCTION_PIECEWISE:
      {
        var numChildren = (int)node->getNumChildren();
        var temps = new double[numChildren];
        for (var i = 0; i < numChildren; i++)
        {
          temps[i] = EvaluateSingle(node->getChild(i), index, sVariableIds, oVariableData,
                                    ListOfParameters);
        }
        return
            MathKGI.Piecewise(temps);
      }
    case libsbml.AST_FUNCTION_POWER:
      return Math.Pow(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                     ListOfParameters),
                      EvaluateSingle(node->getRightChild(), index, sVariableIds, oVariableData,
                                     ListOfParameters));
    case libsbml.AST_FUNCTION_ROOT:
      return MathKGI.Root(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                         ListOfParameters),
                          EvaluateSingle(node->getRightChild(), index, sVariableIds, oVariableData,
                                         ListOfParameters));
    case libsbml.AST_FUNCTION_SEC:
      return MathKGI.Sec(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                        ListOfParameters));
    case libsbml.AST_FUNCTION_SECH:
      return MathKGI.Sech(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                         ListOfParameters));
    case libsbml.AST_FUNCTION_SIN:
      return Math.Sin(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                     ListOfParameters));
    case libsbml.AST_FUNCTION_SINH:
      return Math.Sinh(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                      ListOfParameters));
    case libsbml.AST_FUNCTION_TAN:
      return Math.Tan(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                     ListOfParameters));
    case libsbml.AST_FUNCTION_TANH:
      return Math.Tanh(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                      ListOfParameters));
    case libsbml.AST_LOGICAL_AND:
      {
        var numChildren = (int)node->getNumChildren();
        var temps = new double[numChildren];
        for (var i = 0; i < numChildren; i++)
        {
          temps[i] = EvaluateSingle(node->getChild(i), index, sVariableIds, oVariableData,
                                    ListOfParameters);
        }
        return
            MathKGI.And(temps);
      }
    case libsbml.AST_LOGICAL_NOT:
      {
        return
            MathKGI.Not(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                       ListOfParameters));
      }
    case libsbml.AST_LOGICAL_OR:
      {
        var numChildren = (int)node->getNumChildren();
        var temps = new double[numChildren];
        for (var i = 0; i < numChildren; i++)
        {
          temps[i] = EvaluateSingle(node->getChild(i), index, sVariableIds, oVariableData,
                                    ListOfParameters);
        }
        return
            MathKGI.Or(temps);
      }
    case libsbml.AST_LOGICAL_XOR:
      {
        var numChildren = (int)node->getNumChildren();
        var temps = new double[numChildren];
        for (var i = 0; i < numChildren; i++)
        {
          temps[i] = EvaluateSingle(node->getChild(i), index, sVariableIds, oVariableData,
                                    ListOfParameters);
        }
        return
            MathKGI.Xor(temps);
      }
    case libsbml.AST_RELATIONAL_EQ:
      {
        return
            MathKGI.Eq(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                      ListOfParameters),
                       EvaluateSingle(node->getRightChild(), index, sVariableIds, oVariableData,
                                      ListOfParameters));
      }
    case libsbml.AST_RELATIONAL_GEQ:
      return
          MathKGI.Geq(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                     ListOfParameters),
                      EvaluateSingle(node->getRightChild(), index, sVariableIds, oVariableData,
                                     ListOfParameters));
    case libsbml.AST_RELATIONAL_GT:
      return
          MathKGI.Gt(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                    ListOfParameters),
                     EvaluateSingle(node->getRightChild(), index, sVariableIds, oVariableData,
                                    ListOfParameters));
    case libsbml.AST_RELATIONAL_LEQ:
      return
          MathKGI.Leq(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                     ListOfParameters),
                      EvaluateSingle(node->getRightChild(), index, sVariableIds, oVariableData,
                                     ListOfParameters));
    case libsbml.AST_RELATIONAL_LT:
      return
          MathKGI.Lt(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                    ListOfParameters),
                     EvaluateSingle(node->getRightChild(), index, sVariableIds, oVariableData,
                                    ListOfParameters));
    case libsbml.AST_RELATIONAL_NEQ:
      return
          MathKGI.Neq(EvaluateSingle(node->getLeftChild(), index, sVariableIds, oVariableData,
                                     ListOfParameters),
                      EvaluateSingle(node->getRightChild(), index, sVariableIds, oVariableData,
                                     ListOfParameters));
    case libsbml.AST_FUNCTION:
      switch(node->getName())
      {
        case "max":
          return Evaluate(node->getChild(0),sVariableIds, oVariableData,
                                     ListOfParameters).Max();
        case "min":
          return Evaluate(node->getChild(0), sVariableIds, oVariableData,
                                     ListOfParameters).Min();
        case "sum":
          return Evaluate(node->getChild(0), sVariableIds, oVariableData,
                                     ListOfParameters).Sum();
        case "product":
          return Evaluate(node->getChild(0), sVariableIds, oVariableData,
                                     ListOfParameters).Aggregate( (a, b) => a*b);
        default:
          break;
      }
      break;
#endif
    default:
        break;
    }

    return 0;
}

// AST evaluation code adapted from:
//   https://sourceforge.net/p/libsedml/code/HEAD/tree/trunk/libSedML/DataGenerator.cs
static std::vector<double> Evaluate(const libsbml::ASTNode* tree,
                                    const VariableList& variables,
                                    const ParameterList& parameters)
{
    // Andre: don't understand this case?
    // no data
    // ANDRE if (oVariableData == null || oVariableData.Count < 1) return new double[0];

    // ANDRE: the case when its just a single variable
    if ((tree->getType() == libsbml::AST_NAME)
        && (variables.count(tree->getName())))
    {
        const Variable& variable = variables.at(tree->getName());
        return variable.data;
    }

    int length = 0;
    for (const auto& p: variables)
    {
        if (p.second.data.size() > length) length = p.second.data.size();
    }

    std::vector<double> result(length);
    for (int i = 0; i < length; ++i)
    {
        result[i] = EvaluateSingle(tree, i, variables, parameters);
    }
    return result;
}

int Data::computeData()
{
    computedData.clear();
    computedData = Evaluate(math, variables, parameters);
    return 0;
}

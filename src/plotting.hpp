#ifndef PLOTTING_H
#define PLOTTING_H

#include <vector>
#include <map>

#include "dataset.hpp"

typedef std::pair<DataCollection::iterator, DataCollection::iterator> XYpair;
typedef std::map<std::string, XYpair> CurveData;

int plot2d(const std::string& plotId, const CurveData& data,
           const std::string& baseOutputName);

#endif // PLOTTING_H

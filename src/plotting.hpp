#ifndef PLOTTING_H
#define PLOTTING_H

#include <vector>
#include <map>

#include "dataset.hpp"

typedef std::pair<DataCollection::iterator, DataCollection::iterator> XYpair;
typedef std::vector<XYpair> CurveData;

int plot2d(const CurveData& data);

#endif // PLOTTING_H

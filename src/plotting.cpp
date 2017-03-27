#include <iostream>

// common includes
#include <json/json.h>

#include "plotting.hpp"

int plot2d(const CurveData& curveData, const std::string& baseOutputName)
{
    for (const auto& c: curveData)
    {
        std::cout << "Creating curve: " << c.first << std::endl;
    }
#if 0
    // Prepare data to be plotted.
    const XYpair& c = data.back();
    int n = c.first->second.computedData.size();
    for (unsigned int i = 0; i < n ; ++i)
    {
        double x = c.first->second.computedData[i];
        double y = c.second->second.computedData[i];
        xmin = x < xmin ? x : xmin;
        xmax = x > xmax ? x : xmax;
        ymin = y < ymin ? y : ymin;
        ymax = y > ymax ? y : ymax;
    }

    std::string filename = baseOutputName;
    filename += "bob.svg";
    // Plot the data that was prepared above.
    pls->line(n, (PLFLT*)(c.first->second.computedData.data()),
              (PLFLT*)(c.second->second.computedData.data()));
#endif
    return 0;
}

#include <iostream>
#include <fstream>

// common includes
#include <json/json.h>
#include <json/json-forwards.h>

#include "plotting.hpp"

// http://stackoverflow.com/questions/26773043/how-to-write-a-template-converts-vector-to-jsonvalue-jsoncpp
template <typename Iterable>
Json::Value iterable2json(Iterable const& cont)
{
    Json::Value v;
    for (auto&& element: cont)
    {
        v.append(element);
    }
    return v;
}

int plot2d(const std::string& plotId, const CurveData& curveData,
           const std::string& baseOutputName)
{
    Json::Value chart;
    for (const auto& c: curveData)
    {
        std::cout << "Creating curve: " << c.first << std::endl;
        chart[c.first]["x"]["dgId"] = c.second.first->first;
        chart[c.first]["y"]["dgId"] = c.second.second->first;
        chart[c.first]["x"]["data"] = iterable2json(
                    c.second.first->second.computedData);
        chart[c.first]["y"]["data"] = iterable2json(
                    c.second.first->second.computedData);
    }
    // set output target
    std::streambuf* buf;
    std::ofstream of;
    if (baseOutputName.empty())
    {
        buf = std::cout.rdbuf();
    }
    else
    {
        std::string filename = baseOutputName;
        filename += plotId;
        filename += ".json";
        of.open(filename);
        buf = of.rdbuf();
    }
    std::ostream out(buf);
    out << chart;
    out << std::endl;
    //response = Json::FastWriter().write(charts);
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

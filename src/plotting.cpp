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
    Json::Value data;
    for (const auto& c: curveData)
    {
        Json::Value dataset;
        dataset["label"] = c.first;
        std::cout << "Creating curve: " << c.first << std::endl;
        dataset["dgIdX"] = c.second.first->first;
        dataset["dgIdY"] = c.second.second->first;
        const std::vector<double>& xData = c.second.first->second.computedData;
        const std::vector<double>& yData = c.second.second->second.computedData;
        for (unsigned int i = 0; i < xData.size(); ++i)
        {
            Json::Value point;
            point["x"] = xData[i];
            point["y"] = yData[i];
            dataset["data"].append(point);
        }
        data["datasets"].append(dataset);
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
    out << data;
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

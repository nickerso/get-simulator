#include <iostream>

// common includes
#include <plplot/plstream.h>

#include "plotting.hpp"

#ifdef PL_USE_NAMESPACE
using namespace std;
#endif

int plot2d(const CurveData& data)
{
    PLFLT xmin = 0., xmax = 0., ymin = 0., ymax = 0.;

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

    plstream* pls = new plstream();

    // Parse and process command line arguments
    //pls->parseopts( &argc, argv, PL_PARSE_FULL );

    // Initialize plplot
    pls->init();

    // Create a labelled box to hold the plot.
    pls->env(xmin, xmax, ymin, ymax, 0, 0);
    pls->lab("x", "y=100 x#u2#d", "Simple PLplot demo of a 2D line plot");

    // Plot the data that was prepared above.
    pls->line(n, (PLFLT*)(c.first->second.computedData.data()),
              (PLFLT*)(c.second->second.computedData.data()));

    // In C++ we don't call plend() to close PLplot library
    // this is handled by the destructor
    delete pls;
    return 0;
}

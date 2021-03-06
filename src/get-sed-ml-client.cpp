/*
 * get-sed-ml-client.cpp
 *
 *  Created on: Oct 10, 2013
 *      Author: dnic019
 */

/**
 * A client that uses CSim to run CellML/GET simulation experiments.
 */

#include <iostream>
#include <fstream>
#include <map>

#include "get_simulator_config.h"

#include "common.hpp"
#include "utils.hpp"
#include "sedml.hpp"

static void printVersion()
{
    std::cout << "GET Simulator (sed-ml client) version " << GET_SIMULATOR_VERSION_STRING
              << " (" << GET_SIMULATOR_VERSION << ")\n"
              << "http://get.readthedocs.io"
              << std::endl;
}

static void usage(const char* progName)
{
    std::cerr << "Usage: " << progName << " <SED-ML document URL> [report results file]" << std::endl;
    std::cerr << "\tWill output results to stdout if no report results file given" << std::endl;
}

int main(int argc, char* argv[])
{
    printVersion();
    if (argc < 2)
    {
        usage(argv[0]);
        return -1;
    }
    std::string url = buildAbsoluteUri(argv[1], "");
    std::string sedDocumentString = getUrlContent(url);
    if (sedDocumentString.empty())
    {
        std::cerr << "Unable to load document: " << url.c_str() << std::endl;
        usage(argv[0]);
        return -3;
    }
    //std::cout << "SED-ML document string: [[[" << sedDocumentString.c_str() << "]]]" << std::endl;
    Sedml sed;
    if (sed.parseFromString(sedDocumentString) != 0)
    {
        std::cerr << "Error parsing SED-ML document: " << url.c_str() << std::endl;
        return -1;
    }
    // make sure we should be able to execute all the required tasks. We use the SED-ML document URI
    // to resolve any relative URLs referenced as model sources.
    if (sed.buildExecutionManifest(url) != 0)
    {
        std::cerr << "There were errors building the simulation execution manifest." << std::endl;
        return -2;
    }

    // now we can actually execute the tasks
    if (sed.execute() != 0)
    {
        std::cerr << "There were some errors executing the simulation tasks." << std::endl;
        return -3;
    }

    std::fstream fs;
    if (argc > 2) fs.open(argv[2], std::fstream::out);
    // and generate the reports
    if (sed.serialiseReports(fs.is_open() ? fs : std::cout) != 0)
    {
        std::cerr << "There were some errors serialising the reports." << std::endl;
        return -4;
    }
    if (fs.is_open()) fs.close();

    //sed.checkBob();

    return 0;
}

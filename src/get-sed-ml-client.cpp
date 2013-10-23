/*
 * get-sed-ml-client.cpp
 *
 *  Created on: Oct 10, 2013
 *      Author: dnic019
 */

/**
 * A client that uses libsedml to run CellML/GET simulation experiments.
 */

#include <iostream>
#include <fstream>
#include <map>

#include "common.hpp"
#include "GeneralModel.hpp"
#include "cvodes.hpp"
#include "kinsol.hpp"
#include "utils.hpp"
#include "sedml.hpp"

static void usage(const char* progName)
{
    std::cerr << "Usage: " << progName << " <SED-ML document URL>" << std::endl;
}

int main(int argc, char* argv[])
{
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
    // make sure we should be able to execute all the required tasks.
    if (sed.buildExecutionManifest() != 0)
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

    //sed.checkBob();

    return 0;
}

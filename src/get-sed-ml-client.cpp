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
#include <sedml/SedTypes.h>

#include "common.hpp"
#include "GeneralModel.hpp"
#include "cvodes.hpp"
#include "kinsol.hpp"
#include "utils.hpp"

LIBSEDML_CPP_NAMESPACE_USE

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
    std::string sedDocumentString = getUrlContent(argv[1]);
    std::cout << "SED-ML document string: [[[" << sedDocumentString.c_str() << "]]]" << std::endl;
    SedDocument* doc;
    doc = readSedMLFromString(sedDocumentString.c_str());
    if (doc->getErrorLog()->getNumFailsWithSeverity(LIBSEDML_SEV_ERROR) > 0)
    {
        std::cerr << "Error loading SED-ML document: " << argv[1] << std::endl;
        std::cerr << doc->getErrorLog()->toString();
        delete doc;
        return -2;
    }
    return 0;
}

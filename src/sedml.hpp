#ifndef SEDML_HPP
#define SEDML_HPP

/**
 * @brief My wrapper around libSEDML to abstract the actual SED-ML processing logic in one place.
 */

#include <string>
#include <sedml/SedTypes.h>

#include "dataset.hpp"

class ExecutionManifest;

class Sedml
{
public:
    Sedml();
    ~Sedml();

    /**
     * @brief Parse the given SED-ML document contained in the provided string.
     * @param xmlDocument The SED-ML document to parse.
     * @return 0 if no error, otherwise the number of errors in the SED-ML document
     */
    int parseFromString(const std::string& xmlDocument);

    /**
     * @brief Snoop through this SED-ML document and build a list of the simulations that need to be executed.
     *
     *Run through the outputs for this SED-ML document and work out all the
     *simuation tasks that need to be executed. This method will fail if any
     *simulation tasks are outside the capabilities of this tool.
     *
     * @param baseUri The base URI used to resolve any relative URL references in model sources.
     * @return zero on success, non-zero on failure.
     */
    int buildExecutionManifest(const std::string& baseUri);

    /**
     * @brief Execute the simulation tasks required for this SED-ML document.
     * @return zero on success, non-zero on failure.
     */
    int execute();

    /**
     * @brief Serialise the reports that we know about, presumably after the simulation tasks have been executed.
     * @return zero on success.
     */
    int serialiseReports(std::ostream&);

    int checkBob();

private:
    libsedml::SedDocument* mSed;
    // the collection of all required data generators
    DataCollection* mDataCollection;
    // the list of all tasks that need to be executed
    ExecutionManifest* mExecutionManifest;
    bool mExecutionPerformed;

    /**
     * @brief Create our own representation of a data generator.
     * @param dgId The ID of the required data generator in the current SED-ML document.
     * @return The created data object.
     */
    Data createData(const std::string& dgId);
};

#endif // SEDML_HPP

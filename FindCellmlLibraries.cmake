# Sets CELLML_LIBRARIES to have any CellML libraries required for this platform to link

SET(CELLML_LIBRARIES "")

STRING( TOLOWER ${CMAKE_SYSTEM_NAME} OPERATING_SYSTEM )

# Linux wants to resolve symbols at link time?
if( ${OPERATING_SYSTEM} STREQUAL "linux" )

FIND_LIBRARY(CELLML_LIBRARY cellml
                ${CSIM_DEPENDENCY_DIR}/lib
        /usr/lib
        /usr/local/lib
)

FIND_LIBRARY(CCGS_LIBRARY ccgs
                ${CSIM_DEPENDENCY_DIR}/lib
        /usr/lib
        /usr/local/lib
)
FIND_LIBRARY(ANNO_TOOLS_LIBRARY annotools
                ${CSIM_DEPENDENCY_DIR}/lib
        /usr/lib
        /usr/local/lib
)
FIND_LIBRARY(CEVAS_LIBRARY cevas
                ${CSIM_DEPENDENCY_DIR}/lib
        /usr/lib
        /usr/local/lib
)
FIND_LIBRARY(CUSES_LIBRARY cuses
                ${CSIM_DEPENDENCY_DIR}/lib
        /usr/lib
        /usr/local/lib
)
FIND_LIBRARY(MALAES_LIBRARY malaes
                ${CSIM_DEPENDENCY_DIR}/lib
        /usr/lib
        /usr/local/lib
)

SET(CELLML_LIBRARIES ${CELLML_LIBRARY} ${CCGS_LIBRARY} ${ANNO_TOOLS_LIBRARY} ${CEVAS_LIBRARY} ${CUSES_LIBRARY} ${MALAES_LIBRARY})

endif( ${OPERATING_SYSTEM} STREQUAL "linux" )


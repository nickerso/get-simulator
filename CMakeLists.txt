cmake_minimum_required(VERSION 3.5)
cmake_policy(SET CMP0048 NEW)

# General Epithelial Transport simulation tool
PROJECT(get-simulator
    VERSION 1.0.1
    LANGUAGES CXX)

# Add in the directory with the FindCellML module
#SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${get-simulator-sedml_SOURCE_DIR})

#IF(WIN32)
#    SET(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
#ELSE(WIN32)
#    SET(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
#ENDIF(WIN32)

find_package(sundials_cvode_static CONFIG REQUIRED)
find_package(sundials_kinsol_static CONFIG REQUIRED)
find_package(sundials_nvecserial_static CONFIG REQUIRED)
find_package(sedml-static CONFIG REQUIRED)
find_package(numl-static CONFIG REQUIRED)
find_package(sbml-static CONFIG REQUIRED)
find_package(LibXml2 CONFIG REQUIRED)

find_package(csim CONFIG REQUIRED)

# include(FindCellmlLibraries)

set(PLATFORM_LIBS "curl")

# Set compiler flags
if (WIN32)
else(WIN32)
    add_definitions("-std=c++11" "-Wall" "-Werror")
    set(PLATFORM_LIBS ${PLATFORM_LIBS} "m")
endif(WIN32)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
   list(APPEND PLATFORM_LIBS "z" "lzma" "bz2")
endif()

# Global includes?
INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/src
  #${CLIBSEDML_INCLUDE_DIRS}
  #${LIBSBML_INCLUDE_DIRS}
  #${SUNDIALS_INCLUDE_DIR}
  #${LIBXML2_INCLUDE_DIR}
)
#/Users/dnic019/shared-folders/resources/std-libs/libsbml/5.8.0/include

# Sources
SET(COMMON_SRCS
  src/molecule.cpp
  src/GeneralModel.cpp
  src/common.cpp
  src/cvodes.cpp
  src/kinsol.cpp
  src/utils.cpp
)

SET(get_SRCS
  src/get-simulator.cpp
  ${COMMON_SRCS}
)
SET(get_sedml_SRCS
  src/sedml.cpp
  src/dataset.cpp
  src/simulationenginecsim.cpp
  src/simulationengineget.cpp
  src/get-sed-ml-client.cpp
  ${COMMON_SRCS}
)


###
## http://www.cmake.org/Wiki/CMake_RPATH_handling
###

# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
SET(CMAKE_MACOSX_RPATH  TRUE)

set(GET_EXECUTABLE_NAME "get-simulator")
ADD_EXECUTABLE(${GET_EXECUTABLE_NAME} ${get_SRCS})
TARGET_LINK_LIBRARIES(${GET_EXECUTABLE_NAME}
  csim
  sundials_cvode_static
  sundials_kinsol_static
  sundials_nvecserial_static
  xml2
  ${PLATFORM_LIBS}
)

set(GET_SEDML_EXECUTABLE_NAME "get-sed-ml-client")
ADD_EXECUTABLE(${GET_SEDML_EXECUTABLE_NAME} ${get_sedml_SRCS})
TARGET_LINK_LIBRARIES(${GET_SEDML_EXECUTABLE_NAME}
  PUBLIC
  csim
  sedml-static
  numl-static
  sbml-static
  sundials_cvode_static
  sundials_kinsol_static
  sundials_nvecserial_static
  xml2
  ${PLATFORM_LIBS}
)
#/Users/dnic019/shared-folders/resources/std-libs/libsbml/5.8.0/lib/libsbml.dylib

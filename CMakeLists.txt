# General Epithelial Transport simulation tool
PROJECT(get-simulator)

cmake_minimum_required(VERSION 2.8)

# Add in the directory with the FindCellML module
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${get-simulator_SOURCE_DIR})

IF(WIN32)
    SET(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
ELSE(WIN32)
    SET(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
ENDIF(WIN32)

FIND_PACKAGE(SUNDIALS REQUIRED)
FIND_PACKAGE(LibCSim REQUIRED)
FIND_PACKAGE(LibCsedml REQUIRED)
FIND_PACKAGE(LibXml2 REQUIRED QUIET)

set(PLATFORM_LIBS "curl")

# Set compiler flags
if (WIN32)
else(WIN32)
    add_definitions("-std=c++11" "-Wall" "-Werror")
    set(PLATFORM_LIBS ${PLATFORM_LIBS} "m")
endif(WIN32)

# Global includes?
INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/src
  ${CLIBSEDML_INCLUDE_DIRS}
  ${LIBCSIM_INCLUDE_DIRS}
  ${SUNDIALS_INCLUDE_DIR}
  ${LIBXML2_INCLUDE_DIR}
)

# Sources
SET(COMMON_SRCS
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
  src/get-sed-ml-client.cpp
  ${COMMON_SRCS}
)

set(GET_EXECUTABLE_NAME "get-simulator")
ADD_EXECUTABLE(${GET_EXECUTABLE_NAME} ${get_SRCS})
TARGET_LINK_LIBRARIES(${GET_EXECUTABLE_NAME}
  ${libcsim_LIBRARIES}
  ${SUNDIALS_LIBRARIES}
  ${LIBXML2_LIBRARIES}
  ${PLATFORM_LIBS}
)

set(GET_SEDML_EXECUTABLE_NAME "get-sed-ml-client")
ADD_EXECUTABLE(${GET_SEDML_EXECUTABLE_NAME} ${get_sedml_SRCS})
TARGET_LINK_LIBRARIES(${GET_SEDML_EXECUTABLE_NAME}
  ${libcsim_LIBRARIES}
  ${CLIBSEDML_LIBRARIES}
  ${SUNDIALS_LIBRARIES}
  ${LIBXML2_LIBRARIES}
  ${PLATFORM_LIBS}
)

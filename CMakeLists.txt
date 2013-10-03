# General Epithelial Transport simulation tool
PROJECT(get-simulator)

cmake_minimum_required(VERSION 2.8)

# Add in the directory with the FindCellML module
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${get_SOURCE_DIR})

IF(WIN32)
    SET(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
ELSE(WIN32)
    SET(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
ENDIF(WIN32)

FIND_PACKAGE(SUNDIALS REQUIRED)

# Set compiler flags
if (WIN32)
else(WIN32)
    add_definitions(-Wall -Werror)
    set(PLATFORM_LIBS ${PLATFORM_LIBS} "m")
endif(WIN32)

# Global includes?
INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/src
  ${SUNDIALS_INCLUDE_DIR}
)

# Sources
SET(get_SRCS
  src/get.cpp
  src/GeneralModel.cpp
  src/common.cpp
  src/cvodes.cpp
  src/kinsol.cpp
)

set(EXECUTABLE_NAME "get-simulator")
ADD_EXECUTABLE(${EXECUTABLE_NAME} ${get_SRCS})
TARGET_LINK_LIBRARIES(${EXECUTABLE_NAME}
  ${SUNDIALS_LIBRARIES}
  ${PLATFORM_LIBS}
)

# General Epithelial Transport
PROJECT(get)

cmake_minimum_required(VERSION 2.8)

# Add in the directory with the FindCellML module
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${get_SOURCE_DIR})

IF(WIN32)
    SET(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
ELSE(WIN32)
    SET(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
ENDIF(WIN32)

FIND_PACKAGE(SUNDIALS REQUIRED)

set(USE_KINSOL 1)

# Set compiler flags
if (WIN32)
else(WIN32)
    add_definitions(-Wall -Werror)
    set(PLATFORM_LIBS ${PLATFORM_LIBS} "m")
endif(WIN32)

if(${USE_KINSOL})
    add_definitions(-DUSE_KINSOL)
endif()

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
)

# Sources
SET(ida_SRCS
  src/ida.cpp
  src/GeneralModel.cpp
  src/common.cpp
)

if(${USE_KINSOL})
  set(get_SRCS ${get_SRCS} src/kinsol.cpp)
else()
  set(get_SRCS ${get_SRCS} src/NewtonRaphson.cpp)
endif()

set(EXECUTABLE_NAME "get")
ADD_EXECUTABLE(${EXECUTABLE_NAME} ${get_SRCS})
TARGET_LINK_LIBRARIES(${EXECUTABLE_NAME}
  ${SUNDIALS_LIBRARIES}
  ${PLATFORM_LIBS}
)

ADD_EXECUTABLE(ida ${ida_SRCS})
set_target_properties(ida PROPERTIES COMPILE_DEFINITIONS "IDA")
TARGET_LINK_LIBRARIES(ida
  ${SUNDIALS_LIBRARIES}
  ${PLATFORM_LIBS}
)

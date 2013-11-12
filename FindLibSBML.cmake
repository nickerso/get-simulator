# - Try to find libcsim
# Once done, this will define
#
#  LIBSBML_FOUND - system has libsbml
#  LIBSBML_INCLUDE_DIR - the libsbml include directories
#  LIBSBML_LIBRARIES - link these to use libsbml

# Include dir
find_path(LIBSBML_INCLUDE_DIR sbml/SBMLDocument.h)

# Finally the library itself
find_library(LIBSBML_LIBRARY sbml)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibSBML LIBSBML_LIBRARY LIBSBML_INCLUDE_DIR)

if(LIBSBML_FOUND)
  SET(LIBSBML_INCLUDE_DIRS ${LIBSBML_INCLUDE_DIR})
  SET(LIBSBML_LIBRARIES ${LIBSBML_LIBRARY})
endif(LIBSBML_FOUND)

mark_as_advanced(LIBSBML_LIBRARY LIBSBML_INCLUDE_DIR)



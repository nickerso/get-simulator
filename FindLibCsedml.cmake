# - Try to find clibsedml
# Once done, this will define
#
#  CLIBSEDML_FOUND        - clibsedml was found
#  CLIBSEDML_INCLUDE_DIRS - the clibsedml include directories
#  CLIBSEDML_LIBRARIES    - link these to use clibsedml

#include(LibFindMacros)

# Use pkg-config to get hints about paths
#libfind_pkg_check_modules(clibsedml_PKGCONF clibsedml)

# Include dir
find_path(CLIBSEDML_INCLUDE_DIR
  NAMES sedml/SedTypes.h
  PATHS ${clibsedml_PKGCONF_INCLUDE_DIRS}
)

# Finally the library itself
find_library(CLIBSEDML_LIBRARY
  NAMES ${CLIBSEDML_NAMES} sedml
  PATHS ${clibsedml_PKGCONF_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(cLibsedml CLIBSEDML_LIBRARY CLIBSEDML_INCLUDE_DIR)

if(CLIBSEDML_FOUND)
  set(CLIBSEDML_INCLUDE_DIRS ${CLIBSEDML_INCLUDE_DIR})
  set(CLIBSEDML_LIBRARIES ${CLIBSEDML_LIBRARY})
endif(CLIBSEDML_FOUND)

mark_as_advanced(CLIBSEDML_LIBRARY CLIBSEDML_INCLUDE_DIR)

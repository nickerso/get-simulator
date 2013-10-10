# - Try to find libcsim
# Once done, this will define
#
#  LIBCSIM_FOUND - system has libmicrohttpd
#  LIBCSIM_INCLUDE_DIR - the libmicrohttpd include directories
#  LIBCSIM_LIBRARIES - link these to use libmicrohttpd

# Include dir
find_path(LIBCSIM_INCLUDE_DIR CellmlSimulator.hpp)

# Finally the library itself
find_library(LIBCSIM_LIBRARY csim)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibCSim LIBCSIM_LIBRARY LIBCSIM_INCLUDE_DIR)

if(LIBCSIM_FOUND)
  SET(LIBCSIM_INCLUDE_DIRS ${LIBCSIM_INCLUDE_DIR})
  SET(LIBCSIM_LIBRARIES ${LIBCSIM_LIBRARY})
endif(LIBCSIM_FOUND)

mark_as_advanced(LIBCSIM_LIBRARY LIBCSIM_INCLUDE_DIR)



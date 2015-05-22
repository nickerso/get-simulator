# - Find Sundials libraries that we need
# Find the CVODE and KINSOL includes and libraries

FIND_PATH(SUNDIALS_INCLUDE_DIR cvode/cvode.h
        /usr/include/
        /usr/local/include/
)

FIND_LIBRARY(CVODE_LIBRARY sundials_cvode
        /usr/lib
        /usr/local/lib
)

FIND_LIBRARY(KINSOL_LIBRARY sundials_kinsol
        /usr/lib
        /usr/local/lib
) 

FIND_LIBRARY(NVECTOR_SERIAL_LIBRARY sundials_nvecserial
        /usr/lib
        /usr/local/lib
) 

IF (SUNDIALS_INCLUDE_DIR AND CVODE_LIBRARY AND KINSOL_LIBRARY AND NVECTOR_SERIAL_LIBRARY)
   SET(SUNDIALS_LIBRARIES ${CVODE_LIBRARY} ${KINSOL_LIBRARY} ${NVECTOR_SERIAL_LIBRARY})
   SET(SUNDIALS_FOUND TRUE)
ENDIF ()


IF (SUNDIALS_FOUND)
   IF (NOT SUNDIALS_FIND_QUIETLY)
      MESSAGE(STATUS "Found SUNDIALS: ${SUNDIALS_LIBRARIES}")
   ENDIF ()
ELSE (SUNDIALS_FOUND)
   IF (SUNDIALS_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find SUNDIALS")
   ELSE (SUNDIALS_FIND_REQUIRED)
      MESSAGE(STATUS "SUNDIALS not found")
   ENDIF (SUNDIALS_FIND_REQUIRED)
ENDIF (SUNDIALS_FOUND)

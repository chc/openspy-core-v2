find_path(JANSSON_INCLUDE_DIR NAMES jansson.h PATHS ${JANSSON_ROOT_DIR})

find_library(JANSSON_LIBRARIES NAMES jansson PATHS ${JANSSON_ROOT_DIR})

if (JANSSON_INCLUDE_DIR AND JANSSON_LIBRARIES)
  if (NOT TARGET jansson::jansson)
    add_library(jansson::jansson UNKNOWN IMPORTED)
    set_target_properties(jansson::jansson PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${JANSSON_INCLUDE_DIR} IMPORTED_LOCATION ${JANSSON_LIBRARIES})
  endif ()
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(jansson REQUIRED_VARS JANSSON_INCLUDE_DIR JANSSON_LIBRARIES)

if(JANSSON_FOUND)
  message(STATUS "Found jansson - (include: ${JANSSON_INCLUDE_DIR}, library: ${JANSSON_LIBRARIES})")
  mark_as_advanced(JANSSON_INCLUDE_DIR JANSSON_LIBRARIES)
endif()

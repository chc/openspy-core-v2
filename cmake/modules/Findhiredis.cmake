find_path(HIREDIS_INCLUDE_DIR NAMES hiredis/hiredis.h PATHS ${HIREDIS_ROOT_DIR} ${HIREDIS_ROOT_DIR}/include)

find_library(HIREDIS_LIBRARIES NAMES hiredis PATHS ${HIREDIS_ROOT_DIR} ${HIREDIS_ROOT_DIR}/lib)

if (HIREDIS_INCLUDE_DIR AND HIREDIS_LIBRARIES)
  if (NOT TARGET hiredis::hiredis)
    add_library(hiredis::hiredis UNKNOWN IMPORTED)
    set_target_properties(hiredis::hiredis PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${HIREDIS_INCLUDE_DIR} IMPORTED_LOCATION ${HIREDIS_LIBRARIES})
  endif ()
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(hiredis REQUIRED_VARS HIREDIS_INCLUDE_DIR HIREDIS_LIBRARIES)

if(HIREDIS_FOUND)
  message(STATUS "Found hiredis - (include: ${HIREDIS_INCLUDE_DIR}, library: ${HIREDIS_LIBRARIES})")
  mark_as_advanced(HIREDIS_INCLUDE_DIR HIREDIS_LIBRARIES)
endif()

find_path(HIREDIS_SSL_INCLUDE_DIR NAMES hiredis/hiredis_ssl.h PATHS ${HIREDIS_SSL_ROOT_DIR} ${HIREDIS_SSL_ROOT_DIR}/include)

find_library(HIREDIS_SSL_LIBRARIES NAMES hiredis_ssl PATHS ${HIREDIS_SSL_ROOT_DIR} ${HIREDIS_SSL_ROOT_DIR}/lib)

if (HIREDIS_SSL_INCLUDE_DIR AND HIREDIS_SSL_LIBRARIES)
  if (NOT TARGET hiredis::hiredis_ssl)
    add_library(hiredis::hiredis_ssl UNKNOWN IMPORTED)
    set_target_properties(hiredis::hiredis_ssl PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${HIREDIS_SSL_INCLUDE_DIR} IMPORTED_LOCATION ${HIREDIS_SSL_LIBRARIES})
  endif ()
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(hiredis_ssl REQUIRED_VARS HIREDIS_SSL_INCLUDE_DIR HIREDIS_SSL_LIBRARIES)

if(HIREDIS_SSL_FOUND)
  message(STATUS "Found hiredis_ssl - (include: ${HIREDIS_SSL_INCLUDE_DIR}, library: ${HIREDIS_SSL_LIBRARIES})")
  mark_as_advanced(HIREDIS_SSL_INCLUDE_DIR HIREDIS_SSL_LIBRARIES)
endif()


find_package(PkgConfig QUIET)
pkg_check_modules(PC_RABBITMQ QUIET rabbitmq)

find_path(RABBITMQ_INCLUDE_DIR
        NAMES rabbitmq-c/amqp.h
        HINTS ${PC_RABBITMQ_INCLUDEDIR} ${PC_RABBITMQ_INCLUDE_DIRS})

find_library(RABBITMQ_LIBRARY
        NAMES rabbitmq librabbitmq
        HINTS ${PC_RABBITMQ_LIBDIR} ${PC_RABBITMQ_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(rabbitmq
        REQUIRED_VARS RABBITMQ_LIBRARY RABBITMQ_INCLUDE_DIR)

if (RABBITMQ_FOUND)
    set(RABBITMQ_LIBRARIES ${RABBITMQ_LIBRARY})
    set(RABBITMQ_INCLUDE_DIRS ${RABBITMQ_INCLUDE_DIR})
    if (NOT TARGET rabbitmq::rabbitmq)
        add_library(rabbitmq::rabbitmq UNKNOWN IMPORTED)
        set_target_properties(rabbitmq::rabbitmq PROPERTIES
                IMPORTED_LOCATION "${RABBITMQ_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${RABBITMQ_INCLUDE_DIR}")
    endif ()
endif ()

mark_as_advanced(RABBITMQ_INCLUDE_DIR RABBITMQ_LIBRARY)
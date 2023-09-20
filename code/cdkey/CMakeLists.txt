cmake_minimum_required (VERSION 3.5)

project(cdkey)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

file (GLOB MAIN_SRCS "*.cpp")
file (GLOB MAIN_HDRS "*.h")
file (GLOB SERVER_SRCS "server/*.cpp")
file (GLOB SERVER_HDRS "server/*.h")
file (GLOB SERVER_HANDLERS_SRCS "server/handlers/*.cpp")
file (GLOB SERVER_HANDLERS_HDRS "server/handlers/*.h")


set (ALL_SRCS ${MAIN_SRCS} ${MAIN_HDRS} ${SERVER_SRCS} ${SERVER_HDRS} ${SERVER_HANDLERS_SRCS} ${SERVER_HANDLERS_HDRS})

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

source_group("Sources" FILES ${MAIN_SRCS})
source_group("Sources\\Server" FILES ${SERVER_SRCS})
source_group("Sources\\Server\\handlers" FILES ${SERVER_HANDLERS_SRCS})

source_group("Headers" FILES ${MAIN_HDRS})
source_group("Headers\\Server" FILES ${SERVER_HDRS})
source_group("Headers\\Server\\handlers" FILES ${SERVER_HANDLERS_HDRS})


add_executable (cdkey ${ALL_SRCS})

target_compile_features(cdkey PRIVATE cxx_unrestricted_unions)

IF(WIN32)	
	target_link_libraries(cdkey ws2_32.lib openspy.lib)
ELSE() #unix
	target_link_libraries(cdkey pthread openspy)
ENDIF()
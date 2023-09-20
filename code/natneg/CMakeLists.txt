cmake_minimum_required (VERSION 3.5)

project(natneg)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

file (GLOB MAIN_SRCS "*.cpp")
file (GLOB MAIN_HDRS "*.h")
file (GLOB SERVER_SRCS "server/*.cpp")
file (GLOB SERVER_HDRS "server/*.h")

file (GLOB TASKS_HDRS "tasks/*.h")
file (GLOB TASKS_SRCS "tasks/*.cpp")

file (GLOB HANDLERS_HDRS "server/handlers/*.h")
file (GLOB HANDLERS_SRCS "server/handlers/*.cpp")


set (ALL_SRCS ${MAIN_SRCS} ${MAIN_HDRS} ${SERVER_SRCS} ${SERVER_HDRS} ${TASKS_SRCS} ${TASKS_HDRS} ${HANDLERS_SRCS} ${HANDLERS_HDRS})

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

source_group("Sources" FILES ${MAIN_SRCS})
source_group("Sources\\Server" FILES ${SERVER_SRCS})

source_group("Headers" FILES ${MAIN_HDRS})
source_group("Headers\\Server" FILES ${SERVER_HDRS})


add_executable (natneg ${ALL_SRCS})

target_compile_features(natneg PRIVATE cxx_unrestricted_unions)

IF(WIN32)	
	target_link_libraries(natneg ws2_32.lib openspy.lib)
ELSE() #unix
	target_link_libraries(natneg pthread openspy)
ENDIF()
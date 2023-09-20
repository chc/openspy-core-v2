cmake_minimum_required (VERSION 3.5)

project(gstats)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

file (GLOB MAIN_SRCS "*.cpp")
file (GLOB MAIN_HDRS "*.h")
file (GLOB SERVER_SRCS "server/*.cpp")
file (GLOB SERVER_HDRS "server/*.h")
file (GLOB SERVER_TASKS_SRCS "server/tasks/*.cpp")
file (GLOB SERVER_TASKS_HDRS "server/tasks/*.h")
file (GLOB SERVER_COMMANDS_SRCS "server/commands/*.cpp")
file (GLOB SERVER_COMMANDS_HDRS "server/commands/*.h")

set (ALL_SRCS ${MAIN_SRCS} ${MAIN_HDRS} ${SERVER_SRCS} ${SERVER_HDRS} ${SERVER_TASKS_SRCS} ${SERVER_TASKS_HDRS} ${SERVER_COMMANDS_SRCS} ${SERVER_COMMANDS_HDRS})

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

source_group("Sources" FILES ${MAIN_SRCS})
source_group("Sources\\Server" FILES ${SERVER_SRCS})
source_group("Sources\\Server\\Commands" FILES ${SERVER_COMMANDS_SRCS})
source_group("Sources\\Server\\Tasks" FILES ${SERVER_TASKS_SRCS})

source_group("Headers" FILES ${MAIN_HDRS})
source_group("Headers\\Server" FILES ${SERVER_HDRS})
source_group("Headers\\Server\\Commands" FILES ${SERVER_COMMANDS_HDRS})
source_group("Headers\\Server\\tasks" FILES ${SERVER_TASKS_HDRS})

add_executable (gstats ${ALL_SRCS})

IF(WIN32)	
	target_link_libraries(gstats ws2_32.lib openspy.lib)
ELSE() #unix
	target_link_libraries(gstats pthread openspy)
ENDIF()
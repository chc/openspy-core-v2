cmake_minimum_required (VERSION 3.5)

project(FESL)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

file (GLOB MAIN_SRCS "*.cpp")
file (GLOB MAIN_HDRS "*.h")
file (GLOB SERVER_SRCS "server/*.cpp")
file (GLOB SERVER_HDRS "server/*.h")

file (GLOB SERVER_BASIC_COMMANDS_SRCS "server/commands/basic/*.cpp")
file (GLOB SERVER_BASIC_COMMANDS_HDRS "server/commands/basic/*.h")
file (GLOB SERVER_NU_COMMANDS_SRCS "server/commands/nu/*.cpp")
file (GLOB SERVER_NU_COMMANDS_HDRS "server/commands/nu/*.h")
file (GLOB SERVER_TASKS_SRCS "server/tasks/*.cpp")
file (GLOB SERVER_TASKS_HDRS "server/tasks/*.h")


set (ALL_SRCS ${MAIN_SRCS} ${MAIN_HDRS} ${SERVER_SRCS} ${SERVER_HDRS} ${SERVER_BASIC_COMMANDS_SRCS} ${SERVER_NU_COMMANDS_SRCS} ${SERVER_TASKS_SRCS} ${SERVER_TASKS_HDRS})

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

source_group("Sources" FILES ${MAIN_SRCS})
source_group("Sources\\Server" FILES ${SERVER_SRCS})
source_group("Sources\\Tasks" FILES ${SERVER_TASKS_SRCS})
source_group("Sources\\Server\\commands\\basic" FILES ${SERVER_BASIC_COMMANDS_SRCS})
source_group("Sources\\Server\\commands\\nu" FILES ${SERVER_NU_COMMANDS_SRCS})

source_group("Headers" FILES ${MAIN_HDRS})
source_group("Headers\\Server" FILES ${SERVER_HDRS})
source_group("Headers\\Tasks" FILES ${SERVER_TASKS_HDRS})
source_group("Headers\\Server\\commands\\basic" FILES ${SERVER_BASIC_COMMANDS_HDRS})
source_group("Headers\\Server\\commands\\nu" FILES ${SERVER_NU_COMMANDS_HDRS})
add_executable (FESL ${ALL_SRCS})

IF(WIN32)	
	target_link_libraries(FESL ws2_32.lib openspy.lib)
ELSE() #unix
	target_link_libraries(FESL pthread openspy)
ENDIF()
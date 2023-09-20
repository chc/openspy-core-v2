cmake_minimum_required (VERSION 3.5)

project(qr)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

file (GLOB MAIN_SRCS "*.cpp")
file (GLOB MAIN_HDRS "*.h")
file (GLOB SERVER_SRCS "server/*.cpp")
file (GLOB SERVER_HDRS "server/*.h")

file (GLOB V2_SERVER_SRCS "server/v2/*.cpp")
file (GLOB V2_SERVER_HDRS "server/v2/*.h")
file (GLOB V1_SERVER_SRCS "server/v1/*.cpp")
file (GLOB V1_SERVER_HDRS "server/v1/*.h")

file (GLOB TASKS_SRCS "tasks/*.cpp")
file (GLOB TASKS_HDRS "tasks/*.h")


set (ALL_SRCS ${SERVER_SRCS} ${SERVER_HDRS} ${V2_SERVER_SRCS} ${V2_SERVER_HDRS} ${V1_SERVER_SRCS} ${V1_SERVER_HDRS} ${TASKS_SRCS})

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

source_group("Sources" FILES ${MAIN_SRCS})
source_group("Sources\\Server" FILES ${SERVER_SRCS})
source_group("Sources\\Tasks" FILES ${TASK_SRCS})

source_group("Headers" FILES ${MAIN_HDRS})
source_group("Headers\\Server" FILES ${SERVER_HDRS})
source_group("Headers\\Tasks" FILES ${TASK_HDRS})

add_executable (qr ${ALL_SRCS} ${MAIN_SRCS} ${MAIN_HDRS})

IF(WIN32)	
	target_link_libraries(qr ws2_32.lib openspy.lib)
ELSE() #unix
	target_link_libraries(qr pthread openspy)
ENDIF()
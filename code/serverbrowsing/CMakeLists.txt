cmake_minimum_required (VERSION 3.5)

project(serverbrowsing)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

file (GLOB MAIN_SRCS "*.cpp")
file (GLOB MAIN_HDRS "*.h")
file (GLOB SERVER_SRCS "server/*.cpp")
file (GLOB SERVER_HDRS "server/*.h")
file (GLOB FILTER_SRCS "filter/*.cpp")
file (GLOB FILTER_HDRS "filter/*.h")
file (GLOB TASKS_SRCS "tasks/*.cpp")
file (GLOB TASKS_HDRS "tasks/*.h")


set (ALL_SRCS ${SERVER_SRCS} ${SERVER_HDRS} ${FILTER_SRCS} ${FILTER_HDRS} ${TASKS_SRCS})

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

source_group("Sources" FILES ${MAIN_SRCS})
source_group("Sources\\Server" FILES ${SERVER_SRCS})
source_group("Sources\\Filter" FILES ${FILTER_SRCS})
source_group("Sources\\Tasks" FILES ${TASKS_SRCS})

source_group("Headers" FILES ${MAIN_HDRS})
source_group("Headers\\Server" FILES ${SERVER_HDRS})
source_group("Headers\\Filter" FILES ${FILTER_HDRS})
source_group("Headers\\Tasks" FILES ${TASKS_HDRS})

add_executable (serverbrowsing ${ALL_SRCS} ${MAIN_SRCS} ${MAIN_HDRS})

IF(WIN32)	
	target_link_libraries(serverbrowsing ws2_32.lib openspy.lib)
ELSE() #unix
	target_link_libraries(serverbrowsing pthread openspy)
ENDIF()
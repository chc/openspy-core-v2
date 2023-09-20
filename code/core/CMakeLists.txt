cmake_minimum_required (VERSION 3.5)

project(openspy_core)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

find_package(SSL)
find_package(hiredis REQUIRED)
find_package(Jansson REQUIRED)
find_package(pugixml REQUIRED)
find_package(rabbitmq-c REQUIRED)
find_package(CURL REQUIRED)

#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

file (GLOB OS_SRCS "OS/*.cpp")
file (GLOB OS_HDRS "OS/*.h")
file (GLOB NET_SRCS "OS/Net/*.cpp")
file (GLOB NET_HDRS "OS/Net/*.h")

file (GLOB CONFIG_SRCS "OS/Config/*.cpp")
file (GLOB CONFIG_HDRS "OS/Config/*.h")

file (GLOB NET_EVENTMGR_SRCS "OS/Net/EventMgrs/*.cpp")
file (GLOB NET_EVENTMGR_HDRS "OS/Net/EventMgrs/*.h")
file (GLOB NET_IOIFACES_SRCS "OS/Net/IOIfaces/*.cpp")
file (GLOB NET_IOIFACES_HDRS "OS/Net/IOIfaces/*.h")
file (GLOB NET_PROCESSORS_SRCS "OS/Net/Processors/*.cpp")
file (GLOB NET_PROCESSORS_HDRS "OS/Net/Processors/*.h")

file (GLOB NET_DRIVERS_SRCS "OS/Net/drivers/*.cpp")
file (GLOB NET_DRIVERS_HDRS "OS/Net/drivers/*.h")


file (GLOB LEGACY_SRCS "OS/gamespy/*.cpp" "OS/gamespy/*.c")
file (GLOB LEGACY_HDRS "OS/gamespy/*.h")
file (GLOB LOGGER_SRCS "OS/Logger/*.cpp")
file (GLOB LOGGER_HDRS "OS/Logger/*.h")
file (GLOB TIMER_SRCS "OS/Timer/*.cpp")
file (GLOB TIMER_HDRS "OS/Timer/*.h")
file (GLOB TASK_HDRS "OS/Task/*.h")

file (GLOB SHAREDTASK_ACCOUNT_SRCS "OS/SharedTasks/Account/*.cpp")
file (GLOB SHAREDTASK_AUTH_SRCS "OS/SharedTasks/Auth/*.cpp")
file (GLOB SHAREDTASK_CDKEY_SRCS "OS/SharedTasks/CDKey/*.cpp")
file (GLOB SHAREDTASK_GEO_SRCS "OS/SharedTasks/Geo/*.cpp")
file (GLOB SHAREDTASK_ACCOUNT_TASKS_SRCS "OS/SharedTasks/Account/tasks/*.cpp")
file (GLOB SHAREDTASK_AUTH_TASKS_SRCS "OS/SharedTasks/Auth/tasks/*.cpp")
file (GLOB SHAREDTASK_CDKEY_TASKS_SRCS "OS/SharedTasks/CDKey/tasks/*.cpp")
file (GLOB SHAREDTASK_GEO_TASKS_SRCS "OS/SharedTasks/Geo/tasks/*.cpp")
file (GLOB SHAREDTASK_HDRS "OS/SharedTasks/*.h")
file (GLOB SHAREDTASK_ACCOUNT_HDRS "OS/SharedTasks/Account/*.h")
file (GLOB SHAREDTASK_AUTH_HDRS "OS/SharedTasks/Auth/*.h")
file (GLOB SHAREDTASK_CDKEY_HDRS "OS/SharedTasks/CDKey/*.h")
file (GLOB SHAREDTASK_GEO_HDRS "OS/SharedTasks/Geo/*.h")


file (GLOB MQ_SRCS "OS/MessageQueue/*.cpp" "OS/MessageQueue/rabbitmq/*.cpp")
file (GLOB MQ_HDRS "OS/MessageQueue/*.h" "OS/MessageQueue/rabbitmq/*.h")


set (ALL_SRCS ${SSL_SRCS} ${SERVER_SRCS} ${SERVER_HDRS} ${LEGACY_SRCS} ${LEGACY_HDRS} ${CONFIG_SRCS} ${CONFIG_HDRS} ${OS_SRCS} ${OS_HDRS} ${NET_SRCS} ${NET_HDRS} ${NET_DRIVERS_SRCS} ${NET_DRIVERS_HDRS} ${LOGGER_SRCS} ${LOGGER_HDRS} ${TIMER_SRCS} ${TIMER_HDRS} ${NET_EVENTMGR_SRCS} ${NET_IOIFACES_SRCS} ${NET_PROCESSORS_SRCS} ${NET_PROCESSORS_HDRS} ${MQ_SRCS} ${MQ_HDRS} ${TASK_HDRS} ${SHAREDTASK_ACCOUNT_SRCS} ${SHAREDTASK_AUTH_SRCS} ${SHAREDTASK_ACCOUNT_TASKS_SRCS} ${SHAREDTASK_AUTH_TASKS_SRCS} ${SHAREDTASK_CDKEY_SRCS} ${SHAREDTASK_CDKEY_TASKS_SRCS} ${SHAREDTASK_CDKEY_HDRS} ${SHAREDTASK_GEO_SRCS} ${SHAREDTASK_GEO_HDRS} ${SHAREDTASK_GEO_TASKS_SRCS})

include_directories (${CMAKE_CURRENT_SOURCE_DIR} ${hiredis_INCLUDE_DIRS})

source_group("Sources\\OS\\Net" FILES ${NET_SRCS})
source_group("Sources\\OS\\Net\\EventMgrs" FILES ${NET_EVENTMGR_SRCS})
source_group("Sources\\OS\\Net\\IOFaces" FILES ${NET_IOIFACES_SRCS})
source_group("Sources\\OS\\Net\\Drivers" FILES ${NET_DRIVERS_SRCS})
source_group("Sources\\OS\\Gamespy" FILES ${LEGACY_SRCS})
source_group("Sources\\OS\\Logger" FILES ${LOGGER_SRCS})


source_group("Headers\\OS\\Gamespy" FILES ${LEGACY_HDRS})
source_group("Headers\\OS\\Net" FILES ${NET_HDRS})
source_group("Headers\\OS\\Net\\EventMgrs" FILES ${NET_EVENTMGR_HDRS})
source_group("Headers\\OS\\Net\\IOFaces" FILES ${NET_IOIFACES_HDRS})
source_group("Headers\\OS\\Net\\Drivers" FILES ${NET_DRIVERS_SRCS})

source_group("Sources\\OS\\Logger" FILES ${LOGGER_HDRS})


source_group("Sources\\OS\\MessageQueue" FILES ${MQ_SRCS})
source_group("Headers\\OS\\MessageQueue" FILES ${MQ_HDRS})


IF(WIN32)

	file(GLOB PLAT_THREAD_SRCS "OS/Threads/Win32/*.cpp")
	source_group("Sources\\OS\\Threads\\Win32\\" FILES ${PLAT_THREAD_SRCS})

	file(GLOB PLAT_THREAD_HDRS "OS/Threads/Win32/*.h")	
	source_group("Headers\\OS\\Threads\\Win32\\" FILES ${PLAT_THREAD_HDRS})

	file(GLOB PLAT_LOGGER_SRCS "OS/Logger/Win32/*.cpp")
	source_group("Sources\\OS\\Logger\\Win32\\" FILES ${PLAT_LOGGER_SRCS})

	file(GLOB PLAT_LOGGER_HDRS "OS/Logger/Win32/*.h")	
	source_group("Headers\\OS\\Logger\\Win32\\" FILES ${PLAT_LOGGER_HDRS})

	file(GLOB PLAT_TIMER_SRCS "OS/Timer/Win32/*.cpp")
	source_group("Sources\\OS\\Timer\\Win32\\" FILES ${PLAT_TIMER_SRCS})

	file(GLOB PLAT_TIMER_HDRS "OS/Timer/Win32/*.h")	
	source_group("Headers\\OS\\Timer\\Win32\\" FILES ${PLAT_TIMER_HDRS})

	set (ALL_PLAT ${PLAT_THREAD_SRCS} ${PLAT_THREAD_HDRS} ${PLAT_LOGGER_SRCS} ${PLAT_LOGGER_HDRS} ${PLAT_TIMER_SRCS} ${PLAT_TIMER_HDRS})
	add_library (openspy SHARED ${ALL_SRCS} ${ALL_PLAT})
	target_link_libraries(openspy ws2_32.lib ${CURL_LIBRARIES} pugixml rabbitmq::rabbitmq ${JANSSON_LIBRARIES} ${hiredis_LIBRARIES} ${SSL_LIBS})
	add_definitions(-DEVTMGR_USE_SELECT=1)
ELSE() #unix
	
	file(GLOB PLAT_THREAD_SRCS "OS/Threads/PThreads/*.cpp")
	source_group("Sources\\OS\\Threads\\PThreads\\" FILES ${PLAT_THREAD_SRCS})

	file(GLOB PLAT_THREAD_HDRS "OS/Threads/PThreads/*.h")	
	source_group("Headers\\OS\\Threads\\PThreads\\" FILES ${PLAT_THREAD_HDRS})

	file(GLOB PLAT_LOGGER_SRCS "OS/Logger/Unix/*.cpp")
	source_group("Sources\\OS\\Logger\\Unix\\" FILES ${PLAT_LOGGER_SRCS})

	file(GLOB PLAT_LOGGER_HDRS "OS/Logger/Unix/*.h")	
	source_group("Headers\\OS\\Logger\\Unix\\" FILES ${PLAT_LOGGER_HDRS})

	file(GLOB PLAT_TIMER_SRCS "OS/Timer/Unix/*.cpp")
	source_group("Sources\\OS\\Timer\\Unix\\" FILES ${PLAT_TIMER_SRCS})

	file(GLOB PLAT_TIMER_HDRS "OS/Timer/Unix/*.h")	
	source_group("Headers\\OS\\Timer\\Unix\\" FILES ${PLAT_TIMER_HDRS})

	set (ALL_PLAT ${PLAT_THREAD_SRCS} ${PLAT_THREAD_HDRS} ${PLAT_LOGGER_SRCS} ${PLAT_LOGGER_HDRS} ${PLAT_TIMER_SRCS} ${PLAT_TIMER_HDRS})
	add_library (openspy SHARED ${ALL_SRCS} ${ALL_PLAT})
	target_link_libraries(openspy pthread ${CURL_LIBRARIES} z pugixml rabbitmq::rabbitmq ${JANSSON_LIBRARIES} ${hiredis_LIBRARIES} ${SSL_LIBS})
	if(APPLE)
	add_definitions(-DEVTMGR_USE_SELECT=1)
	else()
	add_definitions(-DEVTMGR_USE_EPOLL=1)
	endif()
ENDIF()

INSTALL(TARGETS openspy
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  COMPONENT library
)
install(DIRECTORY "OS" DESTINATION include FILES_MATCHING PATTERN "*.h")

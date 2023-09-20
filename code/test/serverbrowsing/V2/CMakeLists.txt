cmake_minimum_required (VERSION 3.5)

project(serverbrowsing_v2_test)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})


add_executable (test_sbv2_server_message ${ALL_SRCS} test_common.cpp test_server_message.cpp)
add_test(NAME test_sbv2_server_message COMMAND test_sbv2_server_message WORKING_DIRECTORY .)

add_executable (test_sbv2_groups ${ALL_SRCS} test_common.cpp test_groups.cpp)
add_test(NAME test_sbv2_groups COMMAND test_sbv2_groups WORKING_DIRECTORY .)
IF(WIN32)	
	target_link_libraries(test_sbv2_server_message ws2_32.lib openspy.lib)
	target_link_libraries(test_sbv2_groups ws2_32.lib openspy.lib)
ELSE() #unix
	target_link_libraries(test_sbv2_server_message pthread openspy)
	target_link_libraries(test_sbv2_groups pthread openspy)
ENDIF()

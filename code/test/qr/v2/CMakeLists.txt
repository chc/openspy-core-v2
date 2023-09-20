cmake_minimum_required (VERSION 3.5)

project(qr_test)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})


add_executable (test_qrv2_add_server ${ALL_SRCS} common.cpp test_add_server.cpp)
add_test(NAME test_qrv2_add_server COMMAND test_qrv2_add_server WORKING_DIRECTORY .)

IF(WIN32)	
	target_link_libraries(test_qrv2_add_server ws2_32.lib openspy.lib)
ELSE() #unix
	target_link_libraries(test_qrv2_add_server pthread openspy)
ENDIF()
file(GLOB_RECURSE GAME_SOURCES "*.h" "*.cpp")

add_executable(game ${GAME_SOURCES})

target_link_libraries(game 
	common 
	libgame 
	libpoly 
	libsql 
	libthecore 
	liblua
)

if (WIN32)
	target_link_libraries(game ws2_32)
else()
	target_link_libraries(game pthread md)
endif()

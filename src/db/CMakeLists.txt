file(GLOB_RECURSE DB_SOURCES "*.h" "*.cpp")

add_executable(db ${DB_SOURCES})

target_compile_definitions(db PRIVATE GIT_DESCRIBE="${GIT_DESCRIBE_VERSION}")

target_link_libraries(db 
	common
	libgame 
	libpoly 
	libsql 
	libthecore

	# external
	mariadbclient
)

if (WIN32)
	target_link_libraries(db ws2_32)
else()
	if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
		target_link_libraries(db pthread md)
	else()
		target_link_libraries(db pthread)
	endif()
endif()

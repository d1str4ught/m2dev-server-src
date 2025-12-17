file(GLOB_RECURSE DB_SOURCES "*.h" "*.cpp")

add_executable(db ${DB_SOURCES})

target_compile_definitions(db PRIVATE GIT_DESCRIBE="${GIT_DESCRIBE_VERSION}")
target_precompile_headers(db PRIVATE stdafx.h)

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
	target_link_libraries(db pthread md)
endif()

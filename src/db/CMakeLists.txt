file(GLOB_RECURSE DB_SOURCES "*.h" "*.cpp")

add_executable(db ${DB_SOURCES})

target_link_libraries(db 
	common
	libgame 
	libpoly 
	libsql 
	libthecore
	libredis
)

if (WIN32)
	target_link_libraries(db ws2_32)
else()
	target_link_libraries(db pthread md)
endif()

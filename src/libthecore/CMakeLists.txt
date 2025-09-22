file(GLOB_RECURSE LIBTHECORE_SOURCES "*.h" "*.cpp")

add_library(libthecore STATIC ${LIBTHECORE_SOURCES})

target_link_libraries(libthecore 
	common
	spdlog
)

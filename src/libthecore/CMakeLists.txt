file(GLOB_RECURSE LIBTHECORE_SOURCES "*.h" "*.cpp")

add_library(libthecore STATIC ${LIBTHECORE_SOURCES})
target_precompile_headers(libthecore PRIVATE stdafx.h)

target_link_libraries(libthecore 
	common
	spdlog
)

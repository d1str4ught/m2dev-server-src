file(GLOB_RECURSE LIBTHECORE_SOURCES "*.h" "*.cpp")

add_library(libthecore STATIC ${LIBTHECORE_SOURCES})

target_link_libraries(libthecore 
	common
	#spdlog
)

# Conditional linking for Spdlog
if (USE_SIL AND NOT WIN32)
	# Link to the system-found spdlog library (from root CMakeLists.txt)
	target_link_libraries(libthecore ${SPDLOG_LIB})
else()
	# Link to the vendor target
	target_link_libraries(libthecore spdlog)
endif()

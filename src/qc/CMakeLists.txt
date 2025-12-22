file(GLOB_RECURSE QC_SOURCES "*.h" "*.cpp")

add_executable(qc ${QC_SOURCES})

target_link_libraries(qc 
	liblua 
)

if (WIN32)
    target_link_libraries(qc ws2_32)
else()
    target_link_libraries(qc pthread)
    if (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        target_link_libraries(qc md)
    endif()
endif()

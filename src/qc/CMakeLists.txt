file(GLOB_RECURSE QC_SOURCES "*.h" "*.cpp")

add_executable(qc ${QC_SOURCES})

target_link_libraries(qc 
	liblua 
)

if (WIN32)
	target_link_libraries(db ws2_32)
else()
	target_link_libraries(db pthread md)
endif()

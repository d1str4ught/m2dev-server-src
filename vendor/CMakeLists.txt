add_subdirectory(mariadb-connector-c-3.4.5 EXCLUDE_FROM_ALL)

if (WIN32)
	# set_target_properties(vstl PROPERTIES FOLDER vendor)
endif()
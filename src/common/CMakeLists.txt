file(GLOB_RECURSE COMMON_HEADERS "*.h" "*.hpp")

add_library(common INTERFACE ${COMMON_HEADERS})
target_include_directories(common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

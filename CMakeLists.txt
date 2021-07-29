# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("vehmods-generator")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/win64/${CMAKE_BUILD_TYPE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/win64/${CMAKE_BUILD_TYPE})

# Include sub-projects.
file(GLOB_RECURSE PROJECT_SOURCE_FILES "./src/*.h" "./src/*.hpp" "./src/*.cpp" "./src/*.c")

include_directories(
	"${PROJECT_SOURCE_DIR}/src",
	"${PROJECT_SOURCE_DIR}/vendors/zlib/include"
)

message("Output directory:")
message(${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
message(${CMAKE_SOURCE_DIR}/scripts/)

add_executable(demo ${PROJECT_SOURCE_FILES})
target_link_libraries(demo ${PROJECT_SOURCE_DIR}/vendors/zlib/lib/zlib.lib)
add_custom_command(TARGET demo PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/scripts/ ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})

add_custom_command(TARGET demo PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/resources/ ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/output_files/)
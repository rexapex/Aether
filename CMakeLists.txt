# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.16...3.19)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("ether-language" C)

# Add preprocessor definitions
if(${CMAKE_SYSTEM_NAME} STREQUAL Windows OR ${CMAKE_SYSTEM_NAME} STREQUAL MSYS)
    add_definitions(-DSYSTEM_WINDOWS)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
    add_definitions(-DSYSTEM_LINUX)
endif()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(build-dependencies)

# Build internal libraries
EL_BUILD_LIB_COMPILER()
EL_BUILD_LIB_FILE_SYSTEM()

# Build apps
add_subdirectory(apps/ether-c)

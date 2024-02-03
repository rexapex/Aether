# CMakeList.txt : CMake project for aether-language, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable(aether-c "main.c")

if(CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET aether-c PROPERTY C_STANDARD 17)
endif()

target_compile_features(aether-c PRIVATE c_std_17)

include(include-dependencies)
EL_INCLUDE_LIBS(aether-c)

include(link-dependencies)
EL_LINK_LIB_COMPILER(aether-c)
EL_LINK_LIB_FILE_SYSTEM(aether-c)

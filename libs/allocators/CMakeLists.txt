# CMakeList.txt : CMake project for aether-language, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_library(el_lib_allocators "fmalloc.h" "linear-allocator.c" "linear-allocator.h")

if(CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET el_lib_allocators PROPERTY C_STANDARD 17)
endif()

target_compile_features(el_lib_allocators PRIVATE c_std_17)

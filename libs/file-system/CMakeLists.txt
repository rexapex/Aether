# CMakeList.txt : CMake project for ether-language, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_library(el_lib_file_system "file-system.h" "file-system.c" "path.h")

if(CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET el_lib_file_system PROPERTY C_STANDARD 17)
endif()

target_compile_features(el_lib_file_system PRIVATE c_std_17)

include(include-dependencies)
EL_INCLUDE_LIBS(el_lib_file_system)

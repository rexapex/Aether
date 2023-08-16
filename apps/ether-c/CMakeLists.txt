# CMakeList.txt : CMake project for ether-language, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable(ether-c "main.c")

if(CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ether-c PROPERTY C_STANDARD 17)
endif()

target_compile_features(ether-c PRIVATE c_std_17)

include(include-dependencies)
EL_INCLUDE_LIBS(ether-c)

include(link-dependencies)
EL_LINK_LIB_COMPILER(ether-c)
EL_LINK_LIB_FILE_SYSTEM(ether-c)

# CMakeList.txt : CMake project for ether-language, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_library(el_lib_compiler "lexing/lexer.h" "lexing/lexer.c" "lexing/token-stream.h" "lexing/token-stream.c" "syntax-parsing/parser.c" "syntax-parsing/parser.h" "syntax-parsing/ast.h" "syntax-parsing/ast.c")

if(CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET el_lib_compiler PROPERTY C_STANDARD 17)
endif()

target_compile_features(el_lib_compiler PRIVATE c_std_17)

include(include-dependencies)

# Include dependencies
EL_INCLUDE_LIBS(el_lib_compiler)

include(link-dependencies)

# Link dependencies
EL_LINK_LIB_FILE_SYSTEM(el_lib_compiler)

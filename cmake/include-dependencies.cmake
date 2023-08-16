macro(el_include_libs t)
	target_include_directories(${t} PRIVATE "${PROJECT_SOURCE_DIR}/libs" SYSTEM INTERFACE "${PROJECT_SOURCE_DIR}/libs")
endmacro()

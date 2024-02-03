macro(el_build_lib_allocators)
	add_subdirectory("${PROJECT_SOURCE_DIR}/libs/allocators" "${PROJECT_BINARY_DIR}/libs/allocators")
endmacro()

macro(el_build_lib_compiler)
	add_subdirectory("${PROJECT_SOURCE_DIR}/libs/compiler" "${PROJECT_BINARY_DIR}/libs/compiler")
endmacro()

macro(el_build_lib_containers)
	add_subdirectory("${PROJECT_SOURCE_DIR}/libs/containers" "${PROJECT_BINARY_DIR}/libs/containers")
endmacro()

macro(el_build_lib_file_system)
	add_subdirectory("${PROJECT_SOURCE_DIR}/libs/file-system" "${PROJECT_BINARY_DIR}/libs/file-system")
endmacro()

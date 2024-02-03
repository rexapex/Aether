macro(el_link_lib_allocators t)
	target_link_libraries(${t} PRIVATE el_lib_allocators)
endmacro()

macro(el_link_lib_compiler t)
	target_link_libraries(${t} PRIVATE el_lib_compiler)
endmacro()

macro(el_link_lib_containers t)
	target_link_libraries(${t} PRIVATE el_lib_containers)
endmacro()

macro(el_link_lib_file_system t)
	target_link_libraries(${t} PRIVATE el_lib_file_system)
endmacro()

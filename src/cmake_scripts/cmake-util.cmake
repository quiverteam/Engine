#================================================#
# Some utils for cmake
#================================================#
function(find_files dirs files)
	set(xfiles "")
	foreach(dir IN ${dirs})
		file(GLOB ffiles ${dir})
		list(APPEND xfiles ${ffiles})
	endforeach()
	set(${files} ${xfiles} PARENT_SCOPE)
endfunction()
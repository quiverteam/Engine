project "LinearMath"
	
kind "StaticLib"

language "C++"

includedirs {
	"..",
}

files {
	"**.cpp",
	"**.h"
}
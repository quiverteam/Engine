project "BulletCollision"

language "C++"
	
kind "StaticLib"

includedirs {
	"..",
}

files {
	"**.cpp",
	"**.h"
}
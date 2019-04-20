project "BulletSoftBody"
	
kind "StaticLib"

language "C++"

links { "BulletCollision" }

includedirs {
	"..",
}

files {
	"**.cpp",
	"**.h"
}
project "BulletDynamics"
	
kind "StaticLib"

language "C++"

includedirs {
	"..",
}

files {
	"**.cpp",
	"**.h"
}
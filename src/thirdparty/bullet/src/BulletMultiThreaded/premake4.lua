project "BulletMultiThreaded"
	
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

excludes {
	"GpuSoftBodySolvers/**"
}
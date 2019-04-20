solution "bullet"

if _ACTION == "vs2010" or _ACTION == "vs2008" then
	-- Enable multi-processor compilation
	buildoptions { "/MP"  }
end

act = ""

if _ACTION then
	act = _ACTION
end

--[[---------------------------------
-- Main configuration properities
-----------------------------------]]
configurations { "Release", "Debug" }

configuration "Release"
	defines { "RELEASE=1", "NDEBUG=1", "_RELEASE=1" }
	if os.is( "linux" ) then
		defines { "_LINUX=1", "__linux__=1", "LINUX=1" }
	end
	
	-- TODO: When supported, add NoIncrementalLink flag
	flags { "OptimizeSpeed", "EnableSSE", "StaticRuntime", "NoMinimalRebuild", "FloatFast" }
	targetdir( "../../build/lib/" .. os.get() .. "/release" )
	
configuration "Debug"
	defines { "_DEBUG=1" }
	if os.is("linux") then
		defines { "_LINUX=1", "__linux__=1", "LINUX=1" }
	end
	
	flags { "Symbols", "StaticRuntime", "NoMinimalRebuild", "FloatFast" }
	targetdir("../../build/lib/" .. os.get() .. "/debug")

configuration {}

-- Only support 32 bit builds
configuration { "linux", "gmake" }
	buildoptions { "-fPIC", "-m32", "-msse2" }
	linkoptions { "-m32", "-msse2" }

configuration { "linux", "gmake", "Debug" }
	buildoptions { "-ggdb" }
	linkoptions { "-ggdb" }
	
configuration {}

-- Set generated project location
location( "./" .. act )

--[[--------------
-- Projects
----------------]]
include "../src"

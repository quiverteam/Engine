--[[
configuration "Release"
	targetdir("../../build/lib/" .. os.get() .. "/release")

configuration "Debug"
	targetdir("../../build/lib/" .. os.get() .. "/debug")
]]
	
configuration {}

include "BulletCollision"
include "BulletDynamics"
include "BulletMultiThreaded"
include "BulletSoftBody"
include "LinearMath"

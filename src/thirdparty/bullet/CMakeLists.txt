#
# We will fix up Bullet definitions here
#
if(USE_SSE)
    add_definitions(-DBT_USE_SSE=1)
endif(USE_SSE)

add_definitions(-DMBCS)

add_subdirectory(src/BulletCollision)
add_subdirectory(src/BulletDynamics)
add_subdirectory(src/BulletMultiThreaded)
add_subdirectory(src/BulletSoftBody)
add_subdirectory(src/LinearMath)
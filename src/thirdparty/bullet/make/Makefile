BULLET_SRC_DIR = ../src

BULLET_PROJS = BulletCollision BulletDynamics BulletMultiThreaded BulletSoftBody LinearMath
BULLET_OUT_DIR = ../../build/lib/linux/Release
BULLET_OBJ_DIR = ../../build/obj/linux/Release

# ------------------------ System Commands ------------------------
CC = /usr/bin/g++
LINK = /usr/bin/g++

# ------------------------ Compiler Settings ------------------------
INCLUDES = 
DEFINES = -DLINUX -D_LINUX -D__linux__ -D__LINUX__ -DNDEBUG -DRELEASE -D_RELEASE

LIBS = -lm

CFLAGS = $(INCLUDES) $(DEFINES) -msse2 -m32 -march=i386
LFLAGS = $(LIBS) -msse2 -m32 -march=i386

# ------------------------ MAKE COMMANDS ------------------------
all: $(BULLET_PROJS)

# ------------------------ BulletCollision ------------------------
BULLETCOLLISION_DIR = $(BULLET_SRC_DIR)/BulletCollision

BULLETCOLLISION_SRCS = 	$(wildcard $(BULLETCOLLISION_DIR)/BroadphaseCollision/*.cpp)
BULLETCOLLISION_SRCS += $(wildcard $(BULLETCOLLISION_DIR)/CollisionDispatch/*.cpp)
BULLETCOLLISION_SRCS += $(wildcard $(BULLETCOLLISION_DIR)/CollisionShapes/*.cpp)
BULLETCOLLISION_SRCS += $(wildcard $(BULLETCOLLISION_DIR)/Gimpact/*.cpp)
BULLETCOLLISION_SRCS += $(wildcard $(BULLETCOLLISION_DIR)/NarrowPhaseCollision/*.cpp)

BULLETCOLLISION_OBJS := $(BULLETCOLLISION_SRCS:.cpp=.o)
BULLETCOLLISION_OBJS := $(addprefix $(BULLET_OBJ_DIR)/BulletCollision/, $(notdir $(BULLETCOLLISION_OBJS)))

BulletCollision: $(BULLETCOLLISION_OBJS)
	@echo " + Linking BulletCollision..."
	
$(dir $(BULLETCOLLISION_OBJS))/%.o: $(dir $(BULLETCOLLISION_SRCS))/%.cpp
	@echo " + Compiling '$<'"

# ------------------------ BulletDynamics ------------------------
BULLETDYNAMICS_DIR = $(BULLET_SRC_DIR)/BulletDynamics

BULLETDYNAMICS_SRCS =	$(wildcard $(BULLETDYNAMICS_DIR)/Character/*.cpp)
BULLETDYNAMICS_SRCS +=	$(wildcard $(BULLETDYNAMICS_DIR)/ConstraintSolver/*.cpp)
BULLETDYNAMICS_SRCS +=	$(wildcard $(BULLETDYNAMICS_DIR)/Dynamics/*.cpp)
BULLETDYNAMICS_SRCS +=	$(wildcard $(BULLETDYNAMICS_DIR)/Vehicle/*.cpp)

BULLETDYNAMICS_OBJS := $(BULLETDYNAMICS_SRCS:.cpp=.0)
BULLETDYNAMICS_OBJS := $(subst $(BULLET_SRC_DIR), $(BULLET_OUT_DIR), $(BULLETDYNAMICS_OBJS))

BulletDynamics:
	@echo "BulletDynamics"

# ------------------------ BulletMultiThreaded ------------------------
BULLETMULTITHREADED_DIR = $(BULLET_SRC_DIR)/BulletMultiThreaded

BULLETMULTITHREADED_SRCS =	$(wildcard $(BULLETMULTITHREADED_DIR)/*.cpp)
BULLETMULTITHREADED_SRCS +=	$(wildcard $(BULLETMULTITHREADED_DIR)/SpuNarrowPhaseCollisionTask/*.cpp)

BULLETMULTITHREADED_OBJS := $(BULLETMULTITHREADED_SRCS:.cpp=.0)
BULLETMULTITHREADED_OBJS := $(subst $(BULLET_SRC_DIR), $(BULLET_OUT_DIR), $(BULLETMULTITHREADED_OBJS))

BulletMultiThreaded:
	@echo "BulletMultiThreaded"

# ------------------------ BulletSoftBody ------------------------
BULLETSOFTBODY_DIR = $(BULLET_SRC_DIR)/BulletSoftBody

BULLETSOFTBODY_SRCS =	$(wildcard $(BULLETSOFTBODY_DIR)/*.cpp)

BULLETSOFTBODY_OBJS := $(BULLETSOFTBODY_SRCS:.cpp=.o)
BULLETSOFTBODY_OBJS := $(subst $(BULLET_SRC_DIR), $(BULLET_OUT_DIR), $(BULLETSOFTBODY_OBJS))

BulletSoftBody:
	@echo "BulletSoftBody"

# ------------------------ LinearMath ------------------------
LINEARMATH_DIR = $(BULLET_SRC_DIR)/LinearMath

LINEARMATH_SRCS =	$(wildcard $(LINEARMATH_DIR)/*.cpp)

LINEARMATH_OBJS := $(LINEARMATH_SRCS:.cpp=.o)
LINEARMATH_OBJS := $(subst $(BULLET_SRC_DIR), $(BULLET_OUT_DIR), $(LINEARMATH_OBJS))

LinearMath:
	@echo "LinearMath"
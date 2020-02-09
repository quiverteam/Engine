#!/bin/bash

# Directory of the top source tree
export QUIVER_DIR="~/Desktop/Projects/Quiver/"
export QUIVER_GAME="$QUIVER_DIR/game"

# Flags to pass to make
MAKE_FLAGS="-j8"

# Projects we want to build
PROJECTS="launcher launcher_main tier0 tier1 vstdlib tier2 tier3 filesystem_stdio"

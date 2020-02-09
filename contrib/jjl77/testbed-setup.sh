#!/bin/bash

# simple setup script for the testbed
TOP=$(dirname $0)

source "$TOP/settings.sh"

if [ -d "$TESTBED_DIR" ]; then
	echo "Testbed directory already exists."
	exit 1
fi

mkdir "$TESTBED_DIR"

# Copy the game dir to the testbed dir
/bin/cp -rf "$QUIVER_GAME" "$TESTBED_DIR"
mkdir -p "$TESTBED_DIR/bin/"
/bin/cp -rf "$SDK2013_MP_DIR/bin" "$TESTBED_DIR/bin/linux32"
/bin/cp -rf "$SDK2013_MP_DIR/bin" "$TESTBED_DIR"

# Link HL2 dirs into it
ln -s "$HL2_DIR" "$TESTBED_DIR/hl2"
ln -s "$EP1_DIR" "$TESTBED_DIR/episodic"
ln -s "$EP2_DIR" "$TESTBED_DIR/ep2"


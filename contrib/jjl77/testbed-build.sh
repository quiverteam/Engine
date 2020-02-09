#!/bin/bash

TOP=$(dirname $0)

set -e

# Source muh settings
source "$TOP/settings.sh"
source "$TOP/build-flags.sh"
source "$TOP/build-linux32.sh"

# I have cp aliased so I need to clear the flags
CP="/bin/cp -v"

DEST_DIRS="bin bin/linux32"

eval pushd "$QUIVER_DIR/src"

if [ ! -d build ]; then
	mkdir build 
	cd build
	cmake ../ $CMAKE_FLAGS
	
	if [ $? -ne 0 ]; then
		echo "Configure failed."
		popd 
		exit 1
	fi
else
	cd build
fi

echo "Building..."

make $PROJECTS $MAKE_FLAGS

# If the build failed, we should remake the project with only one job so it's easier to read
# Clear the screen too 
if [ $? -ne 0 ]; then
	clear 
	make $PROJECTS -j1
	exit 1
fi

echo "Copying files..."

# Now, we should copy the files to the required directories
for project in $PROJECTS; do
	for dir in $DEST_DIRS; do
		# Look for all the libs and stuff
		[ -f "$project/$project" ] && $CP "$project/$project" "$TESTBED_DIR/$dir/"
		[ -f "$project/lib$project.so" ] && $CP "$project/lib$project.so" "$TESTBED_DIR/$dir"
		[ -f "$project/$project.so" ] && $CP "$project/$project.so" "$TESTBED_DIR/$dir"
	done
done
# Filesystem is a special cookie -_- and I'm fucking lazy so let's not try to make this extensible 
$CP -f "filesystem/libfilesystem_stdio.so" "$TESTBED_DIR/$dir/filesystem_stdio.so"

popd
echo "Installing launcher script"

$CP "$TOP/launch.sh" "$TESTBED_DIR/launch.sh"
$CP "$TOP/settings.sh" "$TESTBED_DIR/settings.sh"
chmod +rwx "$TESTBED_DIR/launch.sh"
chmod +rwx "$TESTBED_DIR/settings.sh"

echo "Done!"

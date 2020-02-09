#!/bin/bash
TOP=$(dirname $0)
set -e
source "$TOP/settings.sh"

export LD_LIBRARY_PATH="$TESTBED_DIR/bin/linux32:$TESTBED_DIR/bin/:$LD_LIBRARY_PATH"

[ ! -z $1 ] && shift;
cd "$TOP"
echo "Launching game."
if [ ! -z "$GAME_DEBUGGER" ]; then
	"$GAME_DEBUGGER" "$TESTBED_DIR/bin/linux32/launcher_main" $@
else
	"$TESTBED_DIR/bin/linux32/launcher_main" $@
fi 

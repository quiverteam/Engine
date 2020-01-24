#!/bin/bash

BATCHHMODE=0;

# Simple setup script for Linux
# Options: --batch (non-interactive, possibly dangerous)
# -d <dir> set hl2 base dir
if [ $EUID -ne 0 ]; then
	echo -e "\e[91mScript should be run as root, but it might work without it\e[39m"
	#exit 1 
fi


for arg in "$@"; do
	case $arg in 
	-h)
		echo -e "USAGE: $0 [-d hl2_dir] [-b]"
		exit 0
		;;
	-d)
		shift;
		HL2_DIR="$1"
		echo -e "HL2_DIR=$HL2_DIR"
		;;
	-b|--batch)
		echo "FUCK"
		BATCHMODE=1
		shift;
		;;
	esac
	break
done

if [[ ! "$HL2_DIR" ]]; then
	HL2_DIR="~/.steam/steam/steamapps/common/Half-Life 2";
	echo -e "\e[93mThe environment variable HL2_DIR is not set, falling back to $HL2_DIR"
	echo -e "You can also specify your HL2 install directory with the option \"-d path/to/hl2/dir\"\e[39m"
fi

[ ! -d "$HL2_DIR" ] && echo -e "\e[91mThe specified HL2 directory does not exist.\e[39m" && exit 1

# Check for hl2 and the episodes
[ ! -d "$HL2_DIR/hl2" ] && echo -e "\e[91mHL2 subdirectory not found in \"$HL2_DIR\"\e[39m" && exit 1

if [ ! -d "$HL2_DIR/episodic" ]; then
	echo -e "\e[93mepisodic subdirectory not found in \"$HL2_DIR\", skipping it...\e[39m"
else
	echo -e "\e[92mFound episodic at $HL2_DIR/episodic\e[39m"
	LINK_EPISODIC="$HL2_DIR/episodic";
fi

if [ ! -d "$HL2_DIR/ep2" ]; then
	echo -e "\e[93mep2 subdirectory not found in \"$HL2_DIR\", skipping it...\e[39m"
else
	echo -e "\e[92mFound ep2 at $HL2_DIR/ep2\e[39m";
	LINK_EP2="$HL2_DIR/ep2";
fi

[ -d "$(pwd)/hl2" ] && echo "Link $(pwd)hl2 already exists!" && exit 1
[ -d "$(pwd)/ep2" ] && echo "Link $(pwd)ep2 already exists!" && exit 1
[ -d "$(pwd)/episodic" ] && echo "Link $(pwd)episodic already exists!" && exit 1

echo -e "-----------------------------------------------------"
echo -e "Actions:"
echo -e "\tCreate link: $(pwd)/hl2"
[ ! -z "$LINK_EPSIDOIC" ] && echo -e "\tCreate link: $(pwd)/episodic -> $HL2_DIR/episodic"
[ ! -z "$LINK_EP2" ] && echo -e "\tCreate link: $(pwd)/ep2 -> $HL2_DIR/ep2"
echo -e "-----------------------------------------------------"
printf "Procceed? (y/n) "

[[ $BATCHMODE -eq 0 ]] && read YN || YN="y"

if [[ $YN == "y" || $YN == "Y" ]]; then
	ln -s "$HL2_DIR/hl2" "$(pwd)/hl2"
	[ $? -ne 0 ] && echo -e "\e[91mFailed to create link $(pwd)/hl2\e[39m" && exit 1
	
	if [ ! -z "$LINK_EPISODIC" ]; then 
		ln -s "$LINK_EPISODIC" "$(pwd)/episodic"
		[ $? -ne 0 ] && echo -e "\e[91mFailed to create link $(pwd)/episodic\e[39m" && exit 1
	fi
	
	if [ ! -z "$LINK_EP2" ]; then
		ln -s "$LINK_EP2" "$(pwd)/ep2"
		[ $? -ne 0 ] && echo -e "\e[91mFailed to create link $(pwd)/ep2\e[39m" && exit 1
	fi
else
	echo "Aborted."
	exit 0
fi

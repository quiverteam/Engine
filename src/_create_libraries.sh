#!/bin/sh
python devtools/qpc/qpc.py -b "_qpc_scripts/_default.qpc_base" -a libraries -g makefile -g compile_commands -m HL2 HL2MP EPISODIC -mf libraries

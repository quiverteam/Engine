@echo off

py devtools\qpc\qpc.py -b "_qpc_scripts/_default.qpc_base" -a everything -r libraries -g visual_studio -m HL2 HL2MP EPISODIC -mf binaries

pause

@echo off

py devtools\qpc\qpc.py -b "_qpc_scripts/_default.qpc_base" -a game -r physics -g visual_studio -m HL2 HL2MP EPISODIC -mf engine

pause

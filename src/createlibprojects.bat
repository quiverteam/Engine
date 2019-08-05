@echo off

set pyqpc=py devtools\pyqpc\PyQPC.py /basefile "../../vpc_scripts/default.vgc"
%pyqpc% /hl2 /hl2mp /episodic +libraries /vs2019 /mksln libraries

pause
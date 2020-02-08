@echo off

set pyqpc=py devtools\pyqpc\PyQPC.py /basefile "../../vpc_scripts/default.vgc"
%pyqpc% /hl2 /hl2mp /episodic +core /vs2019 /mksln pyvpc_core

pause
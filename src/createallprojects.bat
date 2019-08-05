@echo off

set pyqpc=py devtools\pyqpc\PyQPC.py /basefile "../../vpc_scripts/default.vgc"
%pyqpc% /hl2 /hl2mp /episodic +everything /vs2019 /mksln everything

pause
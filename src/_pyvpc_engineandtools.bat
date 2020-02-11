@echo off

set pyqpc=py devtools\pyqpc\PyQPC.py /basefile "../../vpc_scripts/default.vgc"
set "disabled_projects=-bsppack -bzip2 -ep2_deathmap -vtex_dll -vtex_launcher -normal2ssbump -height2normal -height2ssbump"
%pyqpc% /hl2 /hl2mp /episodic +game +utils +tools %disabled_projects% /vs2019 /mksln pyvpc_engine_and_tools

pause

@echo off
echo Patching NetStorm Version in %NSINSTALLROOT%
echo MAKE SURE THAT NO NETSTORM PROGRAMS ARE RUNNING
pause

rem Patch Batch File Generated 01/23/98

if '%NSINSTALLROOT%'=='' echo THIS BATCH FILE IS NOT MEANT TO BE RUN STANDALONE
if '%NSINSTALLROOT%'=='' echo RUN UPDATE37.EXE INSTEAD
if '%NSINSTALLROOT%'=='' exit -1

echo NSINSTALLROOT='%NSINSTALLROOT%'
echo NSCDROOT='%NSCDROOT%'

attrib -r "%NSINSTALLROOT%\NetStorm.exe"
copy "%NSCDROOT%\NetStorm.exe" "%NSINSTALLROOT%\NetStorm.exe"
zpatch -p "%NSINSTALLROOT%\NetStorm.exe" 1 > "%NSINSTALLROOT%\NetStorm.exe.patch"
del "%NSINSTALLROOT%\NetStorm.exe"
move "%NSINSTALLROOT%\NetStorm.exe.patch" "%NSINSTALLROOT%\NetStorm.exe"

attrib -r "%NSINSTALLROOT%\netstorm.tarc"
copy "%NSCDROOT%\netstorm.tarc" "%NSINSTALLROOT%\netstorm.tarc"
zpatch -p "%NSINSTALLROOT%\netstorm.tarc" 2 > "%NSINSTALLROOT%\netstorm.tarc.patch"
del "%NSINSTALLROOT%\netstorm.tarc"
move "%NSINSTALLROOT%\netstorm.tarc.patch" "%NSINSTALLROOT%\netstorm.tarc"

echo 10.62 > "%NSINSTALLROOT%\netstorm.ver" 

echo Patch complete

exit 0
:end
@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++20 /utf-8 /EHsc /Zs /I"H:\Workspace\git\NetstormReborn\extracted\cpp-resume-1265-candidate\cpppj\src" /I"H:\Workspace\git\NetstormReborn\cpppj\src" "H:\Workspace\git\NetstormReborn\extracted\cpp-resume-1265-candidate\cpppj\src\client\NativeFortSnapshot.cpp" "H:\Workspace\git\NetstormReborn\extracted\cpp-resume-1265-candidate\cpppj\src\client\NativeFortPreparation.cpp" >"H:\Workspace\git\NetstormReborn\extracted\cpp-resume-1265-candidate\candidate-syntax-final.log" 2>&1
set "_netstormSnapshotSyntaxExit=%errorlevel%"
echo %_netstormSnapshotSyntaxExit% >"H:\Workspace\git\NetstormReborn\extracted\cpp-resume-1265-candidate\candidate-syntax-final-exit.txt"
exit /b %_netstormSnapshotSyntaxExit%

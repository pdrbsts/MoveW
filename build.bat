@echo off
echo Building Limpa.exe with MSVC...

REM Set up the environment for MSVC (adjust the path if needed)
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat"

REM Compile the resource script
rc /fo version.res version.rc

REM Compile the program for minimum size with static runtime linking
cl main.cpp version.res /O1 /GL /MT /EHsc /Fe:movew.exe /link /LTCG /OPT:REF /OPT:ICF

mpress -s movew.exe

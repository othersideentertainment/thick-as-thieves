REM https://stackoverflow.com/questions/734598/how-do-i-make-a-batch-file-terminate-upon-encountering-an-error

REM call RunTests.bat
REM if %errorlevel% neq 0 exit /b %errorlevel%

set UE4EditorCmdExe=%1
if "%UE4EditorCmdExe%"=="" (
	set UE4EditorCmdExe=..\..\..\Engine\Binaries\Win64\UnrealEditor-Cmd.exe
	echo falling back
)

"%UE4EditorCmdExe%" TAT\TAT.uproject -unattended -buildmachine -stdout -nosplash -run=OSEMapCheck
if %errorlevel% neq 0 exit /b %errorlevel%

"%UE4EditorCmdExe%" TAT\TAT.uproject -unattended -buildmachine -stdout -nosplash  -run=OSEDataValidation
if %errorlevel% neq 0 exit /b %errorlevel%

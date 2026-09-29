SET AUTOMATION_TEST_MAP=Launcher

..\..\..\Engine\Binaries\Win64\UnrealEditor-Cmd.exe TAT\TAT.uproject ^
  %AUTOMATION_TEST_MAP% -unattended -buildmachine -NullRHI -NoSound -ForceStandalone ^
  -ExecCmds="Automation RunTests Project.+TAT.+OSE.; Quit" -nosplash -stdout -nopause -nocontentbrowser

if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
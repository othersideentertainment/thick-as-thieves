@echo OFF

REM ======================================================
REM  Install Otherside Maya Tools
REM ======================================================


REM ======================================================
REM  Find install location
REM  
REM  Note: Assumes this file is being run from a 
REM  folder one level deep in our package
REM ======================================================


REM set our cd to this bat file's location.
pushd %~dp0
set launchpath=%~dp0

REM go up one level
pushd..

REM save path
set "pth=%cd%"

REM convert to forward slashes
set "pth=%pth:\=/%"

REM get version
set last=%pth%
for %%g in ("%last%") do set last=%%~nxg
echo %last%

REM ======================================================
REM  Invoke Admin Permissions
REM ======================================================

:: BatchGotAdmin
:-------------------------------------
REM  --> Check for permissions
    IF "%PROCESSOR_ARCHITECTURE%" EQU "amd64" (
>nul 2>&1 "%SYSTEMROOT%\SysWOW64\cacls.exe" "%SYSTEMROOT%\SysWOW64\config\system"
) ELSE (
>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"
)

REM --> If error flag set, we do not have admin.
if '%errorlevel%' NEQ '0' (
    echo Requesting administrative privileges...
    goto UACPrompt
) else ( goto gotAdmin )

:UACPrompt
    echo Set UAC = CreateObject^("Shell.Application"^) > "%temp%\getadmin.vbs"
    set params= %*
    echo UAC.ShellExecute "cmd.exe", "/c ""%~f0"" %params:"=""%", "", "runas", 1 >> "%temp%\getadmin.vbs"

    "%temp%\getadmin.vbs"
    del "%temp%\getadmin.vbs"
    exit /B

:gotAdmin
    pushd "%CD%"
    CD /D "%~dp0"
:-------------------------------------- 


REM ======================================================
REM  Find Maya install locations
REM ======================================================

setlocal enabledelayedexpansion
set count=0
set mayaversion=Maya%last%
REM Find all drives
REM for /f "skip=1" %%x in ('wmic logicaldisk get caption ') do (  *WMIC is deprecated in win 11*
for %%x in (A B C D E F G H I J K L M N O P Q R S T U V W X Y Z) do (
	if exist "%%x:\\" (
		REM Test for Autodesk directory
		set autodesk="%%x:\Program Files\Autodesk\"
		if exist !autodesk! (
			cd /d !autodesk!
			REM Find all maya.exes		
			echo mayaversion is %mayaversion%
			for /f "delims=*" %%y in ('dir /a:d /b %mayaversion%*') do (
				set /a count=count+1
				REM get a nice path enclosed in quotes
				set maya=!autodesk!%%y
				set maya=!maya:"=!
				set choice[!count!]=!maya!
			)
		)
	)
)

REM ======================================================
REM  Write .pth to Maya install location
REM ======================================================

REM === No Maya installs found ===

if %count% == 0 (
    echo.
    echo | set /p=[91mMaya install not found^^! Please create .pth manually. Aborting^^![0m
    echo.
    echo.
    pause
    exit
) 

REM === One Maya install found ===

REM assume single location for promptless install
set version=!choice[1]!
for %%g in ("%version%") do set version=%%~nxg
set version=%version:~4%

REM Maya 2022 supported multiple versions of python, and main python folders were suffixed with version number
if %version% EQU 2022 (
    set targetFile=!choice[1]!\Python37\Lib\site-packages\otherside.pth
) else (
    set targetFile=!choice[1]!\Python\Lib\site-packages\otherside.pth
)
set targetFile=%targetFile:"=%
set targetFile="%targetFile%"

if %count% == 1 (
    REM make file writable just in case
    if exist %targetFile% (
        attrib -R %targetFile%
        )

    echo Installing to %targetFile%
    echo #otherside package > %targetFile%
REM    if %version% EQU 2022 (
REM        echo %pth%/python37/ >> %targetFile%
REM    ) else (
REM        echo %pth%/python27/ >> %targetFile%
REM    )
    echo %pth%/python310/ >> %targetFile%
    if exist %targetFile% (
        echo.
        echo | set /p=[92mSuccess^^! You should now see an Otherside menu when you start Maya^^![0m
        echo.
    ) else (
        echo.
        echo | set /p=[91mError: otherside.pth not created^^![0m
        echo.
    )
    echo.
    pause
    exit
)


REM === Multiple Maya installs found ===

REM PROMPT USER
echo.
echo | set /p=[93mSelect a Maya version for Otherside install [1-%count%]:[0m
echo.
echo.

REM Print list of choices
for /l %%x in (1,1,!count!) do (
     echo %%x] !choice[%%x]!
)
echo.

REM Retrieve User input
set /p select=? 
echo.

set version=!choice[%select%]!
for %%g in ("%version%") do set version=%%~nxg
set version=%version:~4%

REM Maya 2022 supported multiple versions of python, and main python folders were suffixed with version number
if %version% EQU 2022 (
    set targetFile=!choice[%select%]!\Python37\Lib\site-packages\otherside.pth
) else (
    set targetFile=!choice[%select%]!\Python\Lib\site-packages\otherside.pth
)
set targetFile=%targetFile:"=%
set targetFile="%targetFile%"

REM Install to selected maya
if exist !choice[%select%]! (
    REM make file writable just in case
    if exist %targetFile% (
        attrib -R %targetFile%
        )

    REM write file
    echo Installing to %targetFile%
    echo #otherside package > %targetFile%
REM    if %version% GEQ 2022 (
REM        echo %pth%/python37/ >> %targetFile%
REM    ) else (
REM        echo %pth%/python27/ >> %targetFile%
REM    )
	echo %pth%/python310/ >> %targetFile%
    REM check file
    if exist %targetFile% (
        echo.
        echo | set /p=[92mSuccess^^! You should now see an Otherside menu when you start Maya^^![0m
        echo.
    ) else (
        echo.
        echo | set /p=[91mError: otherside.pth not created^^![0m
	echo.
    )
    echo.
    pause
    exit
) else (
    echo.
    echo | set /p=[91m!choice[%select%]! not found^^! Please create .pth manually. Aborting^^![0m
    echo.
    pause
    exit
)
@echo off
mkdir export
xcopy /y /f keypad_host.exe export\
xcopy /y /f wmicom3.dll export\
xcopy /y /f nircmd.exe export\
xcopy /y /f toast.exe export\

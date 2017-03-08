@echo off
rd /s /q export
mkdir export
xcopy /y /f keypad_host.exe export\
xcopy /y /f wmicom3.dll export\
xcopy /y /f nircmd.exe export\
xcopy /y /f toast.exe export\
mkdir export\signed_driver
xcopy /y /f /e signed_driver export\signed_driver\

@echo off
python compile_toast.py py2exe
xcopy /f /y dist\toast.exe .\
rd /s /q dist
rd /s /q build

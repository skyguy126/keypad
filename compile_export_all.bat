@echo off
call make.bat
call compile_toast.bat
call export.bat
iscc setup.iss

@set /A _tic=%time:~0,2%*3600^
            +%time:~3,1%*10*60^
            +%time:~4,1%*60^
            +%time:~6,1%*10^
            +%time:~7,1% >nul

cd "%~dp0"
rmdir /q /s "E:\VLCTest"
Xcopy /E /I "VLCTest" "E:\VLCTest"

rem pause

E:\VLCTest\VLCPortable\VLCPortable.exe -f --play-and-exit "E:\VLCTest\bbb.webm"

@set /A _toc=%time:~0,2%*3600^
            +%time:~3,1%*10*60^
            +%time:~4,1%*60^
            +%time:~6,1%*10^
            +%time:~7,1% >nul
@set /A _elapsed=%_toc%-%_tic
@echo Play took %_elapsed% seconds.

pause
exit

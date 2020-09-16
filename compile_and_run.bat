call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

:a
cls
bam server_release && %~dp0\instagib_srv.exe -f sample-ictf.cfg

pause
goto a
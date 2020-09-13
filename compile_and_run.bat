call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
bam server_release && %~dp0\teeworlds_srv.exe -f example_config.cfg

pause
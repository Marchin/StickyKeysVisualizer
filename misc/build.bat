@echo off
REM set CompilerFlags=-MTd -nologo -Gm- -GR- -EHa- -Od -WX -W4 -wd4201 -wd4127 -wd4100 -wd4189 -wd4505 -FC -Zi -fp:fast
set CompilerFlags=-MT -nologo -Gm- -GR- -EHa- -O2 -WX -W4 -wd4201 -wd4127 -wd4100 -wd4189 -wd4505 -FC -fp:fast
set LinkerFlags= -incremental:no -opt:ref  user32.lib Gdi32.lib winmm.lib UxTheme.lib dwmapi.lib
IF NOT EXIST ..\build mkdir ..\build
pushd ..\build
cl /Fe"MarmotStickyKeys" %CompilerFlags% ..\code\win32_marmot.cpp /link  %LinkerFlags%
popd
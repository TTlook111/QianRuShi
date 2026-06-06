@echo off
@title IAR工程重命名工具1.0

setlocal enabledelayedexpansion
setlocal EnableExtensions
for /f %%i in ('dir /b *.eww') do (
set a=%%~ni
)
echo.  当前工程名为：%a%
echo.  请输入新的工程名
set /p newfile=
echo.  正在进行工程重命名，请稍等...

for /f "tokens=* delims=％" %%l in ('type %a%.eww') do (
set line=%%l
set line=!line:%a%=%newfile%!
echo !line!>>%newfile%.eww
)

for /f "tokens=* delims=％" %%l in ('type %a%.dep') do (
set line=%%l
set line=!line:%a%=%newfile%!
echo !line!>>%newfile%.dep
)

for /f "tokens=* delims=％" %%l in ('type %a%.ewd') do (
set line=%%l
set line=!line:%a%=%newfile%!
echo !line!>>%newfile%.ewd
)

for /f "tokens=* delims=％" %%l in ('type %a%.ewp') do (
set line=%%l
set line=!line:%a%=%newfile%!
echo !line!>>%newfile%.ewp
)

for /f "tokens=* delims=％" %%l in ('type %a%.ewt') do (
set line=%%l
set line=!line:%a%=%newfile%!
echo !line!>>%newfile%.ewt
)

del %a%.*

echo
exit
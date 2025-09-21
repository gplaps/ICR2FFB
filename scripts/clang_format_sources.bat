@echo off
echo Applying clang-format

echo Sources:
FOR /F "tokens=*" %%g IN ('dir /s /b ..\src\*.cpp') do (
  call clang-format -i %%g
  <NUL set /p dummyName="."
  )
echo.

echo Headers:
FOR /F "tokens=*" %%g IN ('dir /s /b ..\src\*.h') do (
  call clang-format -i %%g
  <NUL set /p dummyName="."
  )
echo.

echo.
echo Done

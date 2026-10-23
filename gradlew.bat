@echo off
where gradle >nul 2>nul
if %ERRORLEVEL% EQU 0 (
  gradle %*
  exit /b %ERRORLEVEL%
)
echo Gradle is not installed on PATH. Open this project in Android Studio and sync it, or install Gradle 8.9 and run: gradle wrapper
exit /b 1

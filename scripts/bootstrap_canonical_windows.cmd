@echo off
setlocal enabledelayedexpansion

set "BRANCH=%DAWAI_BRANCH%"
if "%BRANCH%"=="" set "BRANCH=canonical-224-audit"

set "REPO_URL=%DAWAI_REPO_URL%"
if "%REPO_URL%"=="" set "REPO_URL=https://github.com/kaeganscott26/DawAI_Ecosystem.git"

set "TARGET_DIR=%~1"
if "%TARGET_DIR%"=="" set "TARGET_DIR=%USERPROFILE%\DawAI_Ecosystem"

set "INSTALL_TARGET=%DAWAI_INSTALL_TARGET%"
if "%INSTALL_TARGET%"=="" set "INSTALL_TARGET=all"

where git >nul 2>nul
if errorlevel 1 (
  echo git is required.
  exit /b 1
)

if exist "%TARGET_DIR%\.git" (
  echo [bootstrap-windows] Updating existing clone in %TARGET_DIR%
  git -C "%TARGET_DIR%" fetch origin "%BRANCH%" || exit /b 1
  git -C "%TARGET_DIR%" checkout "%BRANCH%" || exit /b 1
  git -C "%TARGET_DIR%" pull --ff-only origin "%BRANCH%" || exit /b 1
) else (
  echo [bootstrap-windows] Cloning %REPO_URL%#%BRANCH% into %TARGET_DIR%
  git clone --branch "%BRANCH%" --single-branch "%REPO_URL%" "%TARGET_DIR%" || exit /b 1
)

echo [bootstrap-windows] Running PowerShell installer (target=%INSTALL_TARGET%)
powershell -NoProfile -ExecutionPolicy Bypass -File "%TARGET_DIR%\scripts\install_windows.ps1" -Target "%INSTALL_TARGET%"
if errorlevel 1 exit /b 1

echo [bootstrap-windows] Complete.
endlocal

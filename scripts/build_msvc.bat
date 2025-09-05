@echo off
setlocal

REM URL to the embeddable Python runtime
set PYTHON_URL=https://www.python.org/ftp/python/3.11.5/python-3.11.5-embed-amd64.zip

REM Configure and build with MSVC
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
if %errorlevel% neq 0 goto error

cmake --build build --config Release
if %errorlevel% neq 0 goto error

REM Prepare the embeddable Python runtime
python scripts\prepare_py_runtime.py --python-url %PYTHON_URL% --output python
if %errorlevel% neq 0 goto error

echo Build and runtime preparation complete.
goto end

:error
echo Build or runtime preparation failed.
exit /b 1

:end
exit /b 0

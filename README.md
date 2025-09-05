# mt5bridge_cpp

C++17 bridge embedding CPython to call the MetaTrader5 Python API from native apps (quotes, bars, orders). Ships with an embeddable Python runtime. Windows x64 only.

## Quickstart

1. Clone the repository.
2. Build the bridge using MSVC or MinGW.
   - For a one-step MSVC build run `scripts\build_msvc.bat`.
   - Alternatively follow the manual CMake commands below.
3. Prepare the runtime environment (see [Runtime setup](#runtime-setup)).
4. Run `build\bin\usage_example.exe` to confirm the setup.

## Build

### MSVC

Run the helper script to configure and build a Release binary:

```bat
scripts\build_msvc.bat
```

To invoke CMake manually instead:

```powershell
# From a Developer PowerShell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

### MinGW

```bash
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

## Runtime setup

1. Install MetaTrader 5 and log into an account.
2. Prepare an embeddable Python runtime. For example:

   ```bash
   python scripts/prepare_py_runtime.py --python-url https://www.python.org/ftp/python/3.11.5/python-3.11.5-embed-amd64.zip --output python
   ```

   Ensure the resulting `python` folder accompanies your application or set `PYTHONHOME` to that path.
3. Create a `bridge.ini` with the MT5 terminal path:

   ```ini
   terminal_path=C:\Path\To\MetaTrader5
   ```
4. Run `build\bin\usage_example.exe` from the build directory to verify the setup.

## Example usage

```cpp
#include <mt5bridge/mt5bridge.hpp>

int main() {
    mt5::Bridge bridge;
    bridge.initialize();
    auto quotes = bridge.get_quotes("EURUSD");
    // use quotes...
}
```

See the `examples` directory for more.

## Notes

- Only 64‑bit Windows builds are supported.
- Python 3.11+ is required.
- Issues and pull requests are welcome.


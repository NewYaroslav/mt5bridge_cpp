# mt5bridge_cpp

C++17 bridge embedding CPython to call the MetaTrader5 Python API from native apps (quotes, bars, orders). Ships with an embeddable Python runtime. Windows x64 only.

## Quickstart

1. Clone the repository.
2. Build the bridge using MSVC or MinGW.
3. Prepare the runtime environment.
4. Run an example to confirm the setup.

## Build

### MSVC

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
2. Ensure the `python` runtime folder from this repository accompanies your application or set `PYTHONHOME` to that path.
3. Create `bridge.ini` with `terminal_path` pointing at the MT5 terminal directory.

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


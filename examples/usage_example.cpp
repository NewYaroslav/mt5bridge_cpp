#include <windows.h>
#include <iostream>
#include <string>

#include <jansson.h>

// Typedefs for dynamically loaded functions from mt5_bridge.dll
using mt5bridge_initialize_t = int (*)(const wchar_t *);
using mt5bridge_shutdown_t = void (*)();
using mt5bridge_eval_t = json_t *(*)(json_t *);
using mt5bridge_last_error_t = const char *(*)();

int main() {
    // Load the bridge DLL at runtime
    HMODULE dll = LoadLibraryW(L"mt5_bridge.dll");
    if (!dll) {
        std::cerr << "Failed to load mt5_bridge.dll" << std::endl;
        return 1;
    }

    // Resolve the exported API functions
    auto mt5bridge_initialize =
        reinterpret_cast<mt5bridge_initialize_t>(
            GetProcAddress(dll, "mt5bridge_initialize"));
    auto mt5bridge_shutdown =
        reinterpret_cast<mt5bridge_shutdown_t>(
            GetProcAddress(dll, "mt5bridge_shutdown"));
    auto mt5bridge_eval = reinterpret_cast<mt5bridge_eval_t>(
        GetProcAddress(dll, "mt5bridge_eval"));
    auto mt5bridge_last_error =
        reinterpret_cast<mt5bridge_last_error_t>(
            GetProcAddress(dll, "mt5bridge_last_error"));

    if (!mt5bridge_initialize || !mt5bridge_shutdown || !mt5bridge_eval ||
        !mt5bridge_last_error) {
        std::cerr << "Failed to resolve mt5_bridge API" << std::endl;
        FreeLibrary(dll);
        return 1;
    }

    // Initialize the bridge runtime (no custom Python home)
    if (mt5bridge_initialize(nullptr) != 0) {
        std::cerr << "Initialization failed: " << mt5bridge_last_error()
                  << std::endl;
        FreeLibrary(dll);
        return 1;
    }

    // Prepare a simple request JSON: {"method": "terminal_info"}
    json_t *req = json_pack("{s:s}", "method", "terminal_info");
    json_t *resp = mt5bridge_eval(req);
    json_decref(req);

    if (!resp) {
        std::cerr << "mt5bridge_eval failed: " << mt5bridge_last_error()
                  << std::endl;
        mt5bridge_shutdown();
        FreeLibrary(dll);
        return 1;
    }

    // Dump the JSON response to stdout
    json_dumpf(resp, stdout, JSON_INDENT(2));
    std::cout << std::endl;
    json_decref(resp);

    mt5bridge_shutdown();
    FreeLibrary(dll);
    return 0;
}

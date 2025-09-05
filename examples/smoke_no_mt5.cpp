#include <windows.h>
#include <iostream>
#include <jansson.h>

using mt5bridge_initialize_t = int (*)(const wchar_t *);
using mt5bridge_shutdown_t = void (*)();
using mt5bridge_eval_t = json_t *(*)(json_t *);
using mt5bridge_last_error_t = const char *(*)();

int main() {
    // Load the bridge DLL dynamically.
    HMODULE dll = LoadLibraryW(L"mt5_bridge.dll");
    if (!dll) {
        std::cerr << "Failed to load mt5_bridge.dll" << std::endl;
        return 1;
    }

    // Resolve required API functions.
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

    // Initialize the bridge; this succeeds even without a running terminal.
    if (mt5bridge_initialize(nullptr) != 0) {
        std::cerr << "Initialization failed: " << mt5bridge_last_error()
                  << std::endl;
        FreeLibrary(dll);
        return 1;
    }

    // Issue a minimal request. Without a terminal the response will be
    // JSON null, which is still useful for exercising the pipeline.
    json_t *req = json_pack("{s:s,s:s,s:i}",
                            "method", "get_m1_bars",
                            "symbol", "EURUSD",
                            "count", 1);
    json_t *resp = mt5bridge_eval(req);
    json_decref(req);

    if (!resp) {
        std::cerr << "mt5bridge_eval failed: " << mt5bridge_last_error()
                  << std::endl;
        mt5bridge_shutdown();
        FreeLibrary(dll);
        return 1;
    }

    json_dumpf(resp, stdout, JSON_INDENT(2));
    std::cout << std::endl;
    json_decref(resp);

    mt5bridge_shutdown();
    FreeLibrary(dll);
    return 0;
}


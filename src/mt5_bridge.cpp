/*
 * mt5_bridge.cpp
 *
 * Embeds a CPython interpreter and exposes a small C API used by
 * native applications to invoke MetaTrader5 Python functionality.
 *
 * Invariants:
 *  - Only a single interpreter instance is ever created. Attempts to
 *    initialize more than once simply return success without side
 *    effects.
 *  - All calls into Python hold the Global Interpreter Lock (GIL)
 *    via PyGILState_Ensure to guarantee thread-safety.
 *  - Each request to mt5bridge_eval must be a JSON object that
 *    contains a "method" member describing the operation to perform.
 *
 * Usage pattern:
 *  mt5bridge_initialize(L"C:\\path\\to\\python");
 *  json_t *req = json_pack("{s:s, s:s, s:i}",
 *                          "method", "get_m1_bars",
 *                          "symbol", "EURUSD",
 *                          "count", 10);
 *  json_t *resp = mt5bridge_eval(req);
 *  ... // consume response
 *  json_decref(req);
 *  json_decref(resp);
 *  mt5bridge_shutdown();
 */

#include "mt5bridge/mt5bridge.hpp"

#include <Python.h>
#include <jansson.h>

#include <cstring>
#include <mutex>
#include <string>

namespace {
std::mutex g_mutex;                 // Guards interpreter lifetime.
bool g_initialized = false;         // True once Python is initialized.
std::string g_last_error;           // Last error message exposed by API.

void set_error(const std::string &msg) { g_last_error = msg; }
void clear_error() { g_last_error.clear(); }

void set_python_error() {
    PyObject *ptype = nullptr, *pvalue = nullptr, *ptrace = nullptr;
    PyErr_Fetch(&ptype, &pvalue, &ptrace);
    PyErr_NormalizeException(&ptype, &pvalue, &ptrace);
    PyObject *str = PyObject_Str(pvalue ? pvalue : Py_None);
    if (str) {
        g_last_error = PyUnicode_AsUTF8(str);
        Py_DECREF(str);
    } else {
        g_last_error = "unknown python error";
    }
    Py_XDECREF(ptype);
    Py_XDECREF(pvalue);
    Py_XDECREF(ptrace);
}
} // namespace

extern "C" {

MT5BRIDGE_API int mt5bridge_initialize(const wchar_t *python_home) {
    std::lock_guard<std::mutex> lock(g_mutex);
    clear_error();

    if (g_initialized)
        return 0; // already initialized

    Py_SetProgramName(const_cast<wchar_t *>(L"mt5bridge"));
    if (python_home)
        Py_SetPythonHome(const_cast<wchar_t *>(python_home));

    Py_Initialize();
    if (!Py_IsInitialized()) {
        set_error("Py_Initialize failed");
        return -1;
    }

    // Import MetaTrader5 module and initialize connection.
    PyGILState_STATE gs = PyGILState_Ensure();
    PyObject *mt5 = PyImport_ImportModule("MetaTrader5");
    if (!mt5) {
        set_python_error();
        PyGILState_Release(gs);
        Py_Finalize();
        return -1;
    }

    PyObject *res = PyObject_CallMethod(mt5, "initialize", nullptr);
    if (!res) {
        set_python_error();
        Py_DECREF(mt5);
        PyGILState_Release(gs);
        Py_Finalize();
        return -1;
    }
    Py_DECREF(res);
    Py_DECREF(mt5);
    PyGILState_Release(gs);

    // Release the GIL so that other threads may call into the API.
    PyEval_SaveThread();

    g_initialized = true;
    return 0;
}

MT5BRIDGE_API void mt5bridge_shutdown() {
    std::lock_guard<std::mutex> lock(g_mutex);
    clear_error();

    if (!g_initialized)
        return;

    PyGILState_STATE gs = PyGILState_Ensure();

    // Attempt to gracefully shutdown the MetaTrader5 module.
    PyObject *mt5 = PyImport_ImportModule("MetaTrader5");
    if (mt5) {
        PyObject *res = PyObject_CallMethod(mt5, "shutdown", nullptr);
        if (!res)
            set_python_error();
        else
            Py_DECREF(res);
        Py_DECREF(mt5);
    } else {
        PyErr_Clear();
    }

    PyGILState_Release(gs);

    Py_Finalize();
    g_initialized = false;
}

MT5BRIDGE_API json_t *mt5bridge_eval(json_t *request) {
    if (!request) {
        set_error("request is null");
        return nullptr;
    }
    if (!g_initialized) {
        set_error("bridge not initialized");
        return nullptr;
    }

    ScopedJson result; // ensures result is freed on error paths

    char *req_str = json_dumps(request, JSON_COMPACT);
    if (!req_str) {
        set_error("failed to serialize request");
        return nullptr;
    }

    PyGILState_STATE gs = PyGILState_Ensure();
    PyObject *json_mod = PyImport_ImportModule("json");
    PyObject *mt5 = PyImport_ImportModule("MetaTrader5");
    if (!json_mod || !mt5) {
        set_python_error();
        goto done;
    }

    PyObject *loads = PyObject_GetAttrString(json_mod, "loads");
    PyObject *dumps = PyObject_GetAttrString(json_mod, "dumps");
    if (!loads || !dumps) {
        set_python_error();
        goto done;
    }

    PyObject *req_dict = PyObject_CallFunction(loads, "s", req_str);
    if (!req_dict) {
        set_python_error();
        goto done;
    }

    PyObject *method_obj = PyDict_GetItemString(req_dict, "method");
    if (!method_obj) {
        set_error("missing method");
        Py_DECREF(req_dict);
        goto done;
    }
    const char *method = PyUnicode_AsUTF8(method_obj);

    PyObject *py_response = nullptr;

    if (std::strcmp(method, "get_m1_bars") == 0) {
        PyObject *symbol = PyDict_GetItemString(req_dict, "symbol");
        PyObject *count = PyDict_GetItemString(req_dict, "count");
        PyObject *tf = PyObject_GetAttrString(mt5, "TIMEFRAME_M1");
        if (symbol && count && tf) {
            py_response = PyObject_CallMethod(mt5, "copy_rates_from_pos", "sOii",
                                              PyUnicode_AsUTF8(symbol), tf, 0,
                                              static_cast<int>(PyLong_AsLong(count)));
        }
        Py_XDECREF(tf);
    } else if (std::strcmp(method, "open_market_buy") == 0) {
        PyObject *symbol = PyDict_GetItemString(req_dict, "symbol");
        PyObject *volume = PyDict_GetItemString(req_dict, "volume");
        if (symbol && volume) {
            // ORDER_TYPE_BUY == 0 in MetaTrader5 Python API
            PyObject *order = Py_BuildValue("{s:s,s:d,s:i}",
                                            "symbol", PyUnicode_AsUTF8(symbol),
                                            "volume", PyFloat_AsDouble(volume),
                                            "type", 0);
            py_response = PyObject_CallMethod(mt5, "order_send", "O", order);
            Py_DECREF(order);
        }
    } else {
        set_error("unknown method");
    }

    Py_DECREF(req_dict);

    if (py_response) {
        PyObject *resp_str = PyObject_CallFunctionObjArgs(dumps, py_response, NULL);
        if (resp_str) {
            const char *c_resp = PyUnicode_AsUTF8(resp_str);
            if (c_resp)
                result.ptr = json_loads(c_resp, 0, nullptr);
            else
                set_python_error();
            Py_DECREF(resp_str);
        } else {
            set_python_error();
        }
        Py_DECREF(py_response);
    } else if (PyErr_Occurred()) {
        set_python_error();
    }

    
 done:
    Py_XDECREF(dumps);
    Py_XDECREF(loads);
    Py_XDECREF(json_mod);
    Py_XDECREF(mt5);
    PyGILState_Release(gs);
    free(req_str);
    return result.release();
}

MT5BRIDGE_API const char *mt5bridge_last_error() {
    return g_last_error.empty() ? nullptr : g_last_error.c_str();
}

} // extern "C"


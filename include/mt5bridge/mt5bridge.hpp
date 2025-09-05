/*
 * MIT License
 *
 * Copyright (c) 2025 Aster Seker
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#if !defined(_WIN32)
#error "mt5bridge is only supported on Windows"
#endif

#include <jansson.h>
#include <stdint.h>

#ifdef MT5BRIDGE_BUILD
#define MT5BRIDGE_API extern "C" __declspec(dllexport)
#else
#define MT5BRIDGE_API extern "C" __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Initializes the bridge runtime.
 * Returns 0 on success, non-zero on error.
 */
MT5BRIDGE_API int mt5bridge_initialize(const wchar_t *python_home);

/* Shuts down the bridge runtime, releasing all resources. */
MT5BRIDGE_API void mt5bridge_shutdown();

/* Evaluates a MetaTrader5 request given as JSON object.
 * Returns a newly allocated json_t* result that must be freed with json_decref().
 */
MT5BRIDGE_API json_t *mt5bridge_eval(json_t *request);

/* Returns last error message or nullptr if no error. */
MT5BRIDGE_API const char *mt5bridge_last_error();

#ifdef __cplusplus
} /* extern "C" */
#endif

#ifdef __cplusplus
struct ScopedJson {
    json_t *ptr;

    explicit ScopedJson(json_t *p = nullptr) : ptr(p) {}
    ~ScopedJson() {
        if (ptr)
            json_decref(ptr);
    }

    json_t *get() const { return ptr; }
    json_t *release() {
        json_t *tmp = ptr;
        ptr = nullptr;
        return tmp;
    }

    operator json_t *() const { return ptr; }
};
#endif


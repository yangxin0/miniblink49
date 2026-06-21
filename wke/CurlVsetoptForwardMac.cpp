// CurlVsetoptForwardMac.cpp — provides Curl_vsetopt on macOS by forwarding to the
// system libcurl's public curl_easy_setopt.
//
// wke/wkeGlobalVar.cpp's wkeCurlSetopt (the wkeNetSetCurlOptions API) calls the
// libcurl-internal Curl_vsetopt(handle, option, va_list) directly. That symbol is
// private to libcurl and is NOT exported by the system -lcurl we link against
// (only curl_easy_setopt is). The vendored third_party/libcurl_7.69 sources are
// not built on macOS (and setopt.c is Linux-specific: it includes <linux/tcp.h>
// and the full curl-internal struct layouts, which would not match the system
// libcurl ABI anyway).
//
// Since curl encodes each option's argument type in its numeric value
// (option = CURLOPTTYPE_* base + index, with bases LONG=0, OBJECTPOINT=10000,
// FUNCTIONPOINT=20000, OFF_T=30000), we can recover the type, pull the single
// argument out of the va_list, and re-dispatch to the public curl_easy_setopt
// with the correctly-typed argument. This keeps the wke curl-options API working
// against the system libcurl. Windows still calls the vendored Curl_vsetopt.

#if !defined(_WIN32)

#include "third_party/libcurl/include/curl/curl.h"

#include <stdarg.h>

extern "C" CURLcode Curl_vsetopt(CURL* handle, CURLoption option, va_list arg)
{
    // Recover the option's argument-type bucket from its numeric value.
    const long typeBucket = (static_cast<long>(option) / 10000) * 10000;

    switch (typeBucket) {
    case CURLOPTTYPE_LONG:
        return curl_easy_setopt(handle, option, va_arg(arg, long));

    case CURLOPTTYPE_OBJECTPOINT:   // also STRINGPOINT / SLISTPOINT / BLOB (all pointers)
        return curl_easy_setopt(handle, option, va_arg(arg, void*));

    case CURLOPTTYPE_FUNCTIONPOINT:
        // Function pointers are passed through the same pointer slot.
        return curl_easy_setopt(handle, option, va_arg(arg, void*));

    case CURLOPTTYPE_OFF_T:
        return curl_easy_setopt(handle, option, va_arg(arg, curl_off_t));

    default:
        return CURLE_UNKNOWN_OPTION;
    }
}

#endif // !defined(_WIN32)

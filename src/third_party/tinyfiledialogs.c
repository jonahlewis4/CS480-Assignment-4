#if !defined(_WIN32)
#error "This minimal tinyfiledialogs implementation is Windows-only."
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <string.h>

#include "tinyfiledialogs.h"

#ifndef TFD_MAX_PATH
#define TFD_MAX_PATH 4096
#endif

static char g_tfd_path[TFD_MAX_PATH];

static void build_filter_string(
    char* out, size_t cap,
    int numPatterns, const char* const* patterns,
    const char* desc
) {
    if (!out || cap == 0) return;
    out[0] = '\0';

    const char* useDesc = (desc && desc[0]) ? desc : "Files";

    char patJoined[1024];
    patJoined[0] = '\0';

    if (numPatterns > 0 && patterns) {
        for (int i = 0; i < numPatterns; ++i) {
            const char* p = patterns[i];
            if (!p || !p[0]) continue;
            if (patJoined[0] != '\0') strncat(patJoined, ";", sizeof(patJoined) - strlen(patJoined) - 1);
            strncat(patJoined, p, sizeof(patJoined) - strlen(patJoined) - 1);
        }
    }

    if (patJoined[0] == '\0') {
        strncpy(patJoined, "*.*", sizeof(patJoined) - 1);
        patJoined[sizeof(patJoined) - 1] = '\0';
    }

    size_t pos = 0;
#define APPEND_MS(s) do {                                  \
        const char* ss = (s);                                  \
        size_t n = strlen(ss);                                 \
        if (pos + n + 1 >= cap) return;                        \
        memcpy(out + pos, ss, n);                              \
        pos += n;                                              \
        out[pos++] = '\0';                                     \
    } while(0)

    APPEND_MS(useDesc);
    APPEND_MS(patJoined);
    APPEND_MS("All Files");
    APPEND_MS("*.*");

    if (pos + 1 < cap) out[pos++] = '\0';
    else out[cap - 1] = '\0';

#undef APPEND_MS
}

static void set_initial_path(const char* def) {
    g_tfd_path[0] = '\0';
    if (def && def[0]) {
        strncpy(g_tfd_path, def, TFD_MAX_PATH - 1);
        g_tfd_path[TFD_MAX_PATH - 1] = '\0';
    }
}

const char* tinyfd_openFileDialog(
    const char* aTitle,
    const char* aDefaultPathAndFile,
    int aNumOfFilterPatterns,
    const char* const* aFilterPatterns,
    const char* aSingleFilterDescription,
    int aAllowMultipleSelects
) {
    (void)aAllowMultipleSelects;

    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    char filter[2048];
    build_filter_string(filter, sizeof(filter),
        aNumOfFilterPatterns, aFilterPatterns,
        aSingleFilterDescription);

    set_initial_path(aDefaultPathAndFile);

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrTitle = aTitle ? aTitle : "Open File";
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = g_tfd_path;
    ofn.nMaxFile = (DWORD)TFD_MAX_PATH;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn)) return g_tfd_path;
    return NULL;
}

const char* tinyfd_saveFileDialog(
    const char* aTitle,
    const char* aDefaultPathAndFile,
    int aNumOfFilterPatterns,
    const char* const* aFilterPatterns,
    const char* aSingleFilterDescription
) {
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    char filter[2048];
    build_filter_string(filter, sizeof(filter),
        aNumOfFilterPatterns, aFilterPatterns,
        aSingleFilterDescription);

    set_initial_path(aDefaultPathAndFile);

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrTitle = aTitle ? aTitle : "Save File";
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = g_tfd_path;
    ofn.nMaxFile = (DWORD)TFD_MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetSaveFileNameA(&ofn)) return g_tfd_path;
    return NULL;
}

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

    // Returns pointer to an internal static buffer (do NOT free).
    // Returns NULL if canceled.
    const char* tinyfd_openFileDialog(
        const char* aTitle,
        const char* aDefaultPathAndFile,
        int aNumOfFilterPatterns,
        const char* const* aFilterPatterns,
        const char* aSingleFilterDescription,
        int aAllowMultipleSelects
    );

    // Returns pointer to an internal static buffer (do NOT free).
    // Returns NULL if canceled.
    const char* tinyfd_saveFileDialog(
        const char* aTitle,
        const char* aDefaultPathAndFile,
        int aNumOfFilterPatterns,
        const char* const* aFilterPatterns,
        const char* aSingleFilterDescription
    );

#ifdef __cplusplus
}
#endif

#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cwchar>

// test_Win32
//  - using standard Win32 API LoadString to load string ID
//  - this API also errorneously truncates 'id' into 16-bit WORD, but that's not the reported issue
//    and I don't particularly care about LoadString, because it's in USER32 and thus be used in processes
//    that employ Win32k security mitigations
//
void test_Win32 (HMODULE h, UINT id, const wchar_t * expected, bool should_succeed) {
    const wchar_t * string = nullptr;
    if (auto length = LoadStringW (h, id, reinterpret_cast <LPWSTR> (&string), 0)) {
        std::wprintf (L"%016llx:%05x (%d): %s - ", (std::int64_t) (std::intptr_t) h, id, length, string);

        if (std::wcscmp (string + 24, expected) == 0) {
            std::printf ("correct\n");
        } else {
            std::printf ("INCORRECT\n");
        }
    } else {
        std::wprintf (L"%016llx:%05x - DOES NOT EXISTS\n", (std::int64_t) (std::intptr_t) h, id);
    }
}

// test_Custom
//  - custom function that replicates LoadString API, but without truncating string ID
//
void test_Custom (HMODULE module, UINT id, const wchar_t * expected, bool should_succeed) {
    if (HRSRC r = FindResourceEx (module, RT_STRING, MAKEINTRESOURCE ((id >> 4u) + 1u), 0)) {
        if (HGLOBAL resource = LoadResource (module, r)) {

            if (auto p = static_cast <const wchar_t *> (LockResource (resource))) {
                auto s = static_cast <unsigned short> (*p++);

                for (auto i = 0u; i != id % 16u; ++i) {
                    p += s;
                    s = static_cast <unsigned short> (*p++);
                }

                if (s) {
                    std::wprintf (L"%016llx:%05x (%d): %s - ", (std::int64_t) (std::intptr_t) module, id, s, p);

                    if (std::wcscmp (p + 24, expected) == 0) {
                        std::printf ("correct\n");
                    } else {
                        std::printf ("INCORRECT\n");
                    }
                    return;
                }
            }
        }
    }

    std::wprintf (L"%016llx:%05x - DOES NOT EXISTS\n", (std::int64_t) (std::intptr_t) module, id);
}

int main () {

    // problem demonstration
    //  - NULL = msvc-program.exe = this executable

    test_Win32 (NULL, 0x01234, L"0x01234", true); // correctly loads string 0x01234
    test_Win32 (NULL, 0x12345, L"0x12345", true); // incorrectly loads string 0x02345, BUT MSVC RC INCORRECTLY stored string 0x12345 in it
    test_Win32 (NULL, 0x02345, L"0x02345", true); // correctly loads string 0x02345, BUT MSVC RC INCORRECTLY stored string 0x12345 in it

    test_Custom (NULL, 0x01234, L"0x01234", true); // correctly loads string 0x01234
    test_Custom (NULL, 0x12345, L"0x12345", true); // fails to load string 0x12345, because it does not exists in msvc-program.EXE, due to the RC deficiency <- this is the problem
    test_Custom (NULL, 0x02345, L"0x02345", true); // correctly loads string 0x02345, but MSVC RC INCORRECTLY stored there string 0x12345 <- this is the problem

    std::printf ("\n");

    // expected behavior
    //  - 3rdpartyDLL.dll compiled with GCC/MinGW
    //  - correctly stores 3 strings, IDs: 0x01234, 0x02345 and 0x12345

    if (auto hLibrary = LoadLibrary (L"3rdpartyDLL.dll")) {

        test_Win32 (hLibrary, 0x01234, L"0x01234", true); // correctly loads string 0x01234
        test_Win32 (hLibrary, 0x12345, L"0x12345", true); // incorrectly loads string 0x02345 (LoadStringW deficiency)
        test_Win32 (hLibrary, 0x02345, L"0x02345", true); // correctly loads string 0x02345

        test_Custom (hLibrary, 0x01234, L"0x01234", true); // correctly loads string 0x01234
        test_Custom (hLibrary, 0x12345, L"0x12345", true); // correctly loads string 0x12345
        test_Custom (hLibrary, 0x02345, L"0x02345", true); // correctly loads string 0x02345

    } else {
        std::printf ("Failed to load \"3rdpartyDLL.dll\"\n");
    }
    return 0;
}

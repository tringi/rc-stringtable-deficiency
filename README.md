# Resource Compiler STRINGTABLE Deficiency
*Demonstration of MSVC Resource Compiler deficiency (unneeded limitation) when compiling STRINGTABLE resources*

## Problem summary

* [STRINGTABLE](https://learn.microsoft.com/en-us/windows/win32/menurc/stringtable-resource) strings are stored in groups of 16, in integer-identified resources entry.
* These integer-identified resources are 16-bit. The integer identifier for a STRINGTABLE resource is determined as `(string_id >> 4) + 1`.
* Although string IDs are documented also as 16-bit, this makes it trivial for them to be 20-bit, e.g. string 0x12345 would live in resource 0x1235 (valid 16-bit).
* **In fact GCC (MinGW) resource compiler already does exactly that!**
* While standard API [LoadStringW](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-loadstringw) truncates string ID to 16-bit, it is trivial to implement custom loader using [FindResourceEx](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-findresourceexw), see `test_Custom` in the code.

## Problem fallout

* Porting projects from GCC/MinGW to MSVC which use 20-bit resource strings requires extra effort in renumbering. Expecially adds burden where plugins are involved.

## Solution

1. Lift the restriction on string IDs. Allow them to be 20-bit.
2. Add warning when the value is truncated.

## Problem demonstration

**3rdpartyDLL.dll** is compiled with MinGW, it contains strings with IDs `0x01234`, `0x12345` and `0x02345`.

It is impossible to directly port to MSVC, because RC **silently** truncates `0x12345` to `0x02345` and then reports misleading error, that string `0x02345` already exists.

The example program `msvc-program.exe` showcases results and their expectations (documented in comments).

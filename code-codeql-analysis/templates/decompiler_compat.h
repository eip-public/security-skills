#ifndef DECOMPILER_COMPAT_H
#define DECOMPILER_COMPAT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define __stdcall
#define __cdecl
#define __fastcall
#define __thiscall
#define __usercall
#define __noreturn

/* Ghidra-style unknowns */
typedef uint8_t undefined;
typedef uint8_t undefined1;
typedef uint16_t undefined2;
typedef uint32_t undefined4;
typedef uint64_t undefined8;
typedef uint8_t byte;
typedef uint16_t word;
typedef uint32_t dword;
typedef uint64_t qword;

/* Common decompiler aliases */
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long ulonglong;
typedef long long longlong;

/* Minimal Windows placeholders; extend per target as needed */
typedef void *HANDLE;
typedef void *HWND;
typedef void *HINSTANCE;
typedef void *HMODULE;
typedef void *LPVOID;
typedef const char *LPCSTR;
typedef char *LPSTR;
typedef uint32_t DWORD;
typedef int BOOL;
typedef uintptr_t WPARAM;
typedef intptr_t LPARAM;
typedef intptr_t LRESULT;

#endif /* DECOMPILER_COMPAT_H */

#pragma once
#pragma comment(linker, "/entry:WinMain")
#include <windows.h>
#include <stdio.h>
#include <Winternl.h>
#include <winnt.h>
#pragma intrinsic(strlen)
#pragma intrinsic(wcslen)
#pragma function(memset, memcmp)
int __cdecl memcmp(void const* _Buf1, void const* _Buf2, size_t _Size);
void * __cdecl memset(void *pTarget, int value, size_t cbTarget);
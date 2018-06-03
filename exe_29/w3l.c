/*
	w3l: a PvPGN loader for Warcraft 3 1.29+
	Copyright (C) 2008 Rupan, Keres, Phatdeeva

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifdef _MSC_VER
//#  undef UNICODE
#  if _MSC_VER>=1900
#    define STDC99
#  elif
#    if _MSC_VER > 1400
#      define _CRT_SECURE_NO_WARNINGS
#      define snprintf _snprintf
#    endif
#  endif
#endif

#include "nodefaultlib.h"
#include <tchar.h>
#include "peb.h"

void debug(char *message, ...);

DWORD GetProcessBaseAddress(HANDLE hThread, HANDLE hProcess);
DWORD GetProcessBaseAddressEbx(HANDLE hThread, HANDLE hProcess);
DWORD GetProcessBaseAddressWinapi(HANDLE hProcess);
HMODULE sm_LoadNTDLLFunctions();

#define VERSION "1.29a"

#define DLL_NAME TEXT("w3lh29.dll")

#define WAR3_NOT_FOUND_ERR TEXT("Could not start Warcraft III.exe! Make sure the loader is in your Warcraft III install directory.")
#define DLL_NOT_FOUND_ERR TEXT("Could not find ") DLL_NAME TEXT("! Make sure the loader dll is in your Warcraft III install directory.")
#define ALLOCATE_ERR TEXT("Could not find allocate memory in Warcraft III.exe process!")
#define WRITE_ERR TEXT("Could not find write memory in Warcraft III.exe process!")
#define LOADLIB_ERR TEXT("Could not find LoadLibrary address!")
#define REMOTE_THREAD_ERR TEXT("Could not execute remote thread!")
unsigned char DEPPatchNew[] = { 0x80, 0x00, 0xF3 };
unsigned char DEPPatchOrig[] = { 0x81, 0x01, 0xF4 };

#ifdef _UNICODE
#define LOAD_LIB_FUNC_NAME "LoadLibraryW"
#else
#define LOAD_LIB_FUNC_NAME "LoadLibraryA"
#endif

/* Load war3.exe. Inject wl29.dll */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	STARTUPINFO startupinfo;
	DWORD_PTR baseAddr;
	LPTSTR commandline;
	PROCESS_INFORMATION processinfo;
	ULONG ExecuteFlags = 0x2;//MEM_EXECUTE_OPTION_ENABLE;
	BOOL applied = FALSE;
	HANDLE hDLLFile;
	DWORD dllFileSize;
	TCHAR fullPath[1024];
	LPVOID lpRemoteDLLPathAddr;
	LPVOID lpLoadLibAddr;
	HANDLE hDLLThread;
	HANDLE hKernel32;
	DWORD thread;

	GetStartupInfo(&startupinfo);
	commandline = GetCommandLine();

	// load NtQueryInformationProcess once
	sm_LoadNTDLLFunctions();

	// First try Warcrat III.exe
	if (!CreateProcess(L"Warcraft III.exe", commandline, 0, 0, FALSE, CREATE_SUSPENDED, 0, 0, &startupinfo, &processinfo)) {
		MessageBox(NULL, WAR3_NOT_FOUND_ERR, TEXT("Loader Error"), MB_OK);
		ExitProcess(2);
	}
	baseAddr = GetProcessBaseAddress(processinfo.hThread, processinfo.hProcess);
	debug("Base: %x\r\n", baseAddr);

	GetFullPathName(DLL_NAME, 1024, fullPath, NULL);
	debug("Full dll path: %ls\r\n", fullPath);
	hDLLFile = CreateFile(fullPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDLLFile == INVALID_HANDLE_VALUE) {
		TerminateProcess(processinfo.hProcess, 1);
		MessageBox(NULL, DLL_NOT_FOUND_ERR, TEXT("Loader Error"), MB_OK);
		ExitProcess(2);
	}
	CloseHandle(hDLLFile);

	lpRemoteDLLPathAddr = VirtualAllocEx(processinfo.hProcess, 0, (_tcslen(fullPath) + 1) * sizeof(TCHAR) + 2, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	debug("Remote DLL path: 0x%X\r\n", lpRemoteDLLPathAddr);
	if (lpRemoteDLLPathAddr == NULL) {
		TerminateProcess(processinfo.hProcess, 1);
		MessageBox(NULL, ALLOCATE_ERR, TEXT("Loader Error"), MB_OK);
		ExitProcess(2);
	}
	if (!WriteProcessMemory(processinfo.hProcess, lpRemoteDLLPathAddr, fullPath, (_tcslen(fullPath) + 1) * sizeof(TCHAR) + 2, NULL)) {
		TerminateProcess(processinfo.hProcess, 1);
		MessageBox(NULL, WRITE_ERR, TEXT("Loader Error"), MB_OK);
		ExitProcess(2);
	}
	hKernel32 = GetModuleHandle(TEXT("kernel32.dll"));
	lpLoadLibAddr = GetProcAddress(hKernel32, LOAD_LIB_FUNC_NAME);
	if (lpLoadLibAddr == NULL) {
		TerminateProcess(processinfo.hProcess, 1);
		MessageBox(NULL, LOADLIB_ERR, TEXT("Loader Error"), MB_OK);
		ExitProcess(2);
	}
	debug("LoadLibrary address: 0x%X\r\n", lpLoadLibAddr);
	hDLLThread = CreateRemoteThread(processinfo.hProcess, NULL, 0, lpLoadLibAddr, lpRemoteDLLPathAddr, 0, &thread);
	dllFileSize = GetLastError();
	if (hDLLThread == NULL) {
		TerminateProcess(processinfo.hProcess, 1);
		MessageBox(NULL, REMOTE_THREAD_ERR, TEXT("Loader Error"), MB_OK);
		ExitProcess(2);
	}
	WaitForSingleObject(hDLLThread, INFINITE);

	/*
	hDLLFile = CreateFile(fullPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDLLFile == INVALID_HANDLE_VALUE) {
		TerminateProcess(processinfo.hProcess, 1);
		MessageBox(NULL, DLL_NOT_FOUND_ERR, TEXT("Loader Error"), MB_OK);
		ExitProcess(2);
	}
	
	dllFileSize = GetFileSize(hDLLFile, NULL);
	debug("DLL size: %ld\r\n", dllFileSize);
	lpRemoteDLLAddr = VirtualAllocEx(processinfo.hProcess, NULL, dllFileSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	debug("Remote dll address: 0x%X\r\n", lpRemoteDLLAddr);
	lpBuffer = HeapAlloc(GetProcessHeap(), 0, dllFileSize);
	if (lpBuffer == NULL) {
		TerminateProcess(processinfo.hProcess, 1);
		MessageBox(NULL, TEXT("Couldn't allocate memory"), TEXT("Loader Error"), MB_OK);
		ExitProcess(2);
	}
	if (!ReadFile(hDLLFile, lpBuffer, dllFileSize, &dwBytesRead, NULL) || dwBytesRead != dllFileSize)
	{
		TerminateProcess(processinfo.hProcess, 1);
		MessageBox(NULL, DLL_NOT_READ_ERR, TEXT("Loader Error"), MB_OK);
		ExitProcess(2);
	}
	WriteProcessMemory(processinfo.hProcess, lpRemoteDLLAddr, lpBuffer, dllFileSize, NULL);*/
	ResumeThread(processinfo.hThread);
	ExitProcess(0);
}


// First try WinAPI call, in case of error try ebx hack
DWORD GetProcessBaseAddress(HANDLE hThread, HANDLE hProcess) {
	DWORD   baseAddress;
	baseAddress = GetProcessBaseAddressWinapi(hProcess);
	if (baseAddress != 0xFFFFFFFF)
		return baseAddress;
	return GetProcessBaseAddressEbx(hThread, hProcess);
}

// Dirty Hack. Don't try to do this at home!
// Takes value at (ebx + 8) and surprizingly it's exe base address
DWORD GetProcessBaseAddressEbx(HANDLE hThread, HANDLE hProcess)
{
	DWORD   baseAddress = 0xFFFFFFFF;
	DWORD ebx;
	CONTEXT contx;
	SIZE_T numread = 0;
	char buf[4];

	contx.ContextFlags = CONTEXT_ALL;
	GetThreadContext(hThread, &contx);
	ebx = contx.Ebx;
	debug("Read ebx:0x%08X\r\n", ebx);
	if (!ReadProcessMemory(hProcess, (LPCVOID)(ebx + 8), buf, 4, &numread)) {
		debug("Can't read base address\r\n");
		return 0xFFFFFFFF;
	}

	baseAddress = *(DWORD*)buf;
	return baseAddress;
}

DWORD GetProcessBaseAddressWinapi(HANDLE hProcess)
{
	debug("NtQueryInformationProcess start\r\n");

	DWORD baseAddress = 0xFFFFFFFF;
	
	DWORD pebBaseAddress;
	DWORD dwSize = 0;
	DWORD dwSizeNeeded = 0;
	DWORD dwBytesRead = 0;
	DWORD dwBufferSize = 0;
	HANDLE hHeap = 0;
	WCHAR *pwszBuffer = NULL;

	smPPROCESS_BASIC_INFORMATION pbi = NULL;

	// Attempt to access process
	if (hProcess == INVALID_HANDLE_VALUE) {
		debug("Invalid hProcess handle\r\n");
		return baseAddress;
	}

	// Try to allocate buffer 
	hHeap = GetProcessHeap();

	dwSize = sizeof(smPROCESS_BASIC_INFORMATION);

	pbi = (smPPROCESS_BASIC_INFORMATION)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, dwSize);
	
	// Did we successfully allocate memory
	if (!pbi) {
		debug("Faield to allocate pbi\r\n");
		return baseAddress;
	}

	// Attempt to get basic info on process
	NTSTATUS dwStatus = gNtQueryInformationProcess(hProcess,
		ProcessBasicInformation,
		pbi,
		dwSize,
		&dwSizeNeeded);

	// If we had error and buffer was too small, try again
	// with larger buffer size (dwSizeNeeded)
	if (dwStatus >= 0 && dwSize < dwSizeNeeded)
	{
		if (pbi)
			HeapFree(hHeap, 0, pbi);
		pbi = (smPPROCESS_BASIC_INFORMATION)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, dwSizeNeeded);
		if (!pbi) {
			debug("Failed to allocate pbi\r\n");
			return baseAddress;
		}

		dwStatus = gNtQueryInformationProcess(hProcess,
			ProcessBasicInformation,
			pbi, dwSizeNeeded, &dwSizeNeeded);
	}

	// Did we successfully get basic info on process
	if (dwStatus >= 0)
	{
		debug("NtQueryInformationProcess success\r\n");
		pebBaseAddress = (DWORD)pbi->PebBaseAddress;
		if (pbi)
			HeapFree(hHeap, 0, pbi);

		SIZE_T numread = 0;
		char buf[4];
		if (!ReadProcessMemory(hProcess, (LPCVOID)(pebBaseAddress + 8), buf, 4, &numread)) {
			debug("Can't read base address (winapi)\r\n");
			return 0xFFFFFFFF;
		}
		baseAddress = *(DWORD*)buf;
	}
	if (pbi)
		HeapFree(hHeap, 0, pbi);

	return baseAddress;
}

HMODULE sm_LoadNTDLLFunctions()
{
	HMODULE hNtDll = LoadLibrary(_T("ntdll.dll"));
	if (hNtDll == NULL) {
		debug("LoadLibrary ntdll failed\r\n");
		return NULL;
	}

	gNtQueryInformationProcess = (pfnNtQueryInformationProcess)GetProcAddress(hNtDll, "NtQueryInformationProcess");
	if (gNtQueryInformationProcess == NULL) {
		FreeLibrary(hNtDll);
		debug("gNtQueryInformationProcess definition failed\r\n");
		return NULL;
	}

	debug("LoadNTDLLFunctions success\r\n");

	return hNtDll;
}



void debug(char *message, ...) {
#ifdef _DEBUG
	DWORD temp;
	HANDLE file;
	va_list args;
	char buf[1024];

	memset(buf, 0, sizeof(buf));
	va_start(args, message);
	wvsprintfA(buf, message, args);
	file = CreateFile(L"debug.log", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	SetFilePointer(file, 0, 0, FILE_END);
	WriteFile(file, buf, (DWORD)strlen(buf), &temp, NULL);
	CloseHandle(file);
#endif
}

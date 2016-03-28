/*
    w3l: a PvPGN loader for Warcraft 3 1.22+
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

#include <windows.h>
#include <stdio.h>
#include <Winternl.h>

int InjectDll(PROCESS_INFORMATION processinfo, LPCVOID gameDllOffset, LPCVOID helper);
int InjectByte(PROCESS_INFORMATION processinfo, LPCVOID offset, char byteOrig, char byteSet);
void debug(char *message, ...);

DWORD GetProcessBaseAddress(HANDLE hThread, HANDLE hProcess);

/* offset in war3.exe that specifies the Game.dll to load */
#define	GAME_DLL_127 (LPCVOID)0x131CA20
#define	GAME_DLL_125 (LPCVOID)0x456B9C
#define	GAME_DLL_122 (LPCVOID)0x456B64
#define GAME_DLL_118 (LPCVOID)0x4524D0
#define GAME_DLL_UNK (LPCVOID)0x4534d0

/* offset in war3.exe from base address that specifies the Game.dll to load */
#define	BASE_GAME_DLL_127 (DWORD)0x5CA20
#define	BASE_GAME_DLL_125 (DWORD)0x56B9C
#define	BASE_GAME_DLL_122 (DWORD)0x56B64
#define BASE_GAME_DLL_118 (DWORD)0x524D0
#define BASE_GAME_DLL_UNK (DWORD)0x534d0

#define VERSION "1.22a-1.27a" /* version this loader expects */
#define HELPER_DLL_NAME "w3lh.dll" /* name of dll injected in place of Game.dll */
#define HELPER27_DLL_NAME "wl27.dll" /* name of dll injected in place of Game.dll */
#define GAME_DLL_NAME "Game.dll" /* name of Game.dll (in case it changes) */
#define GAME_DLL_NAME_LEN 8 /* Game.dll length */
#define WAR3_NOT_FOUND_ERR "Could not start War3.exe! Make sure the loader is in your Warcraft III install directory."
#define DEP_PATCH_1 (LPCVOID)0x400169
#define DEP_PATCH_2 (LPCVOID)0x40016F
#define DEP_PATCH_3 (LPCVOID)0x12C0177
#define BASE_DEP_PATCH_1 (DWORD)0x169
#define BASE_DEP_PATCH_2 (DWORD)0x16F
#define BASE_DEP_PATCH_3 (DWORD)0x177

unsigned char DEPPatchNew[] = { 0xF3, 0x00, 0x80};
unsigned char DEPPatchOrig[] = { 0xF4, 0x01, 0x81};
/* Load war3.exe. Patch its memory to replace Game.dll with the helper DLL.
   Once patched, resume war3.exe and exit. war3.exe will then load the helper dll and execute
   GameMain */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	STARTUPINFO startupinfo;
	DWORD_PTR baseAddr;
	LPTSTR commandline;
	PROCESS_INFORMATION processinfo;
	int i;
	ULONG ExecuteFlags = 0x2;//MEM_EXECUTE_OPTION_ENABLE;
	BOOL applied = FALSE;
	const char *errmsg;
	char buf[1024];
	int rval;
	const LPCVOID game27_dll_offsets[] = {
		GAME_DLL_127,		
	};
	const LPCVOID game_dll_offsets[] = {		
		GAME_DLL_125,
		GAME_DLL_122,
		GAME_DLL_118,
		GAME_DLL_UNK
	};

	const DWORD base_game27_dll_offsets[] = {
		BASE_GAME_DLL_127		
	};
	const DWORD base_game_dll_offsets[] = {
		BASE_GAME_DLL_125,
		BASE_GAME_DLL_122,
		BASE_GAME_DLL_118,
		BASE_GAME_DLL_UNK
	};


	GetStartupInfo(&startupinfo);
	commandline = GetCommandLine();

	if(!CreateProcess(L"war3.exe", commandline, 0, 0, FALSE, CREATE_SUSPENDED, 0, 0, &startupinfo, &processinfo)) {
		MessageBoxA(0, WAR3_NOT_FOUND_ERR, "Error", MB_OK);
		ExitProcess(2);
	}
		
	baseAddr = GetProcessBaseAddress(processinfo.hThread, processinfo.hProcess);
	debug("base: %x\r\n", baseAddr);
	
    // 1.27a+
	for (i = 0; i<1; i++) {
		if (baseAddr == 0xFFFFFFFF) {
			debug("[w3l] Trying offset 0x%08X... ", game27_dll_offsets[i]);
			rval = InjectDll(processinfo, game27_dll_offsets[i], &HELPER27_DLL_NAME);
		}
		else {
			debug("[w3l] Trying base offset 0x%08X with base 0x%08X...", base_game27_dll_offsets[i], baseAddr);
			rval = InjectDll(processinfo, (LPCVOID)(baseAddr + base_game27_dll_offsets[i]), &HELPER27_DLL_NAME);

		}
		if (rval == 0) {
			debug("Success.\r\n");
			applied = TRUE;
			break;
		}
		else if (rval == -1) errmsg = "Bad offset";
		else if (rval == 1) errmsg = "Unable to read memory";
		else if (rval == 2) errmsg = "Unable to write memory";
		else if (rval == 3) errmsg = "Unable to set page permissions";
		debug("%s\r\n", errmsg);
	}
	// 1.22a +
	if (applied == FALSE) {

		for (i = 0; i < 5; i++) {
			if (baseAddr == 0xFFFFFFFF) {
				debug("[w3l] Trying offset 0x%08X... ", game_dll_offsets[i]);
				rval = InjectDll(processinfo, game_dll_offsets[i], &HELPER_DLL_NAME);
			}
			else {
				debug("[w3l] Trying base offset 0x%08X with base 0x%08X...", base_game_dll_offsets[i], baseAddr);
				rval = InjectDll(processinfo, (LPCVOID)(baseAddr + base_game_dll_offsets[i]), &HELPER_DLL_NAME);

			}
			if (rval == 0) {
				debug("Success.\r\n");
				break;
			}
			else if (rval == -1) errmsg = "Bad offset";
			else if (rval == 1) errmsg = "Unable to read memory";
			else if (rval == 2) errmsg = "Unable to write memory";
			else if (rval == 3) errmsg = "Unable to set page permissions";
			debug("%s\r\n", errmsg);
		}
	}


	if(rval) {
		TerminateProcess(processinfo.hProcess, 1);
		snprintf(buf, 1024, "There was an error patching war3.exe (%s). Make sure you are using version %s.", errmsg, VERSION);
		MessageBoxA(0, buf, "Patch Error", MB_OK);
		ExitProcess(3);
	}
/* Trying to patch exe for /NXCOMPAT:NO */
	if (baseAddr == 0xFFFFFFFF) {
		rval = InjectByte(processinfo, DEP_PATCH_1, DEPPatchOrig[0], DEPPatchNew[0]);
		if (rval) debug("NXCOMPAT @ 0x%08X patching failed\r\n", DEP_PATCH_1);
		rval = InjectByte(processinfo, DEP_PATCH_2, DEPPatchOrig[1], DEPPatchNew[1]);
		if (rval) debug("NXCOMPAT @ 0x%08X patching failed\r\n", DEP_PATCH_2);
		rval = InjectByte(processinfo, DEP_PATCH_3, DEPPatchOrig[2], DEPPatchNew[2]);
		if (rval) debug("NXCOMPAT @ 0x%08X patching failed\r\n", DEP_PATCH_3);
	}
	else {
		rval = InjectByte(processinfo, (LPCVOID)(baseAddr + BASE_DEP_PATCH_1), DEPPatchOrig[0], DEPPatchNew[0]);
		if (rval) debug("NXCOMPAT @ 0x%08X patching failed\r\n", baseAddr + BASE_DEP_PATCH_1);
		rval = InjectByte(processinfo, (LPCVOID) (baseAddr + BASE_DEP_PATCH_2), DEPPatchOrig[1], DEPPatchNew[1]);
		if (rval) debug("NXCOMPAT @ 0x%08X patching failed\r\n", baseAddr + BASE_DEP_PATCH_2);
		rval = InjectByte(processinfo, (LPCVOID)(baseAddr + BASE_DEP_PATCH_3), DEPPatchOrig[2], DEPPatchNew[2]);
		if (rval) debug("NXCOMPAT @ 0x%08X patching failed\r\n", baseAddr + BASE_DEP_PATCH_3);
	}
	
	ResumeThread(processinfo.hThread);
	ExitProcess(0);
}

/* write to process memory at offset */
int InjectDll(PROCESS_INFORMATION processinfo, LPCVOID offset, LPCVOID helper) {
	char buf[GAME_DLL_NAME_LEN];
	SIZE_T numread=0, numwritten=0;
	DWORD oldprotect, newprotect = PAGE_EXECUTE_READWRITE;

	if(!ReadProcessMemory(processinfo.hProcess, offset, buf, GAME_DLL_NAME_LEN, &numread)) {
		return 1; /* +1 indicates a memory read error */
	}
	if(memcmp(buf, GAME_DLL_NAME, GAME_DLL_NAME_LEN)) {
		return -1; /* -1 indicates a bad offset */
	}
	if(!VirtualProtectEx(processinfo.hProcess, (LPVOID)offset, (SIZE_T)GAME_DLL_NAME_LEN, newprotect, &oldprotect)) {
		return 3; /* +3 indicates we can't set page permissions */
	}
	if(!WriteProcessMemory(processinfo.hProcess, (LPVOID)offset, helper, GAME_DLL_NAME_LEN, &numwritten)) {
		return 2; /* +2 indicates a memory write error */
	}
	if(!VirtualProtectEx(processinfo.hProcess, (LPVOID)offset, (SIZE_T)GAME_DLL_NAME_LEN, oldprotect, &newprotect)) {
		return 3;
	}
	return 0;
}

// Dirty Hack. Don't try to do this at home!
// Takes value at (ebx + 8) and surprizingly it's exe base address
DWORD GetProcessBaseAddress(HANDLE hThread, HANDLE hProcess)
{
	DWORD   baseAddress = 0xFFFFFFFF;
	DWORD ebx;
	CONTEXT contx;
	SIZE_T numread = 0;
	char buf[4];

	contx.ContextFlags = CONTEXT_ALL;
	GetThreadContext(hThread, &contx);
	ebx = contx.Ebx;
	if (!ReadProcessMemory(hProcess, (LPCVOID)(ebx + 8), buf, 4, &numread)) {
		debug("Can't read base address");
		return 0xFFFFFFFF;
	}

	baseAddress = *(DWORD*)buf;
	return baseAddress;
}

/* write to process memory at offset */
int InjectByte(PROCESS_INFORMATION processinfo, LPCVOID offset, char byteOrig, char byteSet) {
	char buf[1];
	SIZE_T numread=0, numwritten=0;
	DWORD oldprotect, newprotect = PAGE_EXECUTE_READWRITE;

	if(!ReadProcessMemory(processinfo.hProcess, offset, buf, 1, &numread)) {
		return 1; /* +1 indicates a memory read error */
	}
	if(memcmp(buf, &byteOrig, 1)) {
		return 4; /* 4 indicates a bad offset */
	}
	if(!VirtualProtectEx(processinfo.hProcess, (LPVOID)offset, (SIZE_T)1, newprotect, &oldprotect)) {
		return 3; /* +3 indicates we can't set page permissions */
	}
	if(!WriteProcessMemory(processinfo.hProcess, (LPVOID)offset, &byteSet, 1, &numwritten)) {
		return 2; /* +2 indicates a memory write error */
	}
	if(!VirtualProtectEx(processinfo.hProcess, (LPVOID)offset, (SIZE_T)1, oldprotect, &newprotect)) {
		return 3;
	}
	return 0;
}
void debug(char *message, ...) {
#ifdef _DEBUG
    DWORD temp;
	HANDLE file;
	va_list args;
	char buf[1024];

	memset(buf, 0, sizeof(buf));
	va_start(args, message);
	vsnprintf(buf, sizeof(buf) - 1, message, args);

	file = CreateFile(L"debug.log", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	SetFilePointer(file, 0, 0, FILE_END);
	WriteFile(file, buf, (DWORD)strlen(buf), &temp, NULL);
	CloseHandle(file);
#endif
}

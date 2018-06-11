/*
    w3l: a PvPGN loader for Warcraft 3 1.27a+
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
	#undef UNICODE
	#if _MSC_VER > 1400
		#define _CRT_SECURE_NO_WARNINGS
	#endif
#endif


#include <windows.h>
#include <stdio.h>
#include "w3lh.h"
#include "patches.h"
#include "dep.h"
#include "functions.h"

//#define DEBUG /* undef to disable logging */
#define LATENCY_FILE "latency.txt"
#define GAME_DLL_NAME "Game.dll"
#define GAME_MAIN "GameMain"
#define GAME_DLL_SIZE 0xA00000 /* size of real "Game.dll" in memory */

#define SUCCESS 0
#define ERR_GAME_DLL_LOAD 1
#define ERR_GAME_MAIN 2
#define ERR_SIG_NOT_FOUND 3
#define ERR_MEM_WRITE 3

#define DEFAULT_LATENCY 0x64

#define GAME_DLL_LOAD_ERROR "Could not open " ## GAME_DLL_NAME
#define GAME_MAIN_ERROR "Could not locate procedure " ## GAME_MAIN
#define SIG_NOT_FOUND_ERROR "Could not find location to patch."
#define MEMORY_WRITE_ERROR "Could not write to process memory."

#define LPH_HASH_INIT_CALL_OFFSET 0x9 /* offset of hash_init call in the logon proof password hashing routine */
#define LPH_DO_HASH_CALL_OFFSET 0x42  /* offset of do_hash call */

int called; /* used to stop multiple calls to DLL entry point */
FARPROC real_game_main; /* pointer to the real "Game.dll" GameMain function */
HMODULE game_dll_base; /* pointer to base of the real "Game.dll" */
HANDLE app_process; /* handle to the war3.exe process */

char read_latency ( ) {
    HFILE file;
	OFSTRUCT ofFile;
	char buf[4];
    int lt, readNum;
    file = OpenFile ( LATENCY_FILE, &ofFile, OF_READ);
    if ( file == HFILE_ERROR )
        return DEFAULT_LATENCY;
    ReadFile( file, buf, 3, &readNum, NULL );
    CloseHandle( file );
	buf[3] = '\0';
	lt = StrToInt( buf );
    if ( lt > 255 || lt < 30)
        return DEFAULT_LATENCY;    
    return (char )lt;
}


/* write patch to memory */
int apply_patch(char *ptr, t_patch *patch) {
	DWORD temp, oldprot, newprot = PAGE_EXECUTE_READWRITE;
	debug("Patching (%s) 0x%08x: ", patch->name, ptr);

	VirtualProtectEx(app_process, ptr, patch->length, newprot, &oldprot);
	if(WriteProcessMemory(app_process, ptr, patch->data, patch->length, &temp)) {
		debug("%d of %d bytes written\r\n", temp, patch->length);
	}
	else {
		debug("failed.\r\n");
		return ERR_MEM_WRITE;
	}
	VirtualProtectEx(app_process, ptr, patch->length, oldprot, &newprot);
	return SUCCESS;
}

/* search buffer of size scan_len for sig */
int find_sig(char *base, t_sig *sig, int scan_len) {
	int i, j, found = 0;

	for(i = 0; i < scan_len; i++) {
		found = 1;
		for(j = 0; j < sig->length; j++) {
			if(((base[i + j]) & (sig->data[j*2])) != (sig->data[j*2 + 1])) {
				found = 0;
				break;
			}
		}
		if(found)
			return i + sig->delta;
	}

	return -1;
}

/* apply a NULL terminated array of sig/patch pairs */
int apply_patches(char *base, void *patches[], int scan_len) {
	int i = 0, patch_loc;
    int ret = SUCCESS; // return value
	t_sig *cur_sig;
	t_patch *cur_patch;
	while(patches[i] != NULL) {
		cur_sig = (t_sig *)(patches[i]);
		cur_patch = (t_patch *)(patches[i + 1]);
		debug("Searching for sig (%s), length 0x%x: ", cur_sig->name, cur_sig->length);
		patch_loc = find_sig(base, cur_sig, scan_len);
		if(patch_loc == -1) {
			debug("not found.\r\n");
			ret = ERR_SIG_NOT_FOUND;
            i+=2;
            continue;
		}
		debug("found at 0x%08x\r\n", base + patch_loc);
		if(apply_patch(base + patch_loc, cur_patch) == ERR_MEM_WRITE)
			ret= ERR_MEM_WRITE;
		i+=2;
	}

	return ret;
}

/* write len bytes of data to patch starting at offset */
void ammend_patch(char *patch, void *data, int offset, int len) {
	char *patch_ptr = patch + offset;
	char *data_ptr = (char *)data;

	while(len > 0) {
		*patch_ptr = *data_ptr;
		patch_ptr++;
		data_ptr++;
		len--;
	}
}

#if ! defined USE_SRP3
/* Applies all patches to "Game.dll"
   See patches.h for more information */
int patch_game(char *base) {
	int hash_init_proc, do_hash_proc, logon_proof_hash_proc;
	int rel_call_addr;
	int create_account3_proc;
	int lph_checked_loc, create_account2_loc;
	t_patch addr_fix;

	debug("Searching for hash_init: ");
	hash_init_proc = (int) w3l_hash_init - (int) base;

	debug("found at 0x%08x\r\n", base + hash_init_proc);

	debug("Searching for do_hash: ");
    
	/*
    In version 1.1, do_hash function was moved to the library. In Game.dll there is indirect call to that function. 
    That's why we count difference between pointers. May cause unknown errors, if star_do_hash will be lesser than
    base.
	*/
	do_hash_proc = ( int) w3l_do_hash - ( int) base;

	debug("found at 0x%08x\r\n", base + do_hash_proc);

	/* Move this function to library; replace call from lph_checked to this one */
	debug("Searching for logon_proof_hash: ");
	logon_proof_hash_proc = (int) w3l_logon_proof_hash_rev - (int)base;
	if(logon_proof_hash_proc == -1) {
		debug("not found.\r\n");
		return ERR_SIG_NOT_FOUND;
	}
	debug("found at 0x%08x\r\n", base + logon_proof_hash_proc);

	/* using create_account3 from library */
	debug("Searching for create_account3: ");
	create_account3_proc = (int)w3l_create_account3 - (int)base;

	debug("found at 0x%08x\r\n", base + create_account3_proc);
	
	debug("Searching for LPH call: ");
	/* Find lph_checked location */
	lph_checked_loc = find_sig(base, &lph_verified_sig, GAME_DLL_SIZE);
	if (lph_checked_loc == -1) {
		debug("not found.\r\n");
		return ERR_SIG_NOT_FOUND;
	}

	debug("Searching for LPH_checked call1: ");
	/* Find lph_checked call location */
	lph_checked_loc = find_sig(base, &lph_verified_call1_sig, GAME_DLL_SIZE);
	if (lph_checked_loc == -1) {
		debug("not found.\r\n");
		return ERR_SIG_NOT_FOUND;
	}
	rel_call_addr = (int)w3l_lph_checked_rev - lph_checked_loc - 4 - (int)base;
	debug("Setting LPH_checked call addr at 0x%08x to 0x%08x\r\n", lph_checked_loc + base, rel_call_addr);
	addr_fix.length = 4;
	addr_fix.data = &(char *)rel_call_addr;
	addr_fix.name = "LPH_checked call1 address";
	if (apply_patch(base + lph_checked_loc, &addr_fix) == ERR_MEM_WRITE)
		return ERR_MEM_WRITE;

	debug("Searching for LPH_checked call2: ");
	/* Find lph_checked call location */
	lph_checked_loc = find_sig(base, &lph_verified_call2_sig, GAME_DLL_SIZE);
	if (lph_checked_loc == -1) {
		debug("not found.\r\n");
		return ERR_SIG_NOT_FOUND;
	}
	rel_call_addr = (int)w3l_lph_checked_rev - lph_checked_loc - 4 - (int)base;
	debug("Setting LPH_checked call addr at 0x%08x to 0x%08x\r\n", lph_checked_loc + base, rel_call_addr);
	addr_fix.length = 4;
	addr_fix.data = &(char *)rel_call_addr;
	addr_fix.name = "LPH_checked call2 address";
	if (apply_patch(base + lph_checked_loc, &addr_fix) == ERR_MEM_WRITE)
		return ERR_MEM_WRITE;

	/* The main createaccount patch is too big to be placed at the main patch location.
	   Instead, helper code redirects to the location. The patch is written right after
	   where the logonproofhash patch is written 
	   TODO: ammend create_account2_patch_data to call create_account3_loc instead
	   of hardcoded location OR redo create_account patches so relocation is not needed */
	debug("Searching for create_account2 patch: ");
	create_account2_loc = find_sig(base, &create_account2_sig, GAME_DLL_SIZE);
	debug("found at 0x%08x\r\n", base + create_account2_loc);
	
	create_account3_proc = (int)w3l_create_account3 - (int)base;
	rel_call_addr = create_account3_proc - create_account2_loc - 5;
	debug("Setting create_account3 call addr at 0x%08x to 0x%08x\r\n", create_account2_loc + base, rel_call_addr);
	ammend_patch(create_account2_patch_data, &rel_call_addr, 1, 4);
	
	if(apply_patch(base + create_account2_loc, &create_account2_patch) == ERR_MEM_WRITE)
		return ERR_MEM_WRITE;

	/* apply the other patches */
	return apply_patches(base, game_patches, GAME_DLL_SIZE);
}
#endif

/* Loads the real "Game.dll" and gets the address of the real "GameMain" function
   Applies patches */
int patch() {
	int rval;
	app_process = GetCurrentProcess();
#ifdef WIN98
    game_dll_base = LoadLibraryA(GAME_DLL_NAME);
    debug("LoadLibraryA",game_dll_base);
#else
    game_dll_base = LoadLibrary(GAME_DLL_NAME);
#endif
	if(!game_dll_base) {
		MessageBox(NULL, GAME_DLL_LOAD_ERROR, "[lib] Patch Error (w3lh.dll)", MB_OK);
		return ERR_GAME_DLL_LOAD;
	}
	debug("base: 0x%08X\r\n", game_dll_base);

	real_game_main = GetProcAddress(game_dll_base, GAME_MAIN);
	if(!real_game_main) {
		MessageBox(NULL, GAME_MAIN_ERROR, "[proc] Patch Error (w3lh.dll)", MB_OK);
		return ERR_GAME_MAIN;
	}
    /* apply delay reduction patches and adRemove patch */
	delay_reducer_patch_data[0] = read_latency( );
    debug( "Latency set to: %hu ms\r\n", ( unsigned char)(delay_reducer_patch_data[0]));
    apply_patches((char *)game_dll_base, unimportant_patches, GAME_DLL_SIZE);

	#if ! defined USE_SRP3
	rval = patch_game((char *)game_dll_base);
	#else
	rval = apply_patches((char *)game_dll_base, game_patches_srp3, GAME_DLL_SIZE);
	#endif
	if(rval == ERR_SIG_NOT_FOUND)
		MessageBox(NULL, SIG_NOT_FOUND_ERROR, "Patch Error (w3lh.dll)", MB_OK);
	else if(rval == ERR_MEM_WRITE)
		MessageBox(NULL, MEMORY_WRITE_ERROR, "Patch Error (w3lh.dll)", MB_OK);

	return rval;

}

/* DLL entry point - called when war3.exe calls LoadLibrary. 
   Patches Game.dll memory on attach before GameMain is called.
   Also called when war3.exe calls FreeLibrary. Intercepts the war3.exe FreeLibrary 
   call and frees the real "Game.dll" */
BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	if(fdwReason == DLL_PROCESS_DETACH && called == 1 && game_dll_base) { /* FreeLibrary called */
		called = 2; /* prevent multiple FreeLibrary calls */
	}

	if(called) /* prevent multiple calls to this function */
		return TRUE;
	called = 1;

	debug("Start DllMain.\r\n");
    if ( SetCurrentProcessDEP( DEP_DISABLED)) 
	    debug ( "DEP disabled\r\n");
    else
        debug ( "DEP can't be disabled\r\n");

	return TRUE;
}

DLLEXPORT void applyPatches(struct workerOptsSt *options) {
	debug("Worker options: 0x%X\r\n", options);
}
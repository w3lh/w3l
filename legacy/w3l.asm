[BITS 32]

%include "win32n.inc"
%define PAGE_READWRITE 4

extern __imp_ReadProcessMemory
extern __imp_WriteProcessMemory
extern __imp_TerminateProcess
extern __imp_MessageBoxA
extern __imp_ExitProcess
extern __imp_GetStartupInfoA
extern __imp_CreateProcessA
extern __imp_GetCommandLineA
extern __imp_GetCurrentProcess
extern __imp_ResumeThread
extern __imp_GetThreadContext

%include "defines.asm"

segment code public use32 class='CODE'

patch_err:	push	byte 1
		push	dword [processinfo]
		call	[__imp_TerminateProcess]
		push	dword MB_OK
		push	dword err
		push	dword gameerr
		push	byte 0
		call	[__imp_MessageBoxA]
		push	byte 3
		call	[__imp_ExitProcess]
		
started_old:	mov	ebx,[processinfo]	;hProcess

		push	dword numread
		push	byte 4
		push	dword buf
		push	dword GAME_DLL_OFF_OLD
		push	ebx
		call	[__imp_ReadProcessMemory]
		test	eax,eax
		jz	near patch_err
	
		mov	eax,[buf]
		cmp	eax,[gamedll]
		jnz	near patch_err		

		push	dword numread
		push	byte 4
		push	dword helper_dll
		push	dword GAME_DLL_OFF_OLD
		push	ebx
		call	[__imp_WriteProcessMemory]
		test	eax,eax
		jz	near patch_err
		jmp	near .finish		

..start		push	dword startupinfo
		call	[__imp_GetStartupInfoA]
		call	[__imp_GetCommandLineA]
		push	dword processinfo
		push	dword startupinfo
		xor	edx,edx
		push	edx
		push	edx
		push	byte CREATE_SUSPENDED
		push	edx
		push	edx
		push	edx
		push	eax
		push	dword war3exe
		call	[__imp_CreateProcessA]
		test	eax,eax
		jnz	.started

.starterr	push	byte MB_OK
		push	dword err
		push	dword war3err
		push	byte 0
		call	[__imp_MessageBoxA]
		push	byte 2
		call	[__imp_ExitProcess]

.started	mov	ebx,[processinfo]	;hProcess

		push	dword numread
		push	byte 4
		push	dword buf
		push	dword GAME_DLL_OFF
		push	ebx
		call	[__imp_ReadProcessMemory]
		test	eax,eax
		jz	near patch_err
	
		mov	eax,[buf]
		cmp	eax,[gamedll]
		jnz	near started_old

		push	dword numread
		push	byte 4
		push	dword helper_dll
		push	dword GAME_DLL_OFF
		push	ebx
		call	[__imp_WriteProcessMemory]
		test	eax,eax
		jz	near patch_err

		;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.finish		mov	edi,[processinfo+4]		;hThread
		push	edi
		call	[__imp_ResumeThread]

		push	byte 0
		call	[__imp_ExitProcess]
		
segment data public use32 class='DATA'
err		db	"Error",0
align 4,db 0
gamedll		db	"Game.dll",0
gameerr	db	"Wrong War3.exe version. (need "
		db	WC3_VERSION
		db	")",0
war3err		db	"Could not start War3.exe! Make sure the loader is in the same dir.",0

war3exe		db	"War3.exe",0
helper_dll	db	HELPER_DLL_NAME

align 4,db 0

hand		dd	0
numread		dd	0
buf		times	4	db 0
processinfo:
startupinfo	times	68	db 0

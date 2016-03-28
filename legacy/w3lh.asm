; link with
; alink -oPE -base 0x13370000 -dll w3lh win32.lib

;ugly+slow code to write debug info to a file
;enable this only to find out what patches failed for future war3 versions
%define DEBUG

;replace war3 password hash with starcraft-compatible hash for pvpgn
;%define PVPGN
%define SWZ

%include "defines.asm"
%include "win32n.inc"
	
extern __imp_LoadLibraryA
extern __imp_GetProcAddress
extern __imp_GetCurrentProcess
extern __imp_ReadProcessMemory
extern __imp_WriteProcessMemory
extern __imp_MessageBoxA
extern __imp_ExitProcess
%ifdef DEBUG
extern __imp_CreateFileA
extern __imp_WriteFile
extern __imp_CloseHandle
extern __imp_SetFilePointer
extern __imp_GetTickCount
extern __imp_wsprintfA
%endif

segment data public use32 class=DATA
segment code public use32 class=CODE

%macro debug 2+
%ifdef DEBUG
segment data
%ifidn %1,0
%%str		db	%2,13,10
%else
%%str		db	%2," - "
%endif
%%strlen	equ	$-%%str

segment code
		pusha

%ifnidn %1,0
		push	dword %1		;for wsprintf
%endif

		push	byte 0
		push	dword FILE_ATTRIBUTE_NORMAL
		push	dword OPEN_ALWAYS
		push	byte 0
		push	byte 0
		push	dword GENERIC_WRITE
		push	dword debugfile
		call	[__imp_CreateFileA]
		xchg	ebp,eax

		push	dword FILE_END
		push	byte 0
		push	byte 0
		push	ebp
		call	[__imp_SetFilePointer]

		push	byte 0
		push	dword temp 
		push	dword %%strlen		;size
		push	dword %%str
		push	ebp			;handle
		call	[__imp_WriteFile]

%ifnidn %1,0
		push	dword fmtstr
		push	dword offsetstr
		call	[__imp_wsprintfA]
		pop	ecx
		pop	ecx
		pop	ecx

		push	byte 0
		push	dword temp 
		push	byte 10
		push	dword offsetstr
		push	ebp			;handle
		call	[__imp_WriteFile]
%endif

		push	ebp
		call	[__imp_CloseHandle]

		popa
%endif
%endmacro

chainback:	jmp 	[__imp_LoadLibraryA]
chainbacklen equ $-chainback

global GameMain
export GameMain
GameMain:	mov	eax,[gamebase]		;ALL YOUR BASE ARE BELONG TO US.
		push	eax
		call	[real_gamemain]
		ret	4


;DLL entry point
;This is a STDCALL entrypoint, so remove 3 params from stack on return
..start:	
dllstart:	cmp	dword [already],byte 0
		jne	.bye

		mov	byte [already],1
		call	Hack

.bye		push	byte 1
		pop	eax
		ret	12

Hack		pusha

		debug	0,"start"

		call	[__imp_GetCurrentProcess]
		xchg	ebp,eax			;ebp=hProcess

		push	dword game_dll
		call	[__imp_LoadLibraryA]
		test	eax,eax
		jz	.bye
		mov	[gamebase],eax
		xchg	ebx,eax

		push	dword gamemain_str
		push	ebx
		call	[__imp_GetProcAddress]
		test	eax,eax
		jz	.bye
		mov	[real_gamemain],eax


		mov	edx,ebx			;edx = base
		call	PatchGame
		mov	edx,WAR3_BASE
		call	PatchWar3

		debug	0,"patches successful."

.bye		popa
		ret


patch_err:	push	dword MB_OK
		push	dword err
		push	dword gameerr
		push	byte 0
		call	[__imp_MessageBoxA]
		push	byte 3
		call	[__imp_ExitProcess]
		ret


;sig format:
;db sigsize
;db m,0,m,1,m,2 ...	;mask & signature (sigsize*2 bytes) here
;db offset_delta	;this is added to start offset where sig is found

;param: edx = base, esi = sig
;ret: eax = offset, esi = after sig
FindSig		push	ebx
		push	ecx

		mov	ecx,[sigscan_size]
		mov	eax,edx

.search		movzx	ebx,byte [esi]		;ebx = sigsize
		dec	ebx
		inc	eax
		dec	ecx
		jz	.notfound
.nextloc	push	ecx
		mov	cl,[esi+1+ebx*2]	;mask
		and	cl,[eax+ebx]		;memloc
		cmp	cl,[esi+1+ebx*2+1]	;sigval
		pop	ecx
		jne	.search
		dec	ebx
		jns	.nextloc

		movzx	ebx,byte [esi]
		lea	esi,[esi+1+ebx*2]
		movsx	ebx,byte [esi]
		add	eax,ebx
		inc	esi
		jmp	short .bye

.notfound	xor	eax,eax
		movzx	ebx,byte [esi]
		lea	esi,[esi+1+ebx*2+1]

.bye		pop	ecx
		pop	ebx
		ret



;patch format:
;db patch_size
;db 0,1,2,3,4,5				;patch (patch_size bytes)

;param: eax = offset, esi = patch
;ret: eax = status, esi = after patch
DoPatch:	push	ebx
		xor	ebx,ebx
		mov	bl,[esi]
		inc	esi

		debug	eax,"DoPatch"

		pusha

		push	dword temp		;num written (ignored)
		push	ebx			;patch size
		push	esi			;patch data
		push	eax			;mem offset
		push	ebp			;hProcess

		call	[__imp_WriteProcessMemory]

		mov	[temp],eax		;save status
		popa

		mov	eax,[temp]

		add	esi,ebx
		pop	ebx
		ret

;param: edx = dll base, ebp = hProcess
PatchWar3:	pusha

		mov	edx,WAR3_BASE
		mov	[sigscan_size],dword WAR3_SIZE

		mov	esi,war3_patchdata

		jmp	PatchGame.nextpatch

;param: edx = dll base, ebp = hProcess
PatchGame:	pusha

		;--- special patches

		mov	[sigscan_size],dword GAME_SIZE

		mov	esi,game_patchdata

%ifdef PVPGN
		call	FindSig
		test	eax,eax
		jz	patch_err
		mov	[hash_init_off],eax
		debug	eax,"hash_init_off"

		call	FindSig
		test	eax,eax
		jz	patch_err
		mov	[do_hash_off],eax
		debug	eax,"do_hash_off"

		call	FindSig
		test	eax,eax
		jz	patch_err
		debug	eax,"logonproofhash"
		mov	ebx,[hash_init_off]
		sub	ebx,eax
		add	[hash_init_call+1],ebx
		mov	ebx,[do_hash_off]
		sub	ebx,eax
		add	[do_hash_call+1],ebx
		call	DoPatch
		test	eax,eax
		jz	patch_err

%endif
%ifdef SWZ
		call	FindSig
		test	eax,eax
		jz	patch_err
		mov	[hash_init_off],eax
		debug	eax,"hash_init_off"

		call	FindSig
		test	eax,eax
		jz	patch_err
		mov	[do_hash_off],eax
		debug	eax,"do_hash_off"

		call	FindSig
		test	eax,eax
		jz	patch_err
		debug	eax,"logonproofhash"
		mov	ebx,[hash_init_off]
		sub	ebx,eax
		add	[hash_init_call+1],ebx
		mov	ebx,[do_hash_off]
		sub	ebx,eax
		add	[do_hash_call+1],ebx
		call	DoPatch
		test	eax,eax
		jz	patch_err

%endif

		;following patches need no special treatment; sig/patch pairs
.nextpatch:	cmp	byte [esi],0
		jz	.bye

		call	FindSig
		test	eax,eax
		jz	near patch_err
		call	DoPatch
		test	eax,eax
		jnz	.nextpatch

.bye 		popa
		ret



;========================================================================

segment data
err		db	"Error",0
game_dll	db	"Game.dll",0
gameerr		db	"Wrong Game.dll version. (need "
		db	WC3_VERSION
		db	")",0

gamemain_str	db	"GameMain",0


%ifdef DEBUG
debugfile	db	"debug.log",0

offsetstr 	db	"00000000",13,10,0
fmtstr		db	"%08x",13,10,0
%endif

align 4

already		dd	0

buf		dd	0
numread		dd	0

sigscan_size	dd	0
temp		dd	0

hash_init_off	dd	0
do_hash_off	dd	0

real_gamemain	dd	0
gamebase	dd	0

;------------------------------------------------------------------------


war3_patchdata:	;--- end
		db 0

;GAME.DLL

game_patchdata:	
%ifdef PVPGN
		;find hash_init
		db 7
		db 0xff,0x90, 0xff,0xc7, 0xff,0x01, 0xff,0x01, 0xff,0x23
		db 0xff,0x45, 0xff,0x67
		db 1

		;find do_hash
		db 19
		db 0xff,0x55, 0xff,0x8b, 0xff,0xec, 0xff,0x81, 0xff,0xec
		db 0xff,0x50, 0xff,0x01, 0xff,0x00, 0xff,0x00, 0xff,0x53
		db 0xff,0x56, 0xff,0x8b, 0xff,0xd9, 0xff,0x57, 0xff,0x8d
		db 0xff,0x73, 0xff,0x14, 0xff,0xb9, 0xff,0x10
		db 0

		;logonproofhash
		db 12
		db 0xff,0x83, 0xff,0xec, 0xff,0x2c, 0xff,0x53, 0xff,0x56
		db 0xff,0x57, 0xff,0x8b, 0xff,0xf9, 0xff,0x8b, 0xff,0x4d
		db 0xff,0x10, 0xff,0xba
		db 0,92
logonproofhash:	pusha
		sub	esp,54h
		mov	ecx,esp
hash_init_call:	call	logonproofhash
		push	byte 40h
		pop	ecx
		lea	edi,[esp+14h]
		mov	edx,edi
		sub	eax,eax
		repe	stosb
		mov	edi,edx
		mov	ebx,[ebp+8]
		lea	esi,[ebx+20h]
		push	10h
		pop	ecx
		push	ecx
		repe movsb
		pop	ecx
		dec	edx

lcase:		inc	edx
		dec	ecx
		js	short lcasedone
		mov	al,[edx]
		cmp	al,41h
		jb	short lcase
		cmp	al,5Ah
		ja	short lcase
		or	byte [edx],20h
		jmp	short lcase
; 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�

lcasedone:
		mov	ecx, esp
		push	ecx
		push	ebx
do_hash_call:	call	logonproofhash
		pop	edi
		pop	esi
		add	edi,0A8h
		push	5
		pop	ecx
		repe	movsd
		add	esp,54h
		popa
		pop	ebp
		sub	eax,eax
		inc	eax
		retn	0Ch

; 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�

%endif ;PVPGN
%ifdef SWZ
		;find hash_init
		db 7
		db 0xff,0x90, 0xff,0xc7, 0xff,0x01, 0xff,0x01, 0xff,0x23
		db 0xff,0x45, 0xff,0x67
		db 1

		;find do_hash
		db 19
		db 0xff,0x55, 0xff,0x8b, 0xff,0xec, 0xff,0x81, 0xff,0xec
		db 0xff,0x50, 0xff,0x01, 0xff,0x00, 0xff,0x00, 0xff,0x53
		db 0xff,0x56, 0xff,0x8b, 0xff,0xd9, 0xff,0x57, 0xff,0x8d
		db 0xff,0x73, 0xff,0x14, 0xff,0xb9, 0xff,0x10
		db 0

		;logonproofhash
		db 12
		db 0xff,0x83, 0xff,0xec, 0xff,0x2c, 0xff,0x53, 0xff,0x56
		db 0xff,0x57, 0xff,0x8b, 0xff,0xf9, 0xff,0x8b, 0xff,0x4d
		db 0xff,0x10, 0xff,0xba
		db 0,92
logonproofhash:	pusha
		sub	esp,54h
		mov	ecx,esp
hash_init_call:	call	logonproofhash
		push	byte 40h
		pop	ecx
		lea	edi,[esp+14h]
		mov	edx,edi
		sub	eax,eax
		repe	stosb
		mov	edi,edx
		mov	ebx,[ebp+8]
		lea	esi,[ebx+20h]
		push	10h
		pop	ecx
		push	ecx
		repe movsb
		pop	ecx
		dec	edx

lcase:		inc	edx
		dec	ecx
		js	short lcasedone
		mov	al,[edx]
		cmp	al,41h
		jb	short lcase
		cmp	al,5Ah
		ja	short lcase
		or	byte [edx],20h
		jmp	short lcase
; 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�

lcasedone:
		mov	ecx, esp
		push	ecx
		push	ebx
do_hash_call:	call	logonproofhash
		pop	edi
		pop	esi
		add	edi,0A8h
		push	5
		pop	ecx
		repe	movsd
		add	esp,54h
		popa
		pop	ebp
		sub	eax,eax
		inc	eax
		retn	0Ch

; 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�

%endif ;SWZ

areqacpt_0:	db 15
		db 0xff,0x3b, 0xff,0xfe, 0xff,0x75, 0xff,0x0b, 0xff,0x5f
		db 0xff,0x5e, 0xff,0x33, 0xff,0xc0, 0xff,0x5b, 0xff,0x8b
		db 0xff,0xe5, 0xff,0x5d, 0xff,0xc2, 0xff,0x10, 0xff,0x00
		db 2,1
		db 0xeb

areqacpt_1:	db 15
		db 0xff,0x83, 0xff,0xbd, 0xff,0x44, 0xff,0xfd, 0xff,0xff
		db 0xff,0xff, 0xff,0x02, 0xff,0x74, 0xff,0x0b, 0xff,0x5f
		db 0xff,0x5e, 0xff,0x33, 0xff,0xc0, 0xff,0x5b, 0xff,0x8b
		db 7,1
		db 0xeb


areqacpt_2:	db 16
		db 0xff,0xb5, 0xff,0x60, 0xff,0xff, 0xff,0xff, 0xff,0xff
		db 0xff,0x33, 0xff,0xd2, 0x00,0x00, 0x00,0x00, 0x00,0x00
		db 0x00,0x00, 0xff,0xff, 0xff,0xff, 0x00,0x00, 0x00,0x00
		db 0xff,0x74
		db 15,1
		db 0xeb


logonproofver:	db 9
		db 0xff,0xf3, 0xff,0xa7, 0xff,0x5f, 0xff,0x0f, 0xff,0x94
		db 0xff,0xc0, 0xff,0x5e, 0xff,0x8b, 0xff,0xe5
		db 3,3
		sub	eax,eax
		inc	eax

acccreatechange	db 10
		db 0xff,0x8b, 0xff,0xf8, 0xff,0x8d, 0xff,0x45, 0xff,0x0c
		db 0xff,0x50, 0xff,0x8d, 0xff,0x4d, 0xff,0x10, 0xff,0x51
		db 0,14h
		db 0x89,0xde,0x81,0xc6,0x10,0x01,0x00,0x00
		db 0xb9,0x08,0x00,0x00,0x00,0x8b,0x7d,0x14
		db 0xf3,0xa5,0xeb,0x1b

fix_desync_1:	db 13
		db 0xff,0x51, 0xff,0xb9, 0x00,0x00, 0x00,0x00, 0x00,0x00
		db 0xff,0x6f, 0xff,0xe8, 0x00,0x00, 0x00,0x00, 0x00,0x00
		db 0x00,0x00, 0xff,0xb8, 0xff,0x01
		db 6,3
		pop ecx
		jmp short $+4

		;--- end
		db 0


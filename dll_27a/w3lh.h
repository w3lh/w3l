#if ! defined(_W3LH_H)
#define _W3LH_H

#include "pstdint.h"
#include "functions.h"

#define DLLEXPORT __declspec(dllexport)
#ifndef USE_SRP3
DLLEXPORT void w3l_do_hash(char *username, bnet_hash_ctx *ctx);
DLLEXPORT int __stdcall w3l_logon_proof_hash_rev(char *arg1, void *arg2, void *arg3);
DLLEXPORT int __fastcall w3l_lph_checked_rev(int *a1, int *a2, void *a3, void *a4);
DLLEXPORT int w3l_create_account3();
#endif
DLLEXPORT void __fastcall w3l_hash_init(uint32_t *data);

#endif /* _W3LH_H */

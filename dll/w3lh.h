#if ! defined(_W3LH_H)
#define _W3LH_H

#include "pstdint.h"
#include "functions.h"

#define DLLEXPORT __declspec(dllexport)
#ifndef USE_SRP3
DLLEXPORT void w3l_do_hash(char *username, bnet_hash_ctx *ctx);
#endif
DLLEXPORT void __fastcall w3l_hash_init(uint32_t *data);

#endif /* _W3LH_H */

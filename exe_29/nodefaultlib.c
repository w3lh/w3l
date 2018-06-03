#include "nodefaultlib.h"

void * __cdecl memset(void *pTarget, int value, size_t cbTarget)
{
	unsigned char *p = (unsigned char *)(pTarget);
	while (cbTarget-- > 0) {
		*p++ = (unsigned char)(value);
	}
	return pTarget;
}
int __cdecl memcmp(void const* _Buf1, void const* _Buf2, size_t _Size)
{
	unsigned char u1, u2;
	unsigned char *b1 = (unsigned char *)_Buf1;
	unsigned char *b2 = (unsigned char *)_Buf2;

	for (; _Size--; b1++, b2++) {
		u1 = *b1;
		u2 = *b2;
		if (u1 != u2) {
			return (u1 - u2);
		}
	}
	return 0;
}
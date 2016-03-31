#include "nodefaultlib.h"
#define DEBUG_FILE L"debug.log"

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

/* Print a log message */
void debug(char *message, ...) {
#ifdef DEBUG
	DWORD temp;
	HANDLE file;
	va_list args;
	char buf[1024];

	memset(buf, 0, sizeof(buf));
	va_start(args, message);
	wvsprintfA(buf, message, args);

	file = CreateFile(DEBUG_FILE, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	SetFilePointer(file, 0, 0, FILE_END);
	WriteFile(file, buf, (DWORD)strlen(buf), &temp, NULL);
	CloseHandle(file);
#endif
}
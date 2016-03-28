nasmw -O 42 -f obj w3l.asm
alink w3l -oPE win32.lib rsrc.res

nasmw -O 42 -f obj w3lh.asm
alink -oPE -base 0x13370000 -dll w3lh win32.lib

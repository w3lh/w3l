w3l - Warcraft III RoC/TFT 1.22a+ PvPGN loader (source code)

This software patches Warcraft III to enable connections to PvPGN servers.  It was written in part by:
  * phatdeeva (w3lh)
  * Rupan (w3l, w3lh)
  * Keres
  * many others

This program is Free Software distributed under the GNU GPL, version 3.  For the full terms of the license refer to license.txt in the root of the source distribution.

1. Compiling

Build w3l.c as an exe. w3lh.c must be built as a DLL. Required exports are contained in the w3lh.def file. You may need to change the base address of w3lh.dll when linking.

By default, VC++ links to its C runtime library dynamically.  It may be required that this not occur, i.e. in case your target audience does not have or does not want to install the Visual C++ runtime libraries.  In this case it is necessary to perform a static link with the C runtime.  Know that doing so will cause the generated code to become quite a bit larger.  Here is where you can do so:

Configuration Properties
 -> C/C++
     -> Code Generation
         -> Runtime Library

It may also be advantageous to change the default base address of your DLL or EXE.  You can do so here:

Configuration Properties
 -> Linker
     -> Advanced
         -> Base Address

The default image base for an executable is 0x400000; for a DLL it is 0x10000000.  The range for a valid base address is (in hex notation) is from 0x1000000 to 0x80000000.  In addition, the base address must be on a multiple of 64K (0x10000).  Some good starting values may be 0x7E540000 for w3l and 0x76290000 for w3lh.

1a. What the heck is "SRP3"?

SRP3 stands for "Secure Remote Password Protocol", version 3.  Technical details can be found here:
  * http://tools.ietf.org/rfc/rfc2945.txt
  * http://en.wikipedia.org/wiki/Secure_remote_password_protocol
In previous versions of the loader, the native Warcraft III hashing code was replaced by an older, different hashing routine taken from Starcraft.  This, however, added complexity to the loader making it more difficult to maintain.  Unpatched copies of Warcraft 3 already use SRP3 to authenticate to the real battle.net, but until recently PvPGN did not support this type of authentication.  As of PvPGN 1.99-svn this support is available.  Using it requires that w3lh be rebuilt with USE_SRP3 defined.  Keep in mind that existing accounts created with the old-style password hashes are incompatible with an SRP3-enabled client.

2. Running

Place both w3l.exe and w3lh.dll into your Warcraft III directory.
Run 'w3l.exe' when you want to connect to a PvPGN server.
Run 'war3.exe' when you want to connect to battle.net.

Notes:
  * To play RoC when you have TFT expansion installed, run 'w3l.exe -classic'
  * To play in windowed mode, run 'w3l.exe -window'
  * If you connect to battle.net while running w3l.exe, you risk getting your cd-key banned.
  * If there are problems, it is probably due to a missing patch. See patches.h for more information.

Standard disclaimers:
  * This is not intended as a piracy tool! Make sure you have bought a copy of Warcraft III before using this loader. Even then you may be breaking the law. If in doubt, do without!
  * This loader will not work for versions below 1.22a. Use the ACiD loader v1.2 for previous versions. It may work for versions higher than 1.22a but that is not guaranteed.
  * Use at your own risk. If you find any problems, fix them ;)

The included release is built with the "old" Starcraft-style hash code.  It should work with existing accounts.  If you want SRP3 support you'll need to rebuild the DLL with USE_SRP3 enabled in patches.h.

3. Enjoy!

4. Thanks go out to

  * ACiD for their 1.18a patch and source
  * gr11x for the initial work on the new patch
  * Rupan for the C port of w3l.asm, the posting of the ACiD patch source, and various fixes to the source
  * PvPGN community for help
  * the PvPGN and bnetd contributors
  * Ken Saunders and mouserunner.net for icon
  * ReservoirDOG for beta-testing
  * Google Chrome developers for DEP disabling method
  * Blizzard for making such a fantastic game

Thanks to Keres on the PvPGN forums for releasing fixes and updates, and to all who helped test and debug this release there.

5. TODO

  * include patch and sig asm instructions

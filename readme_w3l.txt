w3l - Warcraft III RoC/TFT 1.22a+ PvPGN loader (binary)
Legal note: 
This is not intended as a piracy tool! Make sure you have bought a copy of Warcraft III before using this loader. Even then you may be breaking the law. If in doubt, do without! 
This loader will not work for versions below 1.22a. Use the ACiD loader v1.2 for previous versions. It may work for versions higher than 1.27a but that is not guaranteed. 
Use at your own risk. 

Using: 
Place w3l.exe, w3lh.dll and wl27.dll into your Warcraft III directory. 
Run 'w3l.exe' when you want to connect to a PvPGN server. 
Run 'war3.exe' when you want to connect to battle.net. 
You can set delay value in file latency.txt. The proper delay value lies between 30 and 255. If wrong value is set or file is missed, delay value will be set to 100 ms. Default Blizzard's value is 250.

Notes: 
To play RoC when you have TFT expansion installed, run 'w3l.exe -classic' 
To play in windowed mode, run 'w3l.exe -window' 
If you connect to battle.net while running w3l.exe, you risk getting your cd-key banned. 


Thanks: 
Thanks to ACiD for their 1.18a patch and source, to phatdeeva and gr11x for the initial work on the new patch, to Rupan for the C port of w3l.asm, the posting of the ACiD patch source, and various fixes to the source, to the PvPGN and bnetd contributors, and to Blizzard for making such a fantastic game. 
Thanks to Keres on the PvPGN forums for releasing fixes and updates, and to all who helped test and debug this release there. Thanks to Google Chrome developers for DEP disabling method. Thanks to Phoenicks on the Bored Aussie forums for ad blocking patch and 1.25b game.dll offset.

Latest releases of w3l @ http://keres.myftp.org
PvPGN forums @ http://pelish.spfree.net/

Changelog:
 Version 1.4.2.0
   * Added 1.27b version support

 Version 1.4.1.0
   * Allows connect to official BNet through GProxy

 Version 1.4.0.1
   * Remove tons of CRT code to reduce executable size

 Version 1.4
   * Added 1.27a version support

 Version 1.3
   * Fixed 1.25b patch bug
   * Some technical improvements by Rupan

 Version 1.2
   * Fixed DEP bug with wc3 1.24 and Windows 7.

 Version 1.1.1
   * Blocking ad requests
   * Fixed latency bug in SRP3 version

 Version 1.1
   * Info updated

 Version 1.1 beta3
   * Disabling DEP protection

 Version 1.1 beta2
   * Delay reduction can be controlled by file latency.txt

 Version 1.1 beta1
   * Fixed 1.23a patch bugs
   * Add delay reduction
  
 Version 1.0
   * All known bugs fixed
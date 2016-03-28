/*
    w3l: a PvPGN loader for Warcraft 3 1.22+
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
#define DEBUG_FILE L"debug.log"
#include "dep.h"
#include <windows.h>
#include <stdio.h>

void debug(char *message, ...) {
#ifdef DEBUG
	DWORD temp;
	HANDLE file;
	va_list args;
	char buf[1024];

	memset(buf, 0, sizeof(buf));
	va_start(args, message);
	vsnprintf(buf, sizeof(buf) - 1, message, args);

	file = CreateFile((LPCWSTR)DEBUG_FILE, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	SetFilePointer(file, 0, 0, FILE_END);
	WriteFile(file, buf, (DWORD)strlen(buf), &temp, NULL);
	CloseHandle(file);
#endif
}

namespace {  
  
// These values are in the Windows 2008 SDK but not in the previous ones. Define  
// the values here until we're sure everyone updated their SDK.  
#ifndef PROCESS_DEP_ENABLE  
#define PROCESS_DEP_ENABLE                          0x00000001  
#endif  
#ifndef PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION  
#define PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION     0x00000002  
#endif  
  
// SetProcessDEPPolicy is declared in the Windows 2008 SDK.  
typedef BOOL (WINAPI *FnSetProcessDEPPolicy)(DWORD dwFlags);  
  
// Completely undocumented from Microsoft. You can find this information by  
// disassembling Vista's SP1 kernel32.dll with your favorite disassembler.  
enum PROCESS_INFORMATION_CLASS {  
  ProcessExecuteFlags = 0x22,  
};  
  
// Flags named as per their usage.  
const int MEM_EXECUTE_OPTION_ENABLE = 1;  
const int MEM_EXECUTE_OPTION_DISABLE = 2;  
const int MEM_EXECUTE_OPTION_ATL7_THUNK_EMULATION = 4;  
const int MEM_EXECUTE_OPTION_PERMANENT = 8;  
  
// Not exactly the right signature but that will suffice.  
typedef HRESULT (WINAPI *FnNtSetInformationProcess)(  
    HANDLE ProcessHandle,  
    PROCESS_INFORMATION_CLASS ProcessInformationClass,  
    PVOID ProcessInformation,  
    ULONG ProcessInformationLength);  
  
}  // namespace  
  
extern "C"
BOOL SetCurrentProcessDEP(enum DepEnforcement enforcement) {  
  // Try documented ways first.  
  // Only available on Vista SP1 and Windows 2008.  
  // http://msdn.microsoft.com/en-us/library/bb736299.aspx  
  FnSetProcessDEPPolicy SetProcDEP =  
      reinterpret_cast<FnSetProcessDEPPolicy>(  
          GetProcAddress(GetModuleHandle(L"kernel32.dll"),  
                                         "SetProcessDEPPolicy"));  
  
  if (SetProcDEP) {  
	    debug ( "SetProcDEP used\r\n");
    ULONG dep_flags;  
    switch (enforcement) {  
      case DEP_DISABLED:  
        dep_flags = 0;  
        break;  
      case DEP_ENABLED:  
        dep_flags = PROCESS_DEP_ENABLE |  
                    PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION;  
        break;  
      case DEP_ENABLED_ATL7_COMPAT:  
        dep_flags = PROCESS_DEP_ENABLE;  
        break;  
      default:  
        return false;  
    }  
    if( SetProcDEP(dep_flags)) return 1;  
  }  
  
  // Go in darker areas.  
  // Only available on Windows XP SP2 and Windows Server 2003 SP1.  
  // http://www.uninformed.org/?v=2&a=4  
  FnNtSetInformationProcess NtSetInformationProc =  
      reinterpret_cast<FnNtSetInformationProcess>(  
          GetProcAddress(GetModuleHandle(L"ntdll.dll"),  
                                         "NtSetInformationProcess"));  
  
  if (!NtSetInformationProc) 
  {
	  debug ( "NtSetInformationProc failed\r\n");
    return false;  
  }
  // Flags being used as per SetProcessDEPPolicy on Vista SP1.  
  ULONG dep_flags;  
  switch (enforcement) {  
    case DEP_DISABLED:  
      // 2  
      dep_flags = MEM_EXECUTE_OPTION_DISABLE;  
      break;  
    case DEP_ENABLED:  
      // 9  
      dep_flags = MEM_EXECUTE_OPTION_PERMANENT | MEM_EXECUTE_OPTION_ENABLE;  
      break;  
    case DEP_ENABLED_ATL7_COMPAT:  
      // 0xD  
      dep_flags = MEM_EXECUTE_OPTION_PERMANENT | MEM_EXECUTE_OPTION_ENABLE |  
                  MEM_EXECUTE_OPTION_ATL7_THUNK_EMULATION;  
      break;  
    default:  
      return false;  
  }  
  debug ( "NtSetInformationProc used\r\n");
  
  HRESULT status = NtSetInformationProc(GetCurrentProcess(),  
                                        ProcessExecuteFlags,  
                                        &dep_flags,  
                                        sizeof(dep_flags));  
  return SUCCEEDED(status);  
}  


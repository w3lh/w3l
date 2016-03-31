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
#ifndef DEP_H__
#define DEP_H__
#include <windows.h>
#include "nodefaultlib.h"
enum DepEnforcement {
  // DEP is completely disabled.
  DEP_DISABLED,
  // DEP is permanently enforced.
  DEP_ENABLED,
  // DEP with support for ATL7 thunking is permanently enforced.
  DEP_ENABLED_ATL7_COMPAT,
};

// Change the Data Execution Prevention (DEP) status for the current process.
// Once enabled, it cannot be disabled.
 #ifdef __cplusplus
 extern "C" {
 #endif 
BOOL SetCurrentProcessDEP(enum DepEnforcement enforcement);
 #ifdef __cplusplus
 }
 #endif 
#endif  // DEP_H__

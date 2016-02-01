;=============================================================================
; @file globals.inc
;
; Global variables used by boot loaders.
;
; Copyright 2016 Brett Vickers.
; Use of this source code is governed by a BSD-style license that can
; be found in the LICENSE file.
;=============================================================================

%ifndef __MONK_BOOT_GLOBALS_INC__
%define __MONK_BOOT_GLOBALS_INC__

%include "include/mem.inc"

;=============================================================================
; Global variables (stored at Mem.Boot.Globals)
;=============================================================================
struc Globals, Mem.Globals
    .DriveNumber            resd    1
    .RootDirectorySector    resw    1
    .KernelSector           resw    1
    .KernelSize             resd    1
endstruc

%endif ; __MONK_BOOT_GLOBALS_INC__

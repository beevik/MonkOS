;=============================================================================
; @file bios.inc
;
; BIOS constants and structures, usable in real mode only.
;
; Copyright 2016 Brett Vickers.
; Use of this source code is governed by a BSD-style license that can
; be found in the LICENSE file.
;=============================================================================

%ifndef __MONK_BOOT_BIOS_INC__
%define __MONK_BOOT_BIOS_INC__


;=============================================================================
; Disk Address Packet (DAP)
;=============================================================================
struc BIOS.DAP

    .Bytes:                 resw    1   ; size of DAP in bytes (16)
    .ReadSectors:           resw    1   ; number of sectors to read
    .TargetBufferOffset:    resw    1   ; target buffer address offset
    .TargetBufferSegment:   resw    1   ; target buffer address segment
    .FirstSector:           resq    1   ; first sector to read (up to 8 bytes)

endstruc


%endif ; __MONK_BOOT_BIOS_INC__

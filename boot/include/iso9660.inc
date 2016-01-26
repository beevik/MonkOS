;=============================================================================
; @file iso9660.inc
;
; ISO9660 constants and structures
;
; Copyright 2016 Brett Vickers.
; Use of this source code is governed by a BSD-style license that can
; be found in the LICENSE file.
;=============================================================================

%ifndef __MONK_BOOT_ISO9660_INC__
%define __MONK_BOOT_ISO9660_INC__


;=============================================================================
; PrimaryVolumeDescriptor
;=============================================================================
struc ISO.PrimaryVolumeDescriptor

    .Type                   resb    1
    .Identifier             resb    5
    .Version                resb    1
                            resb    1   ; unused
    .SystemId               resb    32
    .VolumeId               resb    32
                            resb    8   ; unused
    .VolumeSpaceSize        resq    1   ; int32 LSB-MSB
                            resb    32  ; unused
    .VolumeSetSize          resd    1   ; int16 LSB-MSB
    .VolumeSeqNum           resd    1   ; int16 LSB-MSB
    .BlockSize              resd    1   ; int16 LSB-MSB
    .PathTableSize          resq    1   ; int32 LSB-MSB
    .PathTableLBA           resd    1   ; int32 LSB
    .PathTableLBAOpt        resd    1   ; int32 LSB
    .MpathTableLBA          resd    1   ; int32 MSB
    .MpathTableLBAOpt       resd    1   ; int32 MSB
    .RootDirEntry           resb    34  ; contains an ISO.DirectoryEntry
    .VolumeSetId            resb    128
    .Publisher              resb    128
    .DataPreparer           resb    128
    .AppId                  resb    128
    .Copyright              resb    38
    .AbstractFileId         resb    36
    .BiblioFileId           resb    37
    .CreationDateTime       resb    17
    .ModifyDateTime         resb    17
    .ExpireDateTime         resb    17
    .EffectiveDateTime      resb    17
    .FileStructVersion      resb    1
                            resb    1   ; unused
    .AppData                resb    512
    .Reserved               resb    653

endstruc


;=============================================================================
; DirectoryEntry
;=============================================================================
struc ISO.DirectoryEntry

    .RecordLength           resb    1
    .ExtAttribLength        resb    1
    .LocationLBA            resq    1   ; int32 LSB-MSB
    .Size                   resq    1   ; int32 LSB-MSB
    .RecordingDateTime      resb    7
    .FileFlags              resb    1
    .FileUnitSize           resb    1
    .GapSize                resb    1
    .Volume                 resd    1   ; int16 LSB-MSB
    .NameLength             resb    1
    .Name:                              ; variable length

endstruc

%endif ; __MONK_BOOT_ISO9660_INC__

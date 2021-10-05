;-----------------------------------------------------------------------------
;
;                               OS KERNEL
;
;                   COPYRIGHT (c) 1994 by JOHN C. OVERTON
;               Advanced Communication Development Tools, Inc
;
;
;             Module:  OSSWITCH.ASM
;
;              Title:  Context switch.
;
;        Description:  Switch from one process to another.
;
;             Author:  John C. Overton
;
;               Date:  04/25/94
;
;-----------------------------------------------------------------------------

            PUBLIC _OsSwitch
            .MODEL LARGE
            .CODE

;-----------------------------------------------------------------------------
;           PERFORM A CONTEXT SWITCH (From task level)
;           void OSCtxSw(BYTE **OldStack, BYTE **NewStack)
;-----------------------------------------------------------------------------
_OsSwitch   PROC   FAR
            PUSH   BP
            MOV    BP,SP

            PUSHF                      ; Save processor flags.

            PUSH   AX                  ; Order is in PUSHA order.
            PUSH   CX
            PUSH   DX
            PUSH   BX
            PUSH   SP
            PUSH   BP
            PUSH   SI
            PUSH   DI                  ; Save current task's context

            PUSH   ES

            LES    BX,DWORD PTR [BP+6] ; Get address of where to store ss:ss.
            MOV    ES:[BX],SP          ; Save stack pointer.
            MOV    ES:[BX+2],SS        ; Save stack segment.

            LES    BX,DWORD PTR [BP+10]; Point to new stack ss:sp.
            MOV    SP,ES:[BX]          ; Get new task's stack pointer.
            MOV    SS,ES:[BX+2]        ; Get new task's stack segment.

            POP    ES

            POP    DI                  ; Order is in POPA order.
            POP    SI
            POP    BP
            POP    BX
            POP    BX
            POP    DX
            POP    CX
            POP    AX

            POPF                      ; Restore processor flags.

            POP    BP
            RET                       ;Return to new task
_OsSwitch   ENDP
            END

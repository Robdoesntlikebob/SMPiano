; Commented SPC-700 IPL ROM
; by eKid

; Original disassembly from SID-SPC written by Alfatech/Triad

; This code assembles with TASM

;-------------------------------------------------------------------------------
; DEFINITIONS
;-------------------------------------------------------------------------------

WriteAdr    =$00    ; Write address during transfers
Port0       =$F4    ; I/O ports
Port1       =$F5
Port2       =$F6
Port3       =$F7

.ORG $FFC0
;-------------------------------------------------------------------------------
Start:
;-------------------------------------------------------------------------------

; set stack pointer to $EF
;   "why EF? page1 has 256 memory bytes!"
;   because the value in X is reused in the page0 clear (saves 2 bytes)
;   the higher 16 bytes of page0 contain hardware registers.

    mov x, #$EF
    mov sp, x

; clear zero-page memory

    mov a, #$00
clrpg0: mov (x), a
    dec x
    bne clrpg0

; indicate ready signal, write 0BBAAh to ports 0/1

    mov Port0, #$AA
    mov Port1, #$BB

; idle until the SNES sends the transfer signal to port0 ($CC)
; and then process data

wait1:  cmp $F4, #$CC
    bne wait1

    bra ProcessData

;-------------------------------------------------------------------------------
TransferData:
;-------------------------------------------------------------------------------

; wait until Port0 gets zero written to it

wait2:  mov y, Port0
    bne wait2

; this is the main transfer loop

transfer_bytes:
    cmp y, Port0    ; check for data
    bne check_end

    mov a, Port1    ; read byte of data
    mov Port0, y    ; reply to SNES (snes can write new data now)
    mov [WriteAdr]+Y, A ; write data to memory
    inc y       ; increment index
    bne transfer_bytes  ; loop

; index overflowed, increment high byte of WriteAdr
    inc WriteAdr+1

check_end:

; if y - port0 < 0 then the transfer is complete (SNES added 2 or more)

    bpl transfer_bytes

; confirm this! we may have checked with invalid data
; also, this is used when the "inc WriteAdr+1" path is taken
;         (when transferring to $8000 or higher)

    cmp y, Port0
    bpl transfer_bytes

; transfer is finished, process data again

;-------------------------------------------------------------------------------
ProcessData:
;-------------------------------------------------------------------------------

; read word from ports 2/3
; word may be data write address,
; or program entry point (depending on port0)

    movw    ya, Port2
    movw    WriteAdr, ya
    movw    ya, $F4
    mov Port0, a    ; reply to SNES with PT0 data
    mov a, y
    mov x, a

; if port1 wasn't zero, then start the transfer

    bne TransferData

; otherwise...
; jump to program entry point
; X is zero in this case, so this
; is an effective "movw pc, WriteAdr"
    jmp [WriteAdr+X]

;-------------------------------------------------------------------------------
ResetVector:
;-------------------------------------------------------------------------------
    di
    stop

; When program flow is passed to the user code, the Accumulator
; and X/Y index registers are zero, and the SP is initialized to $EF.
; Also, page0 memory is cleared. (EXCEPT for the word at $00)

.end
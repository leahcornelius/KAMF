;------------------------------------------------------------------------
;
;  test.ASM
;
;  Test program for the mc6802-based Cornputor 
;  Intended for the SBASM assembler - uses macros specific to this!
;  This will blink LEDs attached to PIA PB0 -> PB6
;  
;  The program starts at address $8000 (CPU's perspective) but should fill the 
;  Entire 32kb ROM (up to 0xFFFF)
;
;  Author: Leah Cornelius @ Cornelius Innovations 
;  Date: 2/05/2023
;
;------------------------------------------------------------------------
             .LI    OFF                 ; Dont print the assembly to console when assembling
             .CR    6800                ; Select cross overlay (Motorola 6802)
             .OR    $8000               ; The program will start at address $8000 
             .TF    corn8.bin, BIN        ; Set raw binary output

;------------------------------------------------------------------------
;  Declaration of constants
;------------------------------------------------------------------------

PIA_A           .EQ     $0080           ; Pia data register A 
PIA_B           .EQ     $0081           ; Pia data register B 
CON_A           .EQ     $0082           ; Pia control register A
CON_B           .EQ     $0083           ; Pia control register B
TIMER_HIGH      .EQ     $0000          ; Blink timer (high byte)
TIMER_LOW       .EQ     $0001          ; Blink timer (low byte)
PORT_B_PATTERN  .EQ     $0005          ; Pattern for port B

;------------------------------------------------------------------------
;  Reset and initialisation
;------------------------------------------------------------------------

RESET           LDS     #$007F          ; Reset stack pointer
                LDAA    #%0000.0100     ; Initialise PIA ports
                STAA    CON_A
                STAA    CON_B
                LDAA    #%1111.1111     ; All pins are outputs
                STAA    PIA_A           ;  for both PIA ports
                STAA    PIA_B
                CLRB
                STAB    CON_A
                STAB    CON_B
                STAA    PIA_A
                STAA    PIA_B
                LDAA    #%0000.0100     ; Select data registers again
                STAA    CON_A
                STAA    CON_B
                LDAA    #%1111.1111     
                STAA    PORT_B_PATTERN ; Led pattern for port B
                CLRA
                CLRB


;------------------------------------------------------------------------
;  Main program loop
;------------------------------------------------------------------------

MAIN            BSR     REFRESH_LEDS    ; Show pattern on LEDS
                BSR     .SETUP_DELAY
                BSR     DELAY           ; Wait for ~1 s
                BSR     UPDATE_PATTERN  ; Update the in memory pattern to be sent to LEDs
                BRA     MAIN            ; Loop forever

.SETUP_DELAY    LDAA #%1111.1111        ; ~1s delay
                LDAB #%1111.1111 
                RTS
;------------------------------------------------------------------------
; Delay subroutine
; Uses two bytes, set to the values of accumulators A & B (high and low)
; Max delay is ~1s (A and B set to 0xFF), must set the content of accumulators
; Before starting this subroutine
;------------------------------------------------------------------------
DELAY           STAA    TIMER_HIGH         ; Set the specified delay (high)
                STAB    TIMER_LOW          ;                         (low)
                BSR     .HIGH_LOOP          ; Perform iterations
                RTS

.LOW_LOOP       DEC     TIMER_LOW           ; Decrement low byte
                BNE     .LOW_LOOP           ; Repeat until low byte is 0
                RTS                     

.HIGH_LOOP      STAB TIMER_LOW          ; Reset low byte
                BSR .LOW_LOOP           ; Complete 1 low byte iteration
                DEC TIMER_HIGH          ; Decrement high byte
                BNE .HIGH_LOOP          ; Repeat until high byte is 0
                RTS

;------------------------------------------------------------------------
;  Blink LEDs on PIA port B (PB0 -> PB6)
;------------------------------------------------------------------------

UPDATE_PATTERN  LDAA    PORT_B_PATTERN            ; Get current LED pattern
                EORA    #%1111.1111     ; Toggle all bits
                STAA    PORT_B_PATTERN
                RTS

REFRESH_LEDS    LDAA    PORT_B_PATTERN
                STAA    PIA_B           ; Send to PIA
                RTS


;------------------------------------------------------------------------
;  Interrupt and reset vectors
;------------------------------------------------------------------------

                .NO     $FFF8,$FF
                .DA     RESET           ;IRQ (Not used)
                .DA     RESET           ;SWI (Not used)
                .DA     RESET           ;NMI (Not used)
                .DA     RESET           ;Reset

;------------------------------------------------------------------------
;
;  test.ASM
;
;  Test program for the mc6802-based Cornputor 
;  Intended for the SBASM assembler - uses macros specific to this!
;  This will blink and LED on PIA PB0 -> PB6.
;  The string "Lo and behold" is stored at memory location $0900
;  this serves only as a "why the fuck not" - plus makes it easier to see 
;  if the hex file is correctly flashed. 
;  
;  The program starts at address $F800 (CPU's perspective) but should fill the 
;  Entire 2KiB RAM (used as ROM, reporgramed after each power cycle) 
;
;  Author: Leah Cornelius @ Cornelius Innovations 
;  Date: 2/05/2023
;
;------------------------------------------------------------------------

             .CR    6800                ; Select cross overlay (Motorola 6802)
             .OR    $F800               ; The program will start at address $F800 
             .TF    test.hex,INT        ; Set intel hex output (this can be used on non intel processors)

;------------------------------------------------------------------------
;  Declaration of constants
;------------------------------------------------------------------------

PIA_A           .EQ     $0080           ; Pia data register A 
PIA_B           .EQ     $0081           ; Pia data register B 
CON_A           .EQ     $0082           ; Pia control register A
CON_B           .EQ     $0083           ; Pia control register B
BLINK_TIMER     .EQ     $0084           ; Blink timer

HELLO           .EQ     $0900
                .DB     "Lo and behold",0

;------------------------------------------------------------------------
;  Reset and initialisation
;------------------------------------------------------------------------

RESET           LDS     #$007F          ; Reset stack pointer
                LDAA    #%0000.0100     ; Initialise PIA ports
                STAA    CON_A
                STAA    CON_B
                LDAA    #%0111.1111     ; b0..b6 are outputs b7 is input
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
                LDAA    #200              ; Initialise blink timer
                STAA    BLINK_TIMER

;------------------------------------------------------------------------
;  Main program loop
;------------------------------------------------------------------------

MAIN            BSR     BLINK_LEDS      ;     Blink LEDS
                CLRB                    ; In case push button A not down
                
                DEC     BLINK_TIMER     ; Decrement counter
                BPL     .WAIT_TIME      
                LDAA    #200              ; Restart counter
                STAA    BLINK_TIMER
.WAIT_TIME      NOP                     ; Wait for some time
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                NOP
                RTS

;------------------------------------------------------------------------
;  Blink LEDs on PIA port B (PB0 -> PB6)
;------------------------------------------------------------------------

BLINK_LEDS     LDAA    PIA_B            ; Get current LED pattern
                EORA    #%1111.1111     ; Toggle all bits
                STAA    PIA_B           ; and show it
                RTS

;------------------------------------------------------------------------
;  Interrupt and reset vectors
;------------------------------------------------------------------------

                .NO     $FFF8,$FF
                .DA     RESET           ;IRQ (Not used)
                .DA     RESET           ;SWI (Not used)
                .DA     RESET           ;NMI (Not used)
                .DA     RESET           ;Reset

                .LI     OFF
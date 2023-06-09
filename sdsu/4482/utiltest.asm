       COMMENT *

This file is used to generate DSP code for the utility board. It will time
     the exposure, operate the shutter, control the CCD temperature and 
     turn the analog power on. This is Rev. 3.00 software. 
Modified 1-12-97 for 10 MHz input clock frequency by adding 2 to elapsed
     exposure time rather than one. 
Power ON sequence written for Gen II power control board, Rev. 4A

-d DOWNLOAD 'HOST'	To generate code for downloading to DSP memory.
-d DOWNLOAD 'ROM'	To generate code for writing to the ROM.

	*
        PAGE    132	; Printronix page width - 132 columns

; Name it a section so it doesn't conflict with other application programs
	SECTION	UTILTEST

;  These are also defined in "utilboot.asm", so be sure they agree
APL_NUM	EQU	1	; Application number from 1 to 10
APL_ADR	EQU	$90	; Starting address of application program
BUF_STR	EQU	$80	; Starting address of buffers in X:
BUF_LEN	EQU	$20	; Length of buffers
SSI_BUF	EQU	BUF_STR		; Starting address of SCI buffer in X:
COM_BUF EQU     SSI_BUF+BUF_LEN	; Starting address of command buffer in X:
COM_TBL EQU     COM_BUF+BUF_LEN	; Starting address of command table in X:

;  Define some useful constants
APL_XY	EQU	$1EE0	; Starting address in EEPROM of X: and Y: values
DLY_MUX EQU     70      ; Number of DSP cycles to delay for MUX settling
DLY_AD  EQU     100     ; Number of DSP cycles to delay for A/D settling

; Assign addresses to port B data register
PBD     EQU     $FFE4   ; Port B Data Register
IPR     EQU     $FFFF   ; Interrupt Priority Register

;  Addresses of memory mapped components in Y: data memory space
;  Write addresses first
WR_DIG  EQU     $FFF0   ; was $FFFF  Write Digital output values D00-D15
WR_MUX  EQU     $FFF1   ; Select MUX connected to A/D input - one of 16
EN_DIG	EQU	$FFF2	; Enable digital outputs
WR_DAC3 EQU     $FFF7   ; Write to DAC#3 D00-D11
WR_DAC2 EQU     $FFF6   ; Write to DAC#2 D00-D11
WR_DAC1 EQU     $FFF5   ; Write to DAC#1 D00-D11
WR_DAC0 EQU     $FFF4   ; Write to DAC#0 D00-D11

;  Read addresses next
RD_DIG  EQU     $FFF0   ; Read Digital input values D00-D15
STR_ADC EQU     $FFF1   ; Start ADC conversion, ignore data
RD_ADC  EQU     $FFF2   ; Read A/D converter value D00-D11
WATCH   EQU     $FFF7   ; Watch dog timer - tell it that DSP is alive

;  Bit definitions of STATUS word
ST_SRVC	EQU     0       ; Set if ADC routine needs executing
ST_EX   EQU     1       ; Set if timed exposure is in progress
ST_SH   EQU     2       ; Set if shutter is open
ST_READ EQU     3	; Set if a readout needs to be initiated
STRT_EX	EQU	4	; Set to indicate start of exposure

; Bit definitions of software OPTIONS word
OPT_SH  EQU     0       ; Set to open and close shutter

;  Bit definitions of Port B = Host Processor Interface
PWR_EN1	EQU     0       ; Power enable bit ONE - Output
PWR_EN0	EQU     1       ; Power enable bit ZERO  - Output
PWRST	EQU     2       ; Reset power conditioner counter - Output
SHUTTER EQU     3       ; Control shutter - Output
IRQ_T   EQU     4       ; Request interrupt service from timing board - Output
SYS_RST EQU     5       ; Reset entire system - Output
WATCH_T EQU     8       ; Processed watchdog signal from timing board - Input

;**************************************************************************
;                                                                         *
;    Register assignments  						  *
;	 R1 - Address of SCI receiver contents				  *
;	 R2 - Address of processed SCI receiver contents		  *
;        R3 - Pointer to current top of command buffer                    *
;        R4 - Pointer to processed contents of command buffer		  *
;	 N4 - Address for internal jumps after receiving 'DON' replies	  *
;        R0, R5, R6, A, X0, X1 - For use by program only                  *
;	 R7 - For use by SCI ISR only					  *
;        Y0, Y1, and B - For use by timer ISR only. If any of these	  *
;		registers are needed elsewhere they must be saved and	  *
;	        restored in the TIMER ISR.                        	  *
;**************************************************************************

; Specify execution and load addresses.
	IF	@SCP("DOWNLOAD","HOST")
	ORG	P:APL_ADR,P:APL_ADR		; Download address
	ELSE
        ORG     P:APL_ADR,P:APL_NUM*$300+$1D00	; EEPROM generation
	ENDIF


; The TIMER addresses must be defined here and SERVICE must follow to match
;   up with the utilboot code
;	JMP	<SERVICE		; Millisecond timer interrupt
	RTS

;  The SERVICE address must be defined here to match up with the utilboot code,
;     and the TIMER routine must follow. They are both do-nothing returns. 
	RTS				; Just return from the subroutine call
TIMER	RTI				; RTI for now so downloading works

; Shutter support subroutines for the TIMER executive
;   Also support shutter connection to timing board for now.
OSHUT	BCLR    #SHUTTER,X:PBD  ; Clear Port B bit #3 to open shutter
        BCLR    #ST_SH,X:<STATUS ; Clear status bit to mean shutter open
        RTS

CSHUT	BSET    #SHUTTER,X:PBD  ; Set Port B bit #3 to close shutter
        BSET    #ST_SH,X:<STATUS ; Set status to mean shutter closed
        RTS

; These are called directly by command, so need to call subroutines in turn
OPEN	JSR	OSHUT		; Call open shutter subroutine
	JMP	<FINISH		; Send 'DON' reply
CLOSE	JSR	CSHUT		; Call close shutter subroutine
	JMP	<FINISH		; Send 'DON' reply

; Test D/A converter
TDA	MOVE	#CTDA,N4	; Set internal jump address	

CTDA	MOVEP	#$C000,X:IPR	; Disable timer interrupts
CTDA1	MOVEP	Y:WATCH,B	; Reset watchdog timer
	CLR 	B
	MOVE	X:<ONE,X0	; DAC increment value
	MOVE	#>$1000,Y0	; Cycle through all 12 bits of the DAC
	DO	Y0,D_TDA
	MOVE	#>$FFF4,R0
	REP	#4		; Update the four DACs in turn
	MOVE	B,Y:(R0)+	; Write to DAC
	ADD	X0,B		; B = B + 1
D_TDA
	MOVE	R1,A		; Keep executing TDA if a new command
	MOVE	R2,X0		;   has not come in
	CMP	X0,A
	JEQ	CTDA1		
	MOVEP	#$8007,X:IPR	; Re-enable timer interrrupts
	JMP	<START

; Test A/D converter
TAD	MOVE	#CTAD,N4	; Set internal jump address	

CTAD	MOVEP	#$C000,X:IPR	; Disable timer interrupts
CTAD1	MOVE	#>$3E8,B	; Starting DAC value = 1000
	MOVE	X:<ONE,X0	; Increment DAC value by one each cycle
	MOVE	#>$7D0,Y0	; So ending DAC value = $BB8 = 3000
	MOVE	#WR_DAC2,R0	; Select DAC2
	MOVE	#WR_DAC3,R5	; Select DAC3	
	MOVEP	#>$9,Y:WR_MUX	; Set MUX to select correct input to A/D
	DO	Y0,L_TAD1
	MOVE	B,Y:(R0)	; Write original value to DAC2
	REP	#DLY_MUX	; Delay for MUX to settle
	NOP
        MOVEP   Y:STR_ADC,X1    ; Start A/D conversion - dummy read
	REP	#DLY_AD		; Delay for A/D to settle
	NOP
        MOVEP   Y:RD_ADC,Y:(R5)	; Get the A/D value and write it to DAC3
	ADD	X0,B
	MOVEP	Y:WATCH,X1	; Reset watchdog timer
L_TAD1
	MOVE	R1,A		; Keep executing TAD if a new command
	MOVE	R2,X0		;   has not come in
	CMP	X0,A
	JEQ	CTAD1 
	MOVEP	#$8007,X:IPR	; Re-enable timer interrupts
	JMP	<START

; Test digital I/O
TDG	MOVEP	#1,Y:EN_DIG	; Enable digital outputs
	MOVE	#CTDG,N4	; Set internal jump address	

CTDG	MOVEP	#$C000,X:IPR	; Disable interrupts
CTDG1	MOVEP	#0,Y:WR_DIG	; Clear all outputs to make it look clean
	MOVEP	Y:WATCH,A	   ; Reset watchdog timer
	REP	#<$FA		; Delay just to make the waveforms
	CLR	A		;   look nicer
	MOVEP	#$FFFF,Y:WR_DIG	; Put a "start" bit to make scope
	REP	#5		;    trigerring easier
	NOP
	MOVEP	#$0000,Y:WR_DIG
	REP	#10
	NOP
	MOVEP	Y:RD_DIG,A1	; Read 16-bit digital number
	DO	#16,D_TDG
	MOVEP	A1,Y:WR_DIG	; Write 16-bit digital number
	LSR	A		; Shift right one bit
	JCS	SET_15		; Need to rotate 16 bits,
	BCLR	#15,A1		;  not 24
	JMP	CLR_15
SET_15	BSET	#15,A1
CLR_15	NOP
D_TDG
	MOVE	R1,A		; Check for input SCI commands
	MOVE	R2,X0
	CMP	X0,A
	JEQ	CTDG1		; Keep going if no commands
	MOVEP	#$8007,X:IPR	; Restore interrupts
	JMP	<START		; Go get the command

;  **************  BEGIN  COMMAND  PROCESSING  ***************
; Subroutine to turn analog power OFF
PWR_OFF_SUBROUTINE
	MOVE	X:<TIMING,A
	MOVE	A,X:(R3)+       ; Header from Utility to timing board
	MOVE	Y:<CSW,A
	MOVE	A,X:(R3)+       ; Clear all analog switches
	MOVE	#*+3,N4		; Set internal jump address after 'DON'
	JMP	<XMT_CHK	; Send the command to the timing board

; Instruct the power control board to turn the analog power switches OFF
	BSET	#9,X:PBDDR	; Make sure PWREN is an input
	IF	@SCP("POWER","R6")
	BSET	#LVEN,X:PBD	; LVEN = HVEN = 1 => Power reset
	BSET	#PWRST,X:PBD
	BSET	#HVEN,X:PBD
	ELSE			; Earlier Revision power control boards
	BSET	#PWRST,X:PBD	; Reset power control board
	REP	#30
	NOP
	BCLR	#PWRST,X:PBD
	ENDIF

	RTS			; Return from PWR_OFF_SUBROUTINE

; Power OFF command execution
PWR_OFF	MOVEP   #$2000,X:IPR    	; Disable TIMER interrupts
	JSR	<PWR_OFF_SUBROUTINE
	JMP	<PWR_END		; Reply 'DON'

; Start power-on cycle
PWR_ON	MOVEP   #$2000,X:IPR    ; Disable TIMER interrupts
	JSR	<PWR_OFF_SUBROUTINE ; Turn everything OFF

; The clocks must be not clocking during power ON because of the A/D converter	
	MOVE	X:<TIMING,A
	MOVE	A,X:(R3)+       ; Header from Utility to timing
	MOVE	Y:<STP,A
	MOVE	A,X:(R3)+       ; Stop the clocks during power on
	MOVE	#*+3,N4		; Set internal jump address after 'DON'
	JMP	<XMT_CHK	; Send the command to the timing board

; Wait for at least one cycle of serial and parallel clocks for the STP 
;   command to take effect
	MOVE	#30000,X0
	DO      X0,WT_PON1	; Wait 30 millisec or so for settling
        REP	#5
	MOVEP	Y:WATCH,X0	; Reset watchdog timer
WT_PON1

; Now turn ON the low voltages (+/- 6.5V, 16.5V) 
	IF	@SCP("POWER","R6")
	BCLR	#LVEN,X:PBD	; LVEN = Low => Turn on +/- 6.5V, +/- 16.5V
	BCLR	#PWRST,X:PBD
	ELSE
	BSET	#LVEN,X:PBD	; Make sure line is high to start with
	DO	#255,L_PON1	; The power conditioner board wants to
	BCHG    #LVEN,X:PBD	;   see 128 H --> L transitions
	NOP			; Backplane signal settling time delay
L_PON1
	ENDIF
	JSR	<PWR_DLY	; Delay for a little while
	MOVEP   #2,Y:WR_MUX     ; Select +15V MUX input
	MOVE	#65000,X0
	DO      X0,WT_PON2	; Wait 20 millisec or so for settling
	REP	#5
	MOVEP	Y:WATCH,X0	; Reset watchdog timer
WT_PON2
        MOVEP   Y:STR_ADC,X0    ; Start A/D conversion - dummy read
        DO	#DLY_AD,L_PON2	; Wait for the A/D to settle
        CLR     A  X:<CFFF,X0	; This saves some space
L_PON2
        MOVEP   Y:RD_ADC,A1     ; Get the A/D value
        AND     X0,A  Y:<T_P15,X0 ; A/D is only valid to 12 bits

; Test that the voltage is in the range abs(initial - target) < margin
        SUB     X0,A  A1,Y:<I_P15
        ABS     A  Y:<K_P15,X0
        SUB     X0,A
        JGT     <PERR           ; Take corrective action

TST_M15 MOVEP   #3,Y:WR_MUX     ; Select -15v MUX input
        DO	#DLY_MUX,L_PON3	; Wait for the MUX to settle
        NOP
L_PON3
        MOVEP   Y:STR_ADC,X0    ; Start A/D conversion - dummy read
        DO	#DLY_AD,L_PON4	; Wait for the A/D to settle
        CLR     A  X:<CFFF,X0	; Clear A, so put it in DO loop
L_PON4
        MOVEP   Y:RD_ADC,A1     ; Get the A/D value
        AND     X0,A  Y:<T_M15,X0 ; A/D is only valid to 12 bits

; Test that the voltage is in the range abs(initial - target) < margin
        SUB     X0,A  A1,Y:<I_M15
        ABS     A  Y:<K_M15,X0
        SUB     X0,A
        JGT     <PERR

; Now turn on the high voltage HV (nominally +36 volts)
	IF	@SCP("POWER","R6")
HV_ON	BCLR	#HVEN,X:PBD	; HVEN = Low => Turn on +36V
	ELSE
HV_ON	BSET	#HVEN,X:PBD	; Make sure line is high to start with
	DO	#255,L_PON5	; The power conditioner board wants to
	BCHG    #HVEN,X:PBD	;   see 128 H --> L transitions
L_PON5
	ENDIF

	JSR	<PWR_DLY	; Delay for a little while	
	MOVEP   #1,Y:WR_MUX     ; Select high voltage MUX input
	MOVE	#65000,X0
	DO      X0,WT_HV	; Wait a few millisec for settling
	NOP
WT_HV
	MOVEP   Y:STR_ADC,X0    ; Start A/D conversion - dummy read
	DO	#DLY_AD,L_PON6	; Wait for the A/D to settle
	CLR     A  X:<CFFF,X0	; Clear A, so put it in DO loop
L_PON6
	MOVEP   Y:RD_ADC,A1     ; Get the A/D value
	AND     X0,A  Y:<T_HV,X0 ; A/D is only valid to 12 bits

; Test that the voltage is in the range abs(initial - target) < margin
	SUB     X0,A  A1,Y:<I_HV
	ABS     A  Y:<K_HV,X0
	SUB     X0,A
	JGT     <PERR           ; Take corrective action

; Start up the clocks and DC biases from the timing board
	MOVE	X:<TIMING,A
	MOVE	A,X:(R3)+       ; Header from Utility to timing
	MOVE	Y:<IDL,A
	MOVE	A,X:(R3)+       ; Start up the clock drivers
	MOVE	X:<TIMING,A
	MOVE	A,X:(R3)+       ; Header from Utility to timing
	MOVE	Y:<SBV,A
	MOVE	A,X:(R3)+       ; Set bias voltages

; Reply with a DONE message to the host computer
PWR_END	MOVE    X:<HOST,A
	MOVE    A,X:(R3)+       ; Header to host
	MOVE    X:<DON,A
	MOVE    A,X:(R3)+       ; Power is now ON
	MOVEP   #$2007,X:IPR    ; Enable TIMER interrupts
	JMP     <XMT_CHK	; Go transmit reply

; Or, return with an error message
PERR	JSR	<PWR_OFF_SUBROUTINE 	; Turn power OFF if there's an error
	MOVE    X:<HOST,A
	MOVE    A,X:(R3)+       ; Header to host
	MOVE    X:<ERR,A
        MOVE    A,X:(R3)+	; Power is ON
	MOVEP   #$2007,X:IPR    ; Enable TIMER interrupts
	JMP     <XMT_CHK	; Go transmit reply

; Delay between power control board instructions
PWR_DLY	DO	#4000,L_DLY
	NOP			
L_DLY
	RTS

; Start an exposure by first issuing a 'CLR' to the timing board
START_EX
	MOVE	X:<TIMING,A
	MOVE	A,X:(R3)+       ; Header from Utility to timing
	MOVE	Y:<CLR,A
	MOVE	A,X:(R3)+       ; Clear the CCD
	MOVE	#DONECLR,N4	; Set internal jump address after 'DON'
	JMP	<XMT_CHK	; Transmit these

; Come to here after timing board has signaled that clearing is done
DONECLR	BSET	#STRT_EX,X:<STATUS
	BSET    #ST_EX,X:<STATUS ; Exposure is in progress
	MOVE	X:<HOST,A
	MOVE	A,X:(R3)+
	MOVE	X:<DON,A
	MOVE	A,X:(R3)+
	JMP     <XMT_CHK	; Issue a 'DON' - exposure has begun

PAUSE   BCLR    #ST_EX,X:<STATUS ; Take out of exposing mode
        JSSET   #OPT_SH,X:<OPTIONS,CSHUT ; Close shutter if needed
        JMP     <FINISH		; Issue 'DON' and get next command

RESUME	BSET    #ST_EX,X:<STATUS ; Put in exposing mode
	JSSET   #OPT_SH,X:<OPTIONS,OSHUT ; Open shutter if needed
        JMP     <FINISH		; Issue 'DON' and get next command

ABORT	BCLR    #ST_EX,X:<STATUS ; Take out of exposing mode
	MOVE    X:<TIMING,A
	MOVE    A,X:(R3)+       ; Header from Utility to timing
	MOVE    Y:<IDL,A
	MOVE    A,X:(R3)+       ; Put timing board in IDLE mode
	JSR     <CSHUT          ; To be sure
	JMP     <FINISH		; Issue 'DON' and get next command

; A 'DON' reply has been received in response to a command issued by
;    the Utility board. Read the X:STATUS bits in responding to it.

; Test if an internal program jump is needed after receiving a 'DON' reply
PR_DONE	MOVE	N4,R0		; Get internal jump address
	MOVE	#<START,N4	; Set internal jump address to default
	JMP	(R0)		; Jump to the internal jump address

; Check for program overflow - its hard to overflow since this application
;   can be very large indeed
	IF	@CVS(N,*)>APL_XY
        WARN    'Application P: overflow!'	; Make sure next application
	ENDIF					;  will not be overwritten

; Command table resident in X: data memory
;  The last part of the command table is not defined for "bootrom"
;     because it contains application-specific commands

	IF	@SCP("DOWNLOAD","HOST")
	ORG	X:COM_TBL,X:COM_TBL
	ELSE			; Memory offsets for generating EEPROMs
        ORG     P:COM_TBL,P:APL_NUM*$300+$1D00+($200-APL_ADR)
	ENDIF
	DC	'PON',PWR_ON	; Power ON
	DC      'POF',PWR_OFF	; Power OFF
	DC	'TAD',TAD	; Test A/D
	DC	'TDA',TDA	; Test D/A
	DC	'TDG',TDG	; Test Digital I/O
	DC	'SEX',START_EX	; Start exposure
	DC	'PEX',PAUSE	; Pause exposure
	DC	'REX',RESUME	; Resume exposure
	DC	'AEX',ABORT	; Abort exposure
	DC	'OSH',OPEN	; Open shutter
	DC	'CSH',CLOSE	; Close shutter
	DC      'DON',PR_DONE	; Process DON reply
	DC	0,START,0,START,0,START,0,START

; Y: parameter table definitions, containing no "bootrom" definitions
	IF	@SCP("DOWNLOAD","HOST")
	ORG	Y:0,Y:0		; Download address
	ELSE
        ORG     Y:0,P:		; EEPROM address continues from P: above
	ENDIF
DIG_IN  DC      0       ; Values of 16 digital input lines
DIG_OUT DC      0       ; Values of 16 digital output lines
DAC0    DC      0       ; Table of four DAC values to be output
DAC1    DC      1000               
DAC2    DC      2000            
DAC3    DC      3000            
NUM_AD  DC      16      ; Number of inputs to A/D converter
AD_IN   DC      0,0,0,0,0,0,0,0
        DC      0,0,0,0,0,0,0,0 ; Table of 16 A/D values
EL_TIM_MSECONDS  DC      0       ; Number of milliseconds elapsed
TGT_TIM DC      6000    ; Number of milliseconds desired in exposure
U_CCDT  DC      $C20    ; Upper range of CCD temperature control loop
L_CCDT  DC      $C50    ; Lower range of CCD temperature control loop
K_CCDT  DC      85      ; Constant of proportionality for CCDT control
A_CCDT  EQU     AD_IN+5 ; Address of CCD temperature in A/D table
T_CCDT	DC	$0FFF	; Target CCD T for small increment algorithmn
T_COEFF	DC	$010000	; Coefficient for difference in temperatures
DAC0_LS	DC	0	; Least significant part of heater voltage

; Define power supply turn-on variables
	IF	@SCP("POWER","R6")
T_HV	DC      $240    ; Target HV supply voltage for Rev 6 pwr contl board
	ELSE
T_HV	DC      $4D0    ; Target HV supply voltage for Rev 2 or 3 boards
	ENDIF
K_HV	DC      $080    ; Tolerance of HV supply voltage
T_P15   DC      $5C0    ; Target +15 volts supply voltage
K_P15   DC      $080     ; Tolerance of +15 volts supply voltage
T_M15   DC      $A40    ; Target -15 volts supply voltage
K_M15   DC      $080     ; Tolerance of -15 volts supply voltage
I_HV	DC      0       ; Initial value of HV
I_P15   DC      0       ; Initial value of +15 volts
I_M15   DC      0       ; Initial value of -15 volts

; Define some command names
CLR     DC      'CLR'   ; Clear CCD
RDC     DC      'RDC'   ; Readout CCD
ABR     DC      'ABR'   ; Abort readout
OSH     DC      'OSH'   ; Open shutter connected to timing board
CSH     DC      'CSH'   ; Close shutter connected to timing board
POK     DC      'POK'   ; Message to host - power in OK
PER     DC      'PER'   ; Message to host - ERROR in power up sequence
SBV	DC	'SBV'	; Message to timing - set bias voltages
IDL	DC	'IDL'	; Message to timing - put camera in idle mode
STP	DC	'STP'	; Message to timing - Stop idling
CSW	DC	'CSW'	; Message to timing - clear switches

; Miscellaneous
CC00	DC	$C00	; Maximum heater voltage so the board doesn't burn up
SV_A1	DC	0	; Save register A1 during analog processing
SV_SR	DC	0	; Save status register during timer processing
EL_TIM_FRACTION DC 0	; Fraction of a millisecond of elapsed exposure time
INCR	DC	$CCCCCC	; Exposure time increment = 0.8 milliseconds
SH_DEL	DC	10	; Shutter closing time
TEMP	DC	0	; Temporary storage location for X:PBD word

; During the downloading of this application program the one millisecond 
;   timer interrupts are enabled, so the utility board will attempt to execute 
;   the partially downloaded TIMER routine, and crash. A workaround is to 
;   put a RTI as the first instruction of TIMER so it doesn't execute, then 
;   write the correct instruction only after all the rest of the application 
;   program has been downloaded. Here it is - 

; Specify execution and load addresses.
	IF	@SCP("DOWNLOAD","HOST")
	ORG	P:APL_ADR,P:APL_ADR		; Download address
	ELSE
        ORG     P:APL_ADR,P:APL_NUM*$300+$1D00	; EEPROM generation
	ENDIF
	JMP	<SERVICE			; Millisecond timer interrupt
	MOVEC	SR,Y:<SV_SR 			; Save Status Register

	ENDSEC		; End of SECTION UTILAPPL

; End of program
        END 

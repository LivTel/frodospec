; Miscellaneous CCD control routines, common to all detector types

POWER_OFF
	JSR	<CLEAR_SWITCHES		; Clear all analog switches
	BSET	#LVEN,X:HDR 
	BSET	#HVEN,X:HDR 
	JMP	<FINISH

; Execute the power-on cycle, as a command
POWER_ON
	JSR	<CLEAR_SWITCHES		; Clear all analog switches
	JSR	<PON			; Turn on the power control board
	JCLR	#PWROK,X:HDR,PWR_ERR	; Test if the power turned on properly
	JSR	<SET_BIASES		; Turn on the DC bias supplies
	JSR	<SEL_OS			; Set up readout parameters
	BSET	#IDLMODE,X:<STATUS	; Idle after readout
	MOVE	#IDLE,R0		; Put controller in IDLE state
	MOVE	R0,X:<IDL_ADR
	JMP	<FINISH

; The power failed to turn on because of an error on the power control board
PWR_ERR	BSET	#LVEN,X:HDR		; Turn off the low voltage emable line
	BSET	#HVEN,X:HDR		; Turn off the high voltage emable line
	JMP	<ERROR

; As a subroutine, turn on the low voltages (+/- 6.5V, +/- 16.5V) and delay
PON	BCLR	#LVEN,X:HDR		; Set these signals to DSP outputs 
	MOVE	#2000000,X0
	DO      X0,*+3			; Wait 20 millisec for settling
	NOP 	

; Turn on the high +36 volt power line and then delay
	BCLR	#HVEN,X:HDR		; HVEN = Low => Turn on +36V
	MOVE	#10000000,X0
	DO      X0,*+3			; Wait 100 millisec for settling
	NOP
	RTS

; Set all the DC bias voltages and video processor offset values, reading
;   them from the 'DACS' table
SET_BIASES
	BSET	#3,X:PCRD		; Turn on the serial clock
	BCLR	#1,X:<LATCH		; Separate updates of clock driver
	BSET	#CDAC,X:<LATCH		; Disable clearing of DACs
	BSET	#ENCK,X:<LATCH		; Enable clock and DAC output switches
	MOVEP	X:LATCH,Y:WRLATCH	; Write it to the hardware
	JSR	<PAL_DLY		; Delay for all this to happen

; Read DAC values from a table, and write them to the DACs
	MOVE	#DACS,R0		; Get starting address of DAC values
	NOP
	NOP
	DO      Y:(R0)+,L_DAC		; Repeat Y:(R0)+ times
	MOVE	Y:(R0)+,A		; Read the table entry
	JSR	<XMIT_A_WORD		; Transmit it to TIM-A-STD
	NOP
L_DAC

; Let the DAC voltages all ramp up before exiting
	MOVE	#400000,X0
	DO	X0,*+3			; 4 millisec delay
	NOP
	BCLR	#3,X:PCRD		; Turn the serial clock off
	RTS

SET_BIAS_VOLTAGES
	JSR	<SET_BIASES
	JMP	<FINISH

CLR_SWS	JSR	<CLEAR_SWITCHES
	JMP	<FINISH

; Clear all video processor analog switches to lower their power dissipation
CLEAR_SWITCHES
	BSET	#3,X:PCRD	; Turn the serial clock on
	MOVE	#$0C3000,A	; Value of integrate speed and gain switches
	CLR	B
	MOVE	#$100000,X0	; Increment over board numbers for DAC writes
	MOVE	#$001000,X1	; Increment over board numbers for WRSS writes
	DO	#15,L_VIDEO	; Fifteen video processor boards maximum
	JSR	<XMIT_A_WORD	; Transmit A to TIM-A-STD
	ADD	X0,A
	MOVE	B,Y:WRSS
	JSR	<PAL_DLY	; Delay for the serial data transmission
	ADD	X1,B
L_VIDEO	
	BCLR	#CDAC,X:<LATCH		; Enable clearing of DACs
	BCLR	#ENCK,X:<LATCH		; Disable clock and DAC output switches
	MOVEP	X:LATCH,Y:WRLATCH 	; Execute these two operations
	BCLR	#3,X:PCRD		; Turn the serial clock off
	RTS

; Open the shutter by setting the backplane bit TIM-LATCH0
OSHUT	BSET    #ST_SHUT,X:<STATUS 	; Set status bit to mean shutter open
	BCLR	#SHUTTER,X:<LATCH	; Clear hardware shutter bit to open
	MOVEP	X:LATCH,Y:WRLATCH	; Write it to the hardware
        RTS

; Close the shutter by clearing the backplane bit TIM-LATCH0
CSHUT	BCLR    #ST_SHUT,X:<STATUS 	; Clear status to mean shutter closed
	BSET	#SHUTTER,X:<LATCH	; Set hardware shutter bit to close
	MOVEP	X:LATCH,Y:WRLATCH	; Write it to the hardware
        RTS

; Open the shutter from the timing board, executed as a command
OPEN_SHUTTER
	JSR	<OSHUT
	JMP	<FINISH

; Close the shutter from the timing board, executed as a command
CLOSE_SHUTTER
	JSR	<CSHUT
	JMP	<FINISH

; Clear the CCD, executed as a command
CLEAR	JSR	<CLR_CCD
	JMP     <FINISH

; Default clearing routine with serial clocks inactive
; Fast clear image before each exposure, executed as a subroutine
CLR_CCD	DO      Y:<NPCLR,LPCLR2	; Loop over number of lines in image
	MOVE    #PARALLEL_CLEAR,R0	; Address of parallel transfer waveform
	CLOCK
;	DO	#1000,LSCLR_1		; Clear out the serial shift register
;	MOVE	#SERIAL_SKIP_SPLIT,R0
;	CLOCK
;LSCLR_1	NOP
	JCLR    #EF,X:HDR,LPCLR1 ; Simple test for fast execution
	MOVE	#COM_BUF,R3
	JSR	<GET_RCV	; Check for FO command
	JCC	<LPCLR1		; Continue no commands received
	ENDDO   		; Cancel the DO loop system stack numbers
	NOP
	JMP	<LPCLR2		; Return prematurely from clearing
LPCLR1	NOP
LPCLR2
	MOVE	#COM_BUF,R3
	JSR	<GET_RCV		; Check for FO command
	RTS

; Start the exposure timer and monitor its progress
EXPOSE	MOVE	X:<EXPOSURE_TIME,B
	TST	B			; Special test for zero exposure time
	JEQ	<END_EXP		; Don't even start an exposure
	SUB	#1,B			; Timer counts from X:TCPR0+1 to zero
	BSET	#TIM_BIT,X:TCSR0	; Enable the timer #0
	MOVE	B,X:TCPR0
CHK_RCV	MOVE	#COM_BUF,R3		; The beginning of the command buffer
	JCLR    #EF,X:HDR,CHK_TIM	; Simple test for fast execution
	JSR	<GET_RCV		; Check for an incoming command
	JCS	<PRC_RCV		; If command is received, go check it
	MOVE	#SERIAL_IDLE,R0
	CLOCK
CHK_TIM	JCLR	#TCF,X:TCSR0,CHK_RCV	; Wait for timer to equal compare value
END_EXP	BCLR	#TIM_BIT,X:TCSR0	; Disable the timer
	JMP	(R7)			; This contains the return address

; Start the exposure, operate the shutter, and initiate the CCD readout
START_EXPOSURE
	MOVE	#$020102,B		; Initialize the PCI image address
	JSR	<XMT_WRD
	MOVE	#'IIA',B
	JSR	<XMT_WRD
	JSR	<CLR_CCD		; Clear out the CCD
	MOVE	#COM_BUF,R3		; The beginning of the command buffer
	JSR	<GET_RCV		; Check for FO command
	JCS	<PRC_RCV		; Process the command 
	MOVE	#TST_RCV,R0		; Process commands during the exposure
	MOVE	R0,X:<IDL_ADR
	JSR	<WAIT_TO_FINISH_CLOCKING

; Operate the shutter if needed and begin exposure
	JCLR	#SHUT,X:STATUS,L_SEX0
	JSR	<OSHUT			; Open the shutter if needed
L_SEX0	MOVE	#L_SEX1,R7		; Return address at end of exposure
	JMP	<EXPOSE			; Delay for specified exposure time
L_SEX1

; Now we really start the CCD readout, alerting the PCI board, closing the 
;  shutter, waiting for it to close and then reading out
STR_RDC	JSR	<PCI_READ_IMAGE		; Get the PCI board reading the image
	BSET	#ST_RDC,X:<STATUS 	; Set status to reading out
	JCLR	#SHUT,X:STATUS,S_DEL0
	JSR	<CSHUT			; Close the shutter if necessary

; Delay readout until the shutter has fully closed
	MOVE	Y:<SHDEL,A
	TST	A
	JLE	<S_DEL0
	MOVE	#100000,X0
	DO	A,S_DEL0		; Delay by Y:SHDEL milliseconds
	DO	X0,S_DEL1
	NOP
S_DEL1	NOP
S_DEL0
	JSET	#TST_IMG,X:STATUS,SYNTHETIC_IMAGE
	JMP	<RDCCD			; Finally, go read out the CCD

; Set the desired exposure time
SET_EXPOSURE_TIME
	MOVE	X:(R3)+,Y0
	MOVE	Y0,X:EXPOSURE_TIME
	MOVEP	Y0,X:TCPR0
	JMP	<FINISH

; Read the time remaining until the exposure ends
READ_EXPOSURE_TIME
	MOVE	X:TCR0,Y1		; Read elapsed exposure time
	JMP	<FINISH1

; Pause the exposure - close the shutter and stop the timer
PAUSE_EXPOSURE
	BCLR    #TIM_BIT,X:TCSR0	; Disable the DSP exposure timer
	JSR	<CSHUT			; Close the shutter
	JMP	<FINISH

; Resume the exposure - open the shutter if needed and restart the timer
RESUME_EXPOSURE
	BSET	#TIM_BIT,X:TCSR0	; Re-enable the DSP exposure timer
	JCLR	#SHUT,X:STATUS,L_RES
	JSR	<OSHUT			; Open the shutter ir necessary
L_RES	JMP	<FINISH

; Abort exposure - close the shutter, stop the timer and resume idle mode
ABORT_EXPOSURE
	JSR	<CSHUT			; Close the shutter
	BCLR    #TIM_BIT,X:TCSR0	; Disable the DSP exposure timer
	JCLR	#IDLMODE,X:<STATUS,NO_IDL2 ; Don't idle after readout
	MOVE	#IDLE,R0
	MOVE	R0,X:<IDL_ADR
	JMP	<RDC_E2
NO_IDL2	MOVE	#TST_RCV,R0
	MOVE	R0,X:<IDL_ADR
RDC_E2	JSR	<WAIT_TO_FINISH_CLOCKING
	BCLR	#ST_RDC,X:<STATUS	; Set status to not reading out
	DO      #4000,*+3		; Wait 40 microsec for the fiber
	NOP				;  optic to clear out 
	JMP	<FINISH
	
; Generate a synthetic image by simply incrementing the pixel counts
SYNTHETIC_IMAGE
	CLR	A
	DO      Y:<NPR,LPR_TST      	; Loop over each line readout
	DO      Y:<NSR,LSR_TST		; Loop over number of pixels per line
	REP	#20			; #20 => 1.0 microsec per pixel
	NOP
	ADD	#1,A			; Pixel data = Pixel data + 1
	NOP
	MOVE	A,B
	JSR	<XMT_PIX		;  transmit them
	NOP
LSR_TST	
	NOP
LPR_TST	
        JMP     <RDC_END		; Normal exit


; Transmit the 16-bit pixel datum in B1 to the host computer
XMT_PIX	ASL	#16,B,B
	NOP
	MOVE	B2,X1
	ASL	#8,B,B
	NOP
	MOVE	B2,X0
	NOP
	MOVEP	X1,Y:WRFO
	MOVEP	X0,Y:WRFO
	RTS

; Test the hardware to read A/D values directly into the DSP instead
;   of using the SXMIT option, A/Ds #2 and 3.
READ_AD	MOVE	X:(RDAD+2),B
	ASL	#16,B,B
	NOP
	MOVE	B2,X1
	ASL	#8,B,B
	NOP
	MOVE	B2,X0
	NOP
	MOVEP	X1,Y:WRFO
	MOVEP	X0,Y:WRFO
	REP	#10
	NOP
	MOVE	X:(RDAD+3),B
	ASL	#16,B,B
	NOP
	MOVE	B2,X1
	ASL	#8,B,B
	NOP
	MOVE	B2,X0
	NOP
	MOVEP	X1,Y:WRFO
	MOVEP	X0,Y:WRFO
	REP	#10
	NOP
	RTS

; Alert the PCI interface board that images are coming soon
PCI_READ_IMAGE
	MOVE	#$020104,B		; Send header word to the FO transmitter
	JSR	<XMT_WRD
	MOVE	#'RDA',B
	JSR	<XMT_WRD
	MOVE	Y:NSR,B			; Number of columns to read
	JSR	<XMT_WRD
	MOVE	Y:NPR,B			; Number of rows to read		
	JSR	<XMT_WRD
	RTS

; Wait for the clocking to be complete before proceeding
WAIT_TO_FINISH_CLOCKING
	JSET	#SSFEF,X:PDRD,*		; Wait for the SS FIFO to be empty	
	RTS

; Delay for serial writes to the PALs and DACs by 8 microsec
PAL_DLY	DO	#800,*+3		; Wait 8 usec for serial data xmit
	NOP
	RTS

; Let the host computer read the controller configuration
READ_CONTROLLER_CONFIGURATION
	MOVE	Y:<CONFIG,Y1		; Just transmit the configuration
	JMP	<FINISH1


; Set the video processor gain and integrator speed for all video boards
;  Command syntax is  SGN  #GAIN  #SPEED, #GAIN = 1, 2, 5 or 10	
;					  #SPEED = 0 for slow, 1 for fast
ST_GAIN	MOVE	X:(R3)+,A	; Gain value (1,2,5 or 10)
	MOVE	#>1,X0
	CMP	X0,A		; Check for gain = x1
	JNE	<STG2
	MOVE	#>$77,B
	JMP	<STG_A
STG2	MOVE	#>2,X0		; Check for gain = x2
	CMP	X0,A
	JNE	<STG5
	MOVE	#>$BB,B
	JMP	<STG_A
STG5	MOVE	#>5,X0		; Check for gain = x5
	CMP	X0,A
	JNE	<STG10
	MOVE	#>$DD,B
	JMP	<STG_A
STG10	MOVE	#>10,X0		; Check for gain = x10
	CMP	X0,A
	JNE	<ERROR
	MOVE	#>$EE,B

STG_A	MOVE	X:(R3)+,A	; Integrator Speed (0 for slow, 1 for fast)
	NOP
	JCLR	#0,A1,STG_B
	BSET	#8,B1
	NOP
	BSET	#9,B1
STG_B	MOVE	#$0C3C00,X0
	OR	X0,B
	NOP
	MOVE	B,Y:<GAIN	; Store the GAIN value for later use

; Send this same value to 15 video processor boards whether they exist or not
	MOVE	#$100000,X0	; Increment value
	MOVE	B,A
	DO	#15,STG_LOOP
	JSR	<XMIT_A_WORD	; Transmit A to TIM-A-STD
	JSR	<PAL_DLY	; Wait for SSI and PAL to be empty
	ADD	X0,B		; Increment the video processor board number
STG_LOOP

	JMP	<FINISH
ERR_SGN	MOVE	X:(R3)+,A
	JMP	<ERROR

; Set a particular DAC numbers, for setting DC bias voltages, clock driver  
;   voltages and video processor offset
; This is code for the ARC32 clock driver and ARC45 CCD video processor
;
; SBN  #BOARD  #DAC  ['CLK' or 'VID'] voltage
;
;				#BOARD is from 0 to 15
;				#DAC number
;				#voltage is from 0 to 4095

SET_BIAS_NUMBER			; Set bias number
	BSET	#3,X:PCRD	; Turn on the serial clock
	MOVE	X:(R3)+,A	; First argument is board number, 0 to 15
	REP	#20
	LSL	A
	NOP
	MOVE	A,X1		; Save the board number
	MOVE	X:(R3)+,A	; Second argument is DAC number
	NOP
	MOVE	A1,Y:0		; Save the DAC number for a little while
	MOVE	X:(R3),B	; Third argument is 'VID' or 'CLK' string
	CMP	#'VID',B
	JNE	<CLK_DRV
	REP	#14
	LSL	A
	NOP
	BSET	#19,A1		; Set bits to mean video processor DAC
	NOP
	BSET	#18,A1
	JMP	<BD_SET
CLK_DRV	CMP	#'CLK',B
	JNE	<ERR_SBN

; For ARC32 do some trickiness to set the chip select and address bits
	MOVE	A1,B
	REP	#14
	LSL	A
	MOVE	#$0E0000,X0
	AND	X0,A
	MOVE	#>7,X0
	AND	X0,B		; Get 3 least significant bits of clock #
	CMP	#0,B
	JNE	<CLK_1
	BSET	#8,A
	JMP	<BD_SET
CLK_1	CMP	#1,B
	JNE	<CLK_2
	BSET	#9,A
	JMP	<BD_SET
CLK_2	CMP	#2,B
	JNE	<CLK_3
	BSET	#10,A
	JMP	<BD_SET
CLK_3	CMP	#3,B
	JNE	<CLK_4
	BSET	#11,A
	JMP	<BD_SET
CLK_4	CMP	#4,B
	JNE	<CLK_5
	BSET	#13,A
	JMP	<BD_SET
CLK_5	CMP	#5,B
	JNE	<CLK_6
	BSET	#14,A
	JMP	<BD_SET
CLK_6	CMP	#6,B
	JNE	<CLK_7
	BSET	#15,A
	JMP	<BD_SET
CLK_7	CMP	#7,B
	JNE	<BD_SET
	BSET	#16,A

BD_SET	OR	X1,A		; Add on the board number
	NOP
	MOVE	A,X0
	MOVE	X:(R3)+,B	; Third argument (again) is 'VID' or 'CLK' string
	CMP	#'VID',B
	JEQ	<VID
	MOVE	X:(R3)+,A	; Fourth argument is voltage value, 0 to $fff
	REP	#4
	LSR	A		; Convert 12 bits to 8 bits for ARC32
	MOVE	#>$FF,Y0	; Mask off just 8 bits
	AND	Y0,A
	OR	X0,A
	JMP	<XMT_SBN
VID	MOVE	X:(R3)+,A	; Fourth argument is voltage value for ARC45, 12 bits
	OR	X0,A

XMT_SBN	JSR	<XMIT_A_WORD	; Transmit A to TIM-A-STD
	JSR	<PAL_DLY	; Wait for the number to be sent
	BCLR	#3,X:PCRD	; Turn the serial clock off
	JMP	<FINISH
ERR_SBN	MOVE	X:(R3)+,A	; Read and discard the fourth argument
	BCLR	#3,X:PCRD	; Turn the serial clock off
	JMP	<ERROR

; Specify the MUX value to be output on the clock driver board
; Command syntax is  SMX  #clock_driver_board #MUX1 #MUX2
;				#clock_driver_board from 0 to 15
;				#MUX1, #MUX2 from 0 to 23

SET_MUX	MOVE	X:(R3)+,A	; Clock driver board number
	REP	#20
	LSL	A
	MOVE	#$003000,X0
	OR	X0,A
	NOP
	MOVE	A,X1		; Move here for storage

; Get the first MUX number
	MOVE	X:(R3)+,A	; Get the first MUX number
	JLT	ERR_SM1
	MOVE	#>24,X0		; Check for argument less than 32
	CMP	X0,A
	JGE	ERR_SM1
	MOVE	A,B
	MOVE	#>7,X0
	AND	X0,B
	MOVE	#>$18,X0
	AND	X0,A
	JNE	<SMX_1		; Test for 0 <= MUX number <= 7
	BSET	#3,B1
	JMP	<SMX_A
SMX_1	MOVE	#>$08,X0
	CMP	X0,A		; Test for 8 <= MUX number <= 15
	JNE	<SMX_2
	BSET	#4,B1
	JMP	<SMX_A
SMX_2	MOVE	#>$10,X0
	CMP	X0,A		; Test for 16 <= MUX number <= 23
	JNE	<ERR_SM1
	BSET	#5,B1
SMX_A	OR	X1,B1		; Add prefix to MUX numbers
	NOP
	MOVE	B1,Y1

; Add on the second MUX number
	MOVE	X:(R3)+,A	; Get the next MUX number
	JLT	<ERROR
	MOVE	#>24,X0		; Check for argument less than 32
	CMP	X0,A
	JGE	<ERROR
	REP	#6
	LSL	A
	NOP
	MOVE	A,B
	MOVE	#$1C0,X0
	AND	X0,B
	MOVE	#>$600,X0
	AND	X0,A
	JNE	<SMX_3		; Test for 0 <= MUX number <= 7
	BSET	#9,B1
	JMP	<SMX_B
SMX_3	MOVE	#>$200,X0
	CMP	X0,A		; Test for 8 <= MUX number <= 15
	JNE	<SMX_4
	BSET	#10,B1
	JMP	<SMX_B
SMX_4	MOVE	#>$400,X0
	CMP	X0,A		; Test for 16 <= MUX number <= 23
	JNE	<ERROR
	BSET	#11,B1
SMX_B	ADD	Y1,B		; Add prefix to MUX numbers
	NOP
	MOVE	B1,A
	JSR	<XMIT_A_WORD	; Transmit A to TIM-A-STD
	JSR	<PAL_DLY	; Delay for all this to happen
	JMP	<FINISH
ERR_SM1	MOVE	X:(R3)+,A
	JMP	<ERROR

; Specify subarray readout coordinates, one rectangle only
SET_SUBARRAY_SIZES
	CLR	A
	NOP	
	MOVE	A,Y:<NBOXES		; Number of subimage boxes = 0 to start
	MOVE    X:(R3)+,X0
	MOVE	X0,Y:<NR_BIAS		; Number of bias pixels to read
	MOVE    X:(R3)+,X0
	MOVE	X0,Y:<NS_READ		; Number of columns in subimage read
	MOVE    X:(R3)+,X0
	MOVE	X0,Y:<NP_READ		; Number of rows in subimage read	
	JMP	<FINISH

; Call this routine once for every subarray to be added to the table
SET_SUBARRAY_POSITIONS
	MOVE	Y:<NBOXES,X0
	MOVE	X:<THREE,X1
	MPY	X0,X1,A
	ASR	A
	MOVE	A0,A1
	MOVE	#>10,X0
	CMP	X0,A
	JGT	<ERROR		; Error if number of boxes > 10
	MOVE	#READ_TABLE,X0
	ADD	X0,A
	NOP
	MOVE	A1,R7
	MOVE	X:(R3)+,X0
	NOP
	NOP
	MOVE	X0,Y:(R7)+	; Number of rows (parallels) to clear
	MOVE	X:(R3)+,X0
	MOVE	X0,Y:(R7)+	; Number of columns (serials) clears before
	MOVE	X:(R3)+,X0	;  the box readout
	MOVE	X0,Y:(R7)+	; Number of columns (serials) clears after	
	MOVE	Y:<NBOXES,A	;  the box readout
	MOVE	X:<ONE,X0
	ADD	X0,A
	NOP
	MOVE	A,Y:<NBOXES
	JMP	<FINISH

; Generate the serial readout waveform table for the chosen
;   value of readout and serial binning.

GENERATE_SERIAL_WAVEFORM
	MOVE	#RESET_CLOCKS,R0
	MOVE	#(PXL_TBL+1),R1
	MOVE	Y:(R0)+,A
	NOP	
	DO	A1,L_RESET_CLOCKS
	MOVE	Y:(R0)+,X0
	MOVE	X0,Y:(R1)+
L_RESET_CLOCKS

; Generate the first set of clocks
	MOVE	Y:FIRST_CLOCKS,R0
	NOP
	NOP
	MOVE	Y:(R0)+,A
	NOP
	DO	A1,L_FIRST_CLOCKS
	MOVE	Y:(R0)+,X0
	MOVE	X0,Y:(R1)+
L_FIRST_CLOCKS

	
; Generate the binning waveforms if needed
	MOVE	Y:<NSBIN,A
	SUB	#1,A
	JLE	<GEN_VID		
	DO	A1,L_BIN
	MOVE	Y:CLOCK_LINE,R0
	NOP
	NOP
	MOVE	Y:(R0)+,A
	NOP
	DO	A1,L_CLOCK_LINE
	MOVE	Y:(R0)+,X0
	MOVE	X0,Y:(R1)+
L_CLOCK_LINE
	NOP
L_BIN

; Generate the video processor waveforms
GEN_VID	MOVE	#VPROC,R0
	MOVE	R1,A			; Store the address where the SXMIT
	ADD	#1,A			;   command is located for later use
	NOP				;   in skipping pre-scan pixels
	MOVE	A1,Y:<SXMIT_ADR 
	DO	Y:(R0)+,L_VID
	MOVE	Y:(R0)+,X0	
	MOVE	X0,Y:(R1)+
L_VID

; Finally, calculate the number of entries in the waveform table just generated
	MOVE	#PXL_TBL,X0
	MOVE	X0,R0
	MOVE	R1,A
	SUB	X0,A
	SUB	#1,A
	NOP
	MOVE	A1,Y:(R0)
	RTS

; Select which readouts to process
;   'SOS'  Amplifier_name  
;	Amplifier_name = '__L', '__R', or '_LR'

SELECT_OUTPUT_SOURCE
	MOVE    X:(R3)+,Y0
	MOVE	Y0,Y:<OS
	JSR	<SEL_OS
	JMP	<FINISH
	
SEL_OS	MOVE	Y:<OS,Y0
	MOVE	#'__L',A		; Left Amplifier = readout #0
	CMP	Y0,A
	JNE	<COMP_R

	MOVE	#FIRST_CLOCKS_LEFT,X0
	MOVE	X0,Y:FIRST_CLOCKS
	MOVE	#CLOCK_LINE_LEFT,X0
	MOVE	X0,Y:CLOCK_LINE
	MOVE    Y:LAST_CLOCK_LEFT,X0
	MOVE	X0,Y:LAST_CK
	MOVE	#$00F000,X0
	MOVE	X0,Y:SXMIT
	MOVE	Y:CHARGE_DUMP_LEFT,X0
	MOVE	X0,Y:CH_DUMP

	MOVE	#SERIAL_SKIP_LEFT,X0
	MOVE	X0,Y:SERIAL_SKIP
	BCLR	#SPLIT_S,X:STATUS
	RTS

COMP_R	MOVE	#'__R',A		; Right Amplifier = readout #1
	CMP	Y0,A
	JNE	<COMP_LR

	MOVE	#FIRST_CLOCKS_RIGHT,X0
	MOVE	X0,Y:FIRST_CLOCKS
	MOVE	#CLOCK_LINE_RIGHT,X0
	MOVE	X0,Y:CLOCK_LINE
	MOVE    Y:LAST_CLOCK_RIGHT,X0
	MOVE	X0,Y:LAST_CK
	MOVE	#$00F041,X0
	MOVE	X0,Y:SXMIT
	MOVE	Y:CHARGE_DUMP_RIGHT,X0
	MOVE	X0,Y:CH_DUMP

	MOVE	#SERIAL_SKIP_RIGHT,X0
	MOVE	X0,Y:SERIAL_SKIP
	BCLR	#SPLIT_S,X:STATUS
	RTS

COMP_LR	MOVE	#'_LR',A		; Both Amplifiers = readout #0, #1
	CMP	Y0,A
	JNE	<ERROR

	MOVE	#FIRST_CLOCKS_SPLIT,X0
	MOVE	X0,Y:FIRST_CLOCKS
	MOVE	#CLOCK_LINE_SPLIT,X0
	MOVE	X0,Y:CLOCK_LINE
	MOVE    Y:LAST_CLOCK_SPLIT,X0
	MOVE	X0,Y:LAST_CK
	MOVE	#$00F040,X0
	MOVE	X0,Y:SXMIT
	MOVE	Y:CHARGE_DUMP_SPLIT,X0
	MOVE	X0,Y:CH_DUMP

	MOVE	#SERIAL_SKIP_SPLIT,X0
	MOVE	X0,Y:SERIAL_SKIP
	BSET	#SPLIT_S,X:STATUS
	RTS

; Set the integrator time in units of 40 nanosec
SET_INTEGRATOR_TIME
	MOVE	X:(R3)+,A		; Get the number of 40 nanosec delays
	LSL	#16,A
	NOP
	MOVE	A1,X0
	MOVE	Y:RST_INT,A
	AND	#$00FFFF,A
	ADD	X0,A
	NOP
	MOVE	A1,Y:RST_INT
	MOVE	Y:SIG_INT,A
	AND	#$00FFFF,A
	ADD	X0,A
	NOP
	MOVE	A1,Y:SIG_INT
	JMP	<FINISH

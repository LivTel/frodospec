       COMMENT *

This file is used to generate boot DSP code for the 250 MHz fiber optic
	timing board using a DSP56303 as its main processor. It supports 
	split serial and frame transfer, but not split parallel nor binning.
	*
	PAGE    132     ; Printronix page width - 132 columns

; Include the boot and header files so addressing is easy
	INCLUDE "timhdr.asm"
	INCLUDE	"timboot.asm"

	ORG	P:,P:

CC	EQU	CCDVIDREV5+TIMREV5+TEMP_POLY+UTILREV3+SPLIT_SERIAL+SUBARRAY+BINNING+SHUTTER_CC

; Put number of words of application in P: for loading application from EEPROM
	DC	TIMBOOT_X_MEMORY-@LCV(L)-1

; Define CLOCK as a macro to produce in-line code to reduce execution time
CLOCK	MACRO
	JCLR	#SSFHF,X:HDR,*		; Don't overfill the WRSS FIFO
	REP	Y:(R0)+			; Repeat
	MOVEP	Y:(R0)+,Y:WRSS		; Write the waveform to the FIFO
	ENDM
	
; Set software to IDLE mode
START_IDLE_CLOCKING
	MOVE	#IDLE,R0		; Exercise clocks when idling
	MOVE	R0,X:<IDL_ADR
	BSET	#IDLMODE,X:<STATUS	; Idle after readout
	JMP     <FINISH			; Need to send header and 'DON'

; Keep the CCD idling when not reading out
IDLE	DO      Y:<NSR,IDL1     	; Loop over number of pixels per line
	MOVE    #SERIAL_IDLE,R0 	; Serial transfer on pixel
	CLOCK  				; Go to it
	MOVE	#COM_BUF,R3
	JSR	<GET_RCV		; Check for FO or SSI commands
	JCC	<NO_COM			; Continue IDLE if no commands received
	ENDDO
	JMP     <PRC_RCV		; Go process header and command
NO_COM	NOP
IDL1
	MOVE    #PARALLEL_CLEAR,R0	; Address of parallel clocking waveform
	CLOCK  				; Go clock out the CCD charge
	JMP     <IDLE

;  *****************  Exposure and readout routines  *****************

; Overall loop - transfer and read NPR lines
RDCCD

; Calculate some readout parameters
	MOVE	Y:<NBOXES,A		; NBOXES = 0 => full image readout
	TST	A
	JNE	<SUB_IMG
	MOVE	A1,Y:<NP_SKIP		; Zero these all out
	MOVE	A1,Y:<NS_SKP1
	MOVE	A1,Y:<NS_SKP2
	MOVE	Y:<NSR,A		; NSERIALS_READ = NSR
	JCLR	#SPLIT_S,X:STATUS,*+3
	ASR	A			; Split serials requires / 2
	NOP
	MOVE	A,Y:<NSERIALS_READ	; Number of columns in each subimage
	JMP	<WT_CLK

; Loop over the required number of subimage boxes
SUB_IMG	MOVE	#READ_TABLE,R7		; Parameter table for subimage readout
	DO	Y:<NBOXES,L_NBOXES	; Loop over number of boxes
	MOVE	Y:(R7)+,X0
	MOVE	X0,Y:<NP_SKIP
	MOVE	Y:(R7)+,X0
	MOVE	Y:<NSBIN,X1		; Multiply by serial binning number
	MPY	X0,X1,A
	ASR	A	
	MOVE	A0,Y:<NS_SKP1
	MOVE	Y:(R7)+,X0
	MOVE	Y:<NSBIN,X1		; Multiply by serial binning number
	MPY	X0,X1,A
	ASR	A
	MOVE	A0,Y:<NS_SKP2
	MOVE	Y:<NS_READ,A
	JCLR	#SPLIT_S,X:STATUS,*+3	; Split serials require / 2
	ASR	A
	NOP
	MOVE	A,Y:<NSERIALS_READ	; Number of columns in each subimage

; Start the loop for parallel shifting desired number of lines
WT_CLK	JSR	<GENERATE_SERIAL_WAVEFORM
	JSR	<WAIT_TO_FINISH_CLOCKING

; Skip over the required number of rows for subimage readout
	MOVE	Y:<NP_SKIP,A		; Number of rows to skip
	TST	A
	JEQ	<CLR_SR
	DO      Y:<NP_SKIP,L_PSKP	
	DO	Y:<NPBIN,L_PSKIP
	MOVE    #<PARALLEL_CLEAR,R0
	CLOCK
L_PSKIP	NOP
L_PSKP

; Clear out the accumulated charge from the serial shift register 
CLR_SR	DO      Y:<NSCLR,L_CLRSR	; Loop over number of pixels to skip
	MOVE    #<SERIALS_CLEAR,R0      ; Address of serial skipping waveforms
	CLOCK    		      	; Go clock out the CCD charge
L_CLRSR		                     	; Do loop restriction  

; Parallel shift the image into the serial shift register
	MOVE	Y:<NPR,A		; Number of rows set by host computer
	MOVE	Y:<NBOXES,B		; NBOXES = 0 => full image readout
	TST	B
	JEQ	*+2
	MOVE	Y:<NP_READ,A		; If NBOXES .NE. 0 use subimage table
	NOP

; This is the main loop over each line to be read out
	DO      A1,LPR			; Number of rows to read out

; Exercise the parallel clocks, including binning if needed
	DO	Y:<NPBIN,L_PBIN
	MOVE    #<PARALLEL,R0
	CLOCK
L_PBIN

; Check for a command once per line. Only the ABORT command should be issued.
	MOVE	#COM_BUF,R3
	JSR	<GET_RCV		; Was a command received?
	JCC	<CONTINUE_READ		; If no, continue reading out
	JMP	<PRC_RCV		; If yes, go process it

; Abort the readout currently underway
ABR_RDC	JCLR	#ST_RDC,X:<STATUS,ABORT_EXPOSURE
	ENDDO				; Properly terminate readout loop
	MOVE	Y:<NBOXES,A		; NBOXES = 0 => full image readout
	TST	A
	JEQ	*+2
	ENDDO				; Properly terminate readout loop
	JMP	<ABORT_EXPOSURE

; Skip over NS_SKP1 columns for subimage readout
CONTINUE_READ
	MOVE	Y:<NS_SKP1,A		; Number of columns to skip
	TST	A
	JLE	<L_SKIP
	DO	Y:<NS_SKP1,L_SKP1	; Number of waveform entries total
	MOVE	Y:<SERIAL_SKIP,R0	; Waveform table starting address
	CLOCK  				; Go clock out the CCD charge
L_SKP1

; Skip over the pre-scan pixels by not transmitting A/D data in PXL_TBL
L_SKIP	MOVE	Y:<SXMIT_ADR,R5	
	NOP
	MOVE	#>%0010111,X0
	MOVE	X0,Y:(R5)		; Overwrite SXMIT with a benign value
	DO	#53,L_PRE_SKIP		; 50 CCD pre-skip, 2 A/D pipeline 
	MOVE	#PXL_TBL,R0		;   + 1 waveform table
	CLOCK		
L_PRE_SKIP
	MOVE	Y:SXMIT,X0		; Restore SXMIT
	MOVE	X0,Y:(R5)

; Finally read some real pixels
	DO	Y:<NSERIALS_READ,L_RD
	MOVE	#PXL_TBL,R0
	CLOCK  				; Go clock out the CCD charge
L_RD

; Skip over NS_SKP2 columns if needed for subimage readout
	MOVE	Y:<NS_SKP2,A		; Number of columns to skip
	TST	A
	JLE	<L_BIAS
	DO	Y:<NS_SKP2,L_SKP2
	MOVE	Y:<SERIAL_SKIP,R0	; Waveform table starting address
	CLOCK  				; Go clock out the CCD charge
L_SKP2

; And read the bias pixels if in subimage readout mode
L_BIAS	MOVE	Y:<NBOXES,A		; NBOXES = 0 => full image readout
	TST	A
	JLE	<END_ROW
	MOVE	Y:<NR_BIAS,A		; NR_BIAS = 0 => no bias pixels
	TST	A
	JLE	<END_ROW
	JCLR	#SPLIT_S,X:STATUS,*+3
	ASR	A			; Split serials require / 2
	NOP
	DO      A1,L_BRD		; Number of pixels to read out
	MOVE	#PXL_TBL,R0
	CLOCK  				; Go clock out the CCD charg
L_BRD	NOP
END_ROW	NOP
LPR	NOP				; End of parallel loop
L_NBOXES NOP				; End of subimage boxes loop

; Restore the controller to non-image data transfer and idling if necessary
RDC_END	JCLR	#IDLMODE,X:<STATUS,NO_IDL ; Don't idle after readout
	MOVE	#IDLE,R0
	MOVE	R0,X:<IDL_ADR
	JMP	<RDC_E
NO_IDL	MOVE	#TST_RCV,R0
	MOVE	R0,X:<IDL_ADR
RDC_E	JSR	<WAIT_TO_FINISH_CLOCKING
	BCLR	#ST_RDC,X:<STATUS	; Set status to not reading out
        JMP     <START

; ******  Include many routines not directly needed for readout  *******
	INCLUDE "timCCDmisc.asm"


TIMBOOT_X_MEMORY	EQU	@LCV(L)

;  ****************  Setup memory tables in X: space ********************

; Define the address in P: space where the table of constants begins

	IF	@SCP("DOWNLOAD","HOST")
	ORG     X:END_COMMAND_TABLE,X:END_COMMAND_TABLE
	ENDIF

	IF	@SCP("DOWNLOAD","ROM")
	ORG     X:END_COMMAND_TABLE,P:
	ENDIF

; Application commands
	DC	'PON',POWER_ON
	DC	'POF',POWER_OFF
	DC	'SBV',SET_BIAS_VOLTAGES
	DC	'IDL',START_IDLE_CLOCKING
	DC	'OSH',OPEN_SHUTTER
	DC	'CSH',CLOSE_SHUTTER
	DC	'RDC',RDCCD 			; Begin CCD readout    
	DC	'CLR',CLEAR  			; Fast clear the CCD   

; Exposure and readout control routines
	DC	'SET',SET_EXPOSURE_TIME
	DC	'RET',READ_EXPOSURE_TIME
	DC	'SEX',START_EXPOSURE
	DC	'PEX',PAUSE_EXPOSURE
	DC	'REX',RESUME_EXPOSURE
	DC	'AEX',ABORT_EXPOSURE
	DC	'ABR',ABR_RDC
	DC	'CRD',CONTINUE_READ

; Support routines
	DC	'SGN',ST_GAIN      
	DC	'SBN',SET_BIAS_NUMBER
	DC	'SMX',SET_MUX
	DC	'CSW',CLR_SWS
	DC	'SOS',SELECT_OUTPUT_SOURCE
	DC	'SSS',SET_SUBARRAY_SIZES
	DC	'SSP',SET_SUBARRAY_POSITIONS
	DC	'RCC',READ_CONTROLLER_CONFIGURATION 
	DC	'SIT',SET_INTEGRATOR_TIME
	
END_APPLICATON_COMMAND_TABLE	EQU	@LCV(L)

	IF	@SCP("DOWNLOAD","HOST")
NUM_COM			EQU	(@LCV(R)-COM_TBL_R)/2	; Number of boot + 
							;  application commands
EXPOSING		EQU	CHK_TIM			; Address if exposing
CONTINUE_READING	EQU	CONT_RD 		; Address if reading out
	ENDIF

	IF	@SCP("DOWNLOAD","ROM")
	ORG     Y:0,P:
	ENDIF

; Now let's go for the timing waveform tables
	IF	@SCP("DOWNLOAD","HOST")
        ORG     Y:0,Y:0
	ENDIF

GAIN	DC	END_APPLICATON_Y_MEMORY-@LCV(L)-1

NSR     DC      1200   	 	; Number Serial Read, prescan + image + bias
NPR     DC      0	     	; Number Parallel Read
NSCLR	DC      NS_CLR  	; To clear the serial register
NPCLR   DC      NP_CLR    	; To clear the parallel register 
NSBIN   DC      1       	; Serial binning parameter
NPBIN   DC      1       	; Parallel binning parameter
TST_DAT	DC	0		; Temporary definition for test images
SHDEL	DC	SH_DEL		; Delay in milliseconds between shutter  
				;   closing and image readout
SXMIT_ADR DC	0		; Address where SXMIT command is located
CONFIG	DC	CC		; Controller configuration
NSERIALS_READ	DC	0	; Number of serials to read

; Waveform parameters
OS		DC	'_LR'	; Output source 
FIRST_CLOCKS	DC	0	; Address of first clocks waveforms
CLOCK_LINE	DC	0	; Clock one complete line of charge

; Readout peculiarity parameters. Default values are selected here.
SERIAL_SKIP 	DC	SERIAL_SKIP_SPLIT	; Serial skipping waveforms

; These three parameters are read from the READ_TABLE when needed by the
;   RDCCD routine as it loops through the required number of boxes
NP_SKIP		DC	0	; Number of rows to skip
NS_SKP1		DC	0	; Number of serials to clear before read
NS_SKP2		DC	0	; Number of serials to clear after read

; Subimage readout parameters.
NBOXES	DC	0		; Number of boxes to read
NR_BIAS	DC	0		; Number of bias pixels to read
NS_READ	DC	0		; Number of columns in subimage read
NP_READ	DC	0		; Number of rows in subimage read
READ_TABLE DC	0,0,0		; #1 = Number of rows to clear 
				; #2 = Number of columns to skip before 
				;   subimage read 
				; #3 = Number of rows to clear after 
				;   subimage clear

; Include the waveform table for the designated type of CCD
	INCLUDE "WAVEFORM_FILE" ; Readout and clocking waveform file

END_APPLICATON_Y_MEMORY	EQU	@LCV(L)

; End of program
	END


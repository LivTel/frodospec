/* ccd_dsp.c -*- mode: Fundamental;-*-
** ccd library
** $Header: /home/cjm/cvs/frodospec/ccd/c/ccd_dsp.c,v 0.6 2000-02-23 11:56:16 cjm Exp $
*/
/**
 * ccd_dsp.c contains all the SDSU CCD Controller commands. Commands are passed to the 
 * controller using the <a href="ccd_interface.html">CCD_Interface_</a> calls.
 * @author SDSU, Chris Mottram
 * @version $Revision: 0.6 $
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include "ccd_interface.h"
#include "ccd_pci.h"
#include "ccd_dsp.h"
#ifdef CFITSIO
#include "fitsio.h"
#endif

/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id: ccd_dsp.c,v 0.6 2000-02-23 11:56:16 cjm Exp $";

/* defines */
/**
 * The maximum memory address of code that isn't boot code. This is used in 
 * <a href="#CCD_DSP_Download">CCD_DSP_Download</a> so that only application code is downloaded
 * and not the boot code bundled with it.
 * @see #CCD_DSP_Download
 */
#define	DSP_DOWNLOAD_ADDR_MAX		0x4000 /* maximum address of code that isn't boot code */
/**
 * Special value to pass into <a href="#DSP_Get_Reply">DSP_Get_Reply</a> as the expected reply parameter, indicating
 * that the reply from the command should be an actual value rather then (usually)
 * <a href="#CCD_DSP_DON">DON</a>.
 */
#define	DSP_ACTUAL_VALUE 		-1 /* flag indicating return value of DSP command is to be returned as data */

/**
 * Amount of time, in milleseconds, remaining for an exposure when we stop sleeping and tell the
 * interface to enter readout mode.
 */
#define READOUT_REMAINING_TIME	(1000)

/* data types */
/**
 * Data type used to hold local data to ccd_dsp.
 */
typedef struct DSP_Attr_Struct
{
/* whether the user has asked the current operation to stop 
** This is volatile as a different thread may change this variable. */
	volatile int Abort;
/* whether the library is performing an exposure and it's status */
	enum CCD_DSP_EXPOSURE_STATUS Exposure_Status;
} DSP_ATTR_T;

/* external variables */

/* internal variables */
/**
 * Variable holding error code of last operation performed by ccd_dsp.
 */
static int DSP_Error_Number = 0;
/**
 * Internal  variable holding description of the last error that occured.
 */
static char DSP_Error_String[CCD_GLOBAL_ERROR_STRING_LENGTH] = "";
/**
 * Data holding the current status of ccd_dsp.
 */
static DSP_ATTR_T DSP_Data;

/* internal functions */
static int DSP_Send_Lda(enum CCD_DSP_BOARD_ID board_id,int data);
static int DSP_Send_Wrm(enum CCD_DSP_BOARD_ID board_id,enum CCD_DSP_MEM_SPACE mem_space,int address,int data);
static int DSP_Send_Rdm(enum CCD_DSP_BOARD_ID board_id,enum CCD_DSP_MEM_SPACE mem_space,int address);
static int DSP_Send_Tdl(enum CCD_DSP_BOARD_ID board_id,int data);

static int DSP_Send_Abr(void);
static int DSP_Send_Clr(void);
static int DSP_Send_Idl(void);
static int DSP_Send_Rdi(void);
static int DSP_Send_Sbv(void);
static int DSP_Send_Sgn(enum CCD_DSP_GAIN gain,int speed);
static int DSP_Send_Stp(void);
static int DSP_Send_Set_NCols(int ncols);
static int DSP_Send_Set_NRows(int nrows);

static int DSP_Send_Aex(void);
static int DSP_Send_Csh(void);
static int DSP_Send_Osh(void);
static int DSP_Send_Pex(void);
static int DSP_Send_Pon(void);
static int DSP_Send_Pof(void);
static int DSP_Send_Read_Temperature(void);
static int DSP_Send_Set_Temperature(int adu);
static int DSP_Send_Rex(void);
static int DSP_Send_Sex(void);
static int DSP_Send_Reset(void);
static int DSP_Send_Set_Util_Options(int bit_value);
static int DSP_Send_Set_Exposure_Time(int msecs);

static int DSP_Clear_Reply_Memory(void);
static int DSP_Set_Destination(enum CCD_DSP_BOARD_ID board_id,int argument_count);
static int DSP_Send_Manual_Command(int cmdr_command,int *argument_list,int argument_count);
static int DSP_Send_Command(int hcvr_command,int *argument_list,int argument_count);
static int DSP_Get_Reply(int expected_reply);

static int DSP_Read_Line(FILE *fp, char *buff);
static int DSP_Get_Download_Type(FILE *fp);
static int DSP_Address_Char_To_Mem_Space(char ch,enum CCD_DSP_MEM_SPACE *mem_space);
static int DSP_Process_Data(FILE *download_fp,enum CCD_DSP_BOARD_ID board_id,
	enum CCD_DSP_MEM_SPACE mem_space,int addr);
static int DSP_Image_Transfer(int ncols,int nrows,int numbytes,
	enum CCD_DSP_DEINTERLACE_TYPE deinterlace_type,char *filename);
static unsigned short *DSP_DeInterlace(int ncols,int nrows,unsigned short *old_iptr,
	enum CCD_DSP_DEINTERLACE_TYPE deinterlace_type);
static int DSP_Save(char *filename,char *exposure_data,int ncols,int nrows,int number_bytes);

/* external functions */

/**
 * This routine sets up ccd_dsp internal variables.
 * It should be called at startup.
 */
void CCD_DSP_Initialise(void)
{
	DSP_Error_Number = 0;
	DSP_Data.Abort = FALSE;
	DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_NONE;
}

/* Boot commands */
/**
 * This routine executes the LoaD Application (LDA) command on a 
 * SDSU Controller board. This
 * causes some DSP application code to be loaded from (EEP)ROM into DSP memory for the controller to execute.
 * @param board_id The SDSU CCD Controller board to send the command to, one of 
 * 	CCD_DSP_INTERFACE_BOARD_ID(interface),
 *	CCD_DSP_TIM_BOARD_ID(timing board) or CCD_DSP_UTIL_BOARD_ID(utility board).
 * @param application_number The number of the application on (EEP)ROM to load.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Lda
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_LDA(enum CCD_DSP_BOARD_ID board_id,int application_number)
{
	DSP_Error_Number = 0;
	/* check - is board_id a legal value */
	if(!CCD_DSP_IS_BOARD_ID(board_id))
	{
		DSP_Error_Number = 1;
		sprintf(DSP_Error_String,"CCD_DSP_Command_LDA:Illegal board ID '%d'.",
			board_id);
		return FALSE;
	}
	if(application_number < 0)
	{
		DSP_Error_Number = 2;
		sprintf(DSP_Error_String,"CCD_DSP_Command_LDA:Illegal application number '%d'.",
			application_number);
		return FALSE;
	}
	if(!DSP_Send_Lda(board_id,application_number))
		return FALSE;
	/* get reply - should be DON */
	return(DSP_Get_Reply(CCD_DSP_DON));
}

/**
 * This routine executes the ReaD Memory (RDM) command on a SDSU Controller board. 
 * This
 * gets the value of a word of memory, location specified by board,memory space and address, and returns
 * it's value.
 * @param board_id The SDSU CCD Controller board to send the command to, one of 
 * 	CCD_DSP_INTERFACE_BOARD_ID(interface),
 *	CCD_DSP_TIM_BOARD_ID(timing board) or CCD_DSP_UTIL_BOARD_ID(utility board).
 * @param mem_space The memory space on board board_id to read from, of type 
 * <a href="#CCD_DSP_MEM_SPACE">CCD_DSP_MEM_SPACE</a>. One of:
 * 	CCD_DSP_MEM_SPACE_D(DRAM),
 * 	CCD_DSP_MEM_SPACE_P(program),
 * 	CCD_DSP_MEM_SPACE_X(X data),
 * 	CCD_DSP_MEM_SPACE_Y(Y data)
 * 	or CCD_DSP_MEM_SPACE_R(ROM).
 * @param address The memory address to read from.
 * @return The routine returns the actual value at the memory location or zero if an error occurs. If zero is
 * 	returned this can either mean that memory address contains zero OR an error occured. It can be
 *	determined properly if an error occured by looking at 
 * 	<a href="#DSP_Error_Number">DSP_Error_Number</a>, it it is zero the memory location contains zero,
 * 	if it is non-zero than an error occured.
 * @see #DSP_Send_Rdm
 * @see #DSP_Get_Reply
 * @see #DSP_ACTUAL_VALUE
 */
int CCD_DSP_Command_RDM(enum CCD_DSP_BOARD_ID board_id,enum CCD_DSP_MEM_SPACE mem_space,int address)
{
	DSP_Error_Number = 0;
	/* check - is board_id a legal value */
	if(!CCD_DSP_IS_BOARD_ID(board_id))
	{
		DSP_Error_Number = 3;
		sprintf(DSP_Error_String,"CCD_DSP_Command_RDM:Illegal board ID '%d'.",
			board_id);
		return FALSE;
	}
	if(!CCD_DSP_IS_MEMORY_SPACE(mem_space))
	{
		DSP_Error_Number = 4;
		sprintf(DSP_Error_String,"CCD_DSP_Command_RDM:Illegal memory space '%c'.",mem_space);
		return FALSE;
	}
	if(address < 0)
	{
		DSP_Error_Number = 5;
		sprintf(DSP_Error_String,"CCD_DSP_Command_RDM:Illegal address '%#x'.",address);
		return FALSE;
	}
	if(!DSP_Send_Rdm(board_id,mem_space,address))
		return FALSE;
	/* get reply - actual value of memory location returned */
	return(DSP_Get_Reply(DSP_ACTUAL_VALUE));
}

/**
 * This routine Tests the Data Link (TDL) on a SDSU Controller board. 
 * This ensures the host computer can communicate with the board.
 * @param board_id The SDSU CCD Controller board to send the command to, one of 
 * 	CCD_DSP_INTERFACE_BOARD_ID(interface),
 *	CCD_DSP_TIM_BOARD_ID(timing board) or CCD_DSP_UTIL_BOARD_ID(utility board).
 * @param data The data value to send to the boards to test the data connection.
 * @return The routine returns the data value if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Tdl
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_TDL(enum CCD_DSP_BOARD_ID board_id,int data)
{
	DSP_Error_Number = 0;
	/* check - is board_id a legal value */
	if(!CCD_DSP_IS_BOARD_ID(board_id))
	{
		DSP_Error_Number = 6;
		sprintf(DSP_Error_String,"CCD_DSP_Command_TDL:Illegal board ID '%d'.",board_id);
		return FALSE;
	}
	if(!DSP_Send_Tdl(board_id,data))
		return FALSE;
	/* get reply - data value sent should be returned */
	return(DSP_Get_Reply(data));
}

/**
 * This routine executes the WRite Memory (WRM) command on a SDSU Controller board.
 * This sets the value of a word of memory, it's location specified by board,memory space and address.
 * @param board_id The SDSU CCD Controller board to send the command to, one of 
 * 	CCD_DSP_INTERFACE_BOARD_ID(interface),
 *	CCD_DSP_TIM_BOARD_ID(timing board) or CCD_DSP_UTIL_BOARD_ID(utility board).
 * @param mem_space The memory space on board board_id to read from, of type 
 * <a href="#CCD_DSP_MEM_SPACE">CCD_DSP_MEM_SPACE</a>. One of:
 * 	CCD_DSP_MEM_SPACE_D(DRAM),
 * 	CCD_DSP_MEM_SPACE_P(program),
 * 	CCD_DSP_MEM_SPACE_X(X data),
 * 	CCD_DSP_MEM_SPACE_Y(Y data)
 * 	or CCD_DSP_MEM_SPACE_R(ROM).
 * @param address The memory address to write data to.
 * @param data The data value to write to the memory address.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Wrm
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_WRM(enum CCD_DSP_BOARD_ID board_id,enum CCD_DSP_MEM_SPACE mem_space,int address,int data)
{
	DSP_Error_Number = 0;
	/* check - is board_id a legal value */
	if(!CCD_DSP_IS_BOARD_ID(board_id))
	{
		DSP_Error_Number = 7;
		sprintf(DSP_Error_String,"CCD_DSP_Command_WRM:Illegal board ID '%d'.",board_id);
		return FALSE;
	}
	if(!CCD_DSP_IS_MEMORY_SPACE(mem_space))
	{
		DSP_Error_Number = 8;
		sprintf(DSP_Error_String,"CCD_DSP_Command_WRM:Illegal memory space '%c'.",mem_space);
		return FALSE;
	}
	if(address < 0)
	{
		DSP_Error_Number = 9;
		sprintf(DSP_Error_String,"CCD_DSP_Command_WRM:Illegal address '%#x'.",address);
		return FALSE;
	}
	if(!DSP_Send_Wrm(board_id,mem_space,address,data))
		return FALSE;
	/* get reply - DON should be returned */
	return(DSP_Get_Reply(CCD_DSP_DON));
}

/* timing board commands */
/**
 * This routine executes the ABort Readout (ABR) command on a SDSU Controller board.
 * If the SDSU CCD Controller is currently reading out the CCD, it is stopped immediately.
 * Unlike most commands, this one does not wait for a DON message to be returned from the controller,
 * as the interface will be returning data when this request is made.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Abr
 */
int CCD_DSP_Command_ABR(void)
{
	DSP_Error_Number = 0;
	if(!DSP_Send_Abr())
		return FALSE;
	/* don't wait for a reply, the PCI interface should be returning data 
	** to us when this request was made */
	return TRUE;
}

/**
 * This routine executes the CLeaR (CLR) command on a SDSU Controller board. This
 * clocks out any stored charge on the CCD, leaving the CCD ready for an exposure.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Clr
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_CLR(void)
{
	int retval;

	DSP_Error_Number = 0;
	DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_CLEAR;
	if(!DSP_Send_Clr())
	{
		DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_NONE;
		return FALSE;
	}
	/* get reply - DON should be returned */
	retval = DSP_Get_Reply(CCD_DSP_DON);
	DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_NONE;
	return retval;
}

/**
 * This routine executes the IDLe (IDL) command on a SDSU Controller board. This
 * puts the clocks in the readout sequence, but does not transfer the clocked data to prevent charge from 
 * building up on the CCD.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #CCD_DSP_Command_STP
 * @see #DSP_Send_Idl
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_IDL(void)
{
	DSP_Error_Number = 0;
	if(!DSP_Send_Idl())
		return FALSE;
	/* get reply - DON should be returned */
	return(DSP_Get_Reply(CCD_DSP_DON));
}

/**
 * This routine executes the ReaDout Ccd (RDC) command on a SDSU Controller board. 
 * This reads out the data on the CCD.
 * @param ncols The number of columns in the image. This must be a positive non-zero integer.
 * @param nrows The number of rows in the image to be readout from the CCD. This must be a positive non-zero integer.
 * @param numbytes The number of bytes to read out from the CCD. This should be (nrows * ncols * 
 * 	<a href="ccd_global.html#CCD_GLOBAL_BYTES_PER_PIXEL">CCD_GLOBAL_BYTES_PER_PIXEL</a>).
 * @param deinterlace_type The algorithm to use for deinterlacing the resulting data. The data needs to be
 * 	deinterlaced if the CCD is read out from multiple readouts. One of 
 * 	<a href="ccd_setup.html#CCD_DSP_DEINTERLACE_TYPE">CCD_DSP_DEINTERLACE_TYPE</a>:
 *	CCD_DSP_DEINTERLACE_SINGLE,
 *	CCD_DSP_DEINTERLACE_SPLIT_PARALLEL,
 * 	CCD_DSP_DEINTERLACE_SPLIT_SERIAL or
 * 	CCD_DSP_DEINTERLACE_SPLIT_QUAD.
 * @param filename The filename to save the exposure into.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Rdi
 * @see #DSP_Image_Transfer
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_RDC(int ncols,int nrows,int numbytes,
	enum CCD_DSP_DEINTERLACE_TYPE deinterlace_type,char *filename)
{
	DSP_Error_Number = 0;
	/* number of columns must be a positive number */
	if(ncols <= 0)
	{
		DSP_Error_Number = 10;
		sprintf(DSP_Error_String,"CCD_DSP_Command_RDC:Illegal ncols '%d'.",ncols);
		return FALSE;
	}
	/* number of rows must be a positive number */
	if(nrows <= 0)
	{
		DSP_Error_Number = 11;
		sprintf(DSP_Error_String,"CCD_DSP_Command_RDC:Illegal nrows '%d'.",nrows);
		return FALSE;
	}
	if(numbytes <= 0)
	{
		DSP_Error_Number = 12;
		sprintf(DSP_Error_String,"CCD_DSP_Command_RDC:Illegal numbytes '%d'.",numbytes);
		return FALSE;
	}
	if(!CCD_DSP_IS_DEINTERLACE_TYPE(deinterlace_type))
	{
		DSP_Error_Number = 13;
		sprintf(DSP_Error_String,"CCD_DSP_Command_RDC:Illegal deinterlace type '%d'.",deinterlace_type);
		return FALSE;
	}
	if(!DSP_Send_Rdi())
		return FALSE;
	/* DSP_Image_Transfer saves the exposed image into a filename */
	if(!DSP_Image_Transfer(ncols,nrows,numbytes,deinterlace_type,filename))
		return FALSE;
	/* get reply - DON should be returned */
	return(DSP_Get_Reply(CCD_DSP_DON));
}

/**
 * This routine executes the Set Bias Voltage (SBV) command on a 
 * SDSU Controller board. This
 * sets the voltage of the video processor DC bias and clock driver DACs from information in DSP memory.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Sbv
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_SBV(void)
{
	DSP_Error_Number = 0;
	if(!DSP_Send_Sbv())
		return FALSE;
	/* get reply - DON should be returned */
	return(DSP_Get_Reply(CCD_DSP_DON));
}

/**
 * SGN command routine. Timing board command that means Set GaiN. 
 * This sets the gains of all the video processors.
 * The integrator speed is also set using this command to slow or fast.
 * @param gain The gain to set the video processors to. One of:
 * 	CCD_DSP_GAIN_ONE(one),CCD_DSP_GAIN_TWO(two),CCD_DSP_GAIN_FOUR(4.75) and
 * 	CCD_DSP_GAIN_NINE(9.5).
 * @param speed The integrator speed to set the video processors to. Either 0 or 1.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Sgn
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_SGN(enum CCD_DSP_GAIN gain,int speed)
{
	DSP_Error_Number = 0;
	if(!CCD_DSP_IS_GAIN(gain))
	{
		DSP_Error_Number = 14;
		sprintf(DSP_Error_String,"CCD_DSP_Command_SGN:Illegal gain '%d'.",gain);
		return FALSE;
	}
	/* speed setting is either 0 or 1 for integrator speed
	** therefore test whether data is a boolean (0 or 1) */
	if(!CCD_GLOBAL_IS_BOOLEAN(speed))
	{
		DSP_Error_Number = 15;
		sprintf(DSP_Error_String,"CCD_DSP_Command_SGN:Illegal speed '%d'.",speed);
		return FALSE;
	}
	if(!DSP_Send_Sgn(gain,speed))
		return FALSE;
	/* get reply - DON should be returned */
	return(DSP_Get_Reply(CCD_DSP_DON));
}

/**
 * This routine executes the SToP (STP) command on the timing board. This
 * stops the clocks clocking the readout sequence.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #CCD_DSP_Command_IDL
 * @see #DSP_Send_Stp
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_STP(void)
{
	DSP_Error_Number = 0;
	if(!DSP_Send_Stp())
		return FALSE;
	/* get reply - DON should be returned */
	return(DSP_Get_Reply(CCD_DSP_DON));
}

/**
 * This routine sets the number of Columns the SDSU Controller will read out. This is sent to the PCI
 * interface which stores it in the timing table.
 * @param ncols The number of columns to read out, after binning has been taken into account.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Set_NCols
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_Set_NCols(int ncols)
{
	DSP_Error_Number = 0;
	if(!DSP_Send_Set_NCols(ncols))
		return FALSE;
	/* get reply - DON should be returned */
	return(DSP_Get_Reply(CCD_DSP_DON));
}

/**
 * This routine sets the number of Rows the SDSU Controller will read out. This is sent to the PCI
 * interface which stores it in the timing table.
 * @param nrows The number of rows to read out, after binning has been taken into account.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Set_NRows
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_Set_NRows(int nrows)
{
	DSP_Error_Number = 0;
	if(!DSP_Send_Set_NRows(nrows))
		return FALSE;
	/* get reply - DON should be returned */
	return(DSP_Get_Reply(CCD_DSP_DON));
}

/* utility board commands */
/**
 * This routine executes the Abort EXposure (AEX) command on a 
 * SDSU utility board. If an
 * exposure is currently underway this is stopped by closing the shutter and putting the CCD in idle mode.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Aex
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_AEX(void)
{
	DSP_Error_Number = 0;
	if(!DSP_Send_Aex())
		return FALSE;
	/* get reply - DON should be returned */
	return(DSP_Get_Reply(CCD_DSP_DON)); 
}

/**
 * This routine executes the Close SHutter (CSH) command on the SDSU utility board.
 * This closes the shutter.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Csh
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_CSH(void)
{
	DSP_Error_Number = 0;
	if(!DSP_Send_Csh())
		return FALSE;
	/* get reply - DON should be returned */
	return(DSP_Get_Reply(CCD_DSP_DON));
}

/**
 * This routine executes the Open SHutter (OSH) command on the SDSU utility board.
 * This opens the shutter.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Osh
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_OSH(void)
{
	DSP_Error_Number = 0;
	if(!DSP_Send_Osh())
		return FALSE;
	/* get reply - DON should be returned */
	return(DSP_Get_Reply(CCD_DSP_DON));
}

/**
 * This routine executes the Pause EXposure (PEX) command on the
 * SDSU utility board. This closes the shutter and stops the timer.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #CCD_DSP_Command_REX
 * @see #DSP_Send_Pex
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_PEX(void)
{
	DSP_Error_Number = 0;
	if(!DSP_Send_Pex())
		return FALSE;
	/* get reply - DON should be returned */
	return(DSP_Get_Reply(CCD_DSP_DON));
}

/**
 * This routine executes the Power ON (PON) command on a SDSU Controller board.
 * This turns the analog power on safely, using the power control board.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Pon
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_PON(void)
{
	DSP_Error_Number = 0;
	if(!DSP_Send_Pon())
		return FALSE;
	/* get reply - DON should be returned */
	return(DSP_Get_Reply(CCD_DSP_DON));
}

/**
 * This routine executes the Power OFF (POF) command on a SDSU Controller board.
 * This turns the analog power off safely, using the power control board.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Pof
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_POF(void)
{
	DSP_Error_Number = 0;
	if(!DSP_Send_Pof())
		return FALSE;
	/* get reply - DON should be returned */
	return(DSP_Get_Reply(CCD_DSP_DON));
}

/**
 * Routine to read the array temperature from the utility board.
 * This calls the PCI interfaces read temperature command. This in turn calls the utility board with
 * read memory command for address 0x0c in Y memory space. The reply memory is then read.
 * @return The adu counts held on the utility board. If an error occurs, zero is returned. Use 
 * 	CCD_DSP_Get_Error_Number to determine whether zero is returned from the board or is an error.
 * @see #DSP_Send_Read_Temperature
 * @see #DSP_Get_Reply
 * @see #DSP_ACTUAL_VALUE
 */
int CCD_DSP_Command_Read_Temperature(void)
{
	DSP_Error_Number = 0;
	if(!DSP_Send_Read_Temperature())
		return FALSE;
	/* get reply - the array temperature adu should be returned */
	return(DSP_Get_Reply(DSP_ACTUAL_VALUE));
}

/**
 * Routine to set the target array temperature on the utility board.
 * This calls the PCI interfaces set temperature command. This in turn calls the utility board with
 * write memory command for address 0x1c in Y memory space. The reply memory is then read to ensure DON is returned.
 * @param The target adu counts for the desired temperature.
 * @return The routine returnd DON if the command suceeded, FALSE if it failed.
 * @see #DSP_Send_Set_Temperature
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_Set_Temperature(int adu)
{
	DSP_Error_Number = 0;
	if(!DSP_Send_Set_Temperature(adu))
		return FALSE;
	/* get reply - DON returned */
	return(DSP_Get_Reply(CCD_DSP_DON));
}

/**
 * This routine executes the Resume EXposure (REX) command on a 
 * SDSU Controller board. This opens the shutter and restarts the timer.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #CCD_DSP_Command_PEX
 * @see #DSP_Send_Rex
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_REX(void)
{
	DSP_Error_Number = 0;
	if(!DSP_Send_Rex())
		return FALSE;
	/* get reply - DON should be returned */
	return(DSP_Get_Reply(CCD_DSP_DON));
}

/**
 * This routine executes the Start EXposure (SEX) command on a 
 * SDSU Controller board. This starts an exposure.
 * @param ncols The number of columns in the image. This must be a positive non-zero integer.
 * @param nrows The number of rows in the image to be readout from the CCD. This must be a positive non-zero integer.
 * @param numbytes The number of bytes to read out from the CCD. This should be (nrows * ncols * 
 * 	<a href="ccd_global.html#CCD_GLOBAL_BYTES_PER_PIXEL">CCD_GLOBAL_BYTES_PER_PIXEL</a>).
 * @param msecs The amount of time in milliseconds to open the shutter for. This must be greater than zero.
 * @param deinterlace_type The algorithm to use for deinterlacing the resulting data. The data needs to be
 * 	deinterlaced if the CCD is read out from multiple readouts. One of 
 * 	<a href="ccd_setup.html#CCD_DSP_DEINTERLACE_TYPE">CCD_DSP_DEINTERLACE_TYPE</a>:
 * 	CCD_DSP_DEINTERLACE_SINGLE,
 * 	CCD_DSP_DEINTERLACE_SPLIT_PARALLEL,
 * 	CCD_DSP_DEINTERLACE_SPLIT_SERIAL or
 * 	CCD_DSP_DEINTERLACE_SPLIT_QUAD.
 * @param filename The filename to save the exposure into.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Sex
 * @see ccd_pci.html#CCD_PCI_HCVR_READ_EXPOSURE_TIME
 * @see #DSP_Send_Rdi
 * @see #DSP_Image_Transfer
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_SEX(int ncols,int nrows,int numbytes,int msecs,
	enum CCD_DSP_DEINTERLACE_TYPE deinterlace_type,char *filename)
{
	int dsp_msecs,rem_msecs;

	DSP_Error_Number = 0;
/* number of columns must be a positive number */
	if(ncols <= 0)
	{
		DSP_Error_Number = 16;
		sprintf(DSP_Error_String,"CCD_DSP_Command_SEX:Illegal ncols '%d'.",ncols);
		return FALSE;
	}
/* number of rows must be a positive number */
	if(nrows <= 0)
	{
		DSP_Error_Number = 17;
		sprintf(DSP_Error_String,"CCD_DSP_Command_SEX:Illegal nrows '%d'.",nrows);
		return FALSE;
	}
	if(numbytes <= 0)
	{
		DSP_Error_Number = 18;
		sprintf(DSP_Error_String,"CCD_DSP_Command_SEX:Illegal numbytes '%d'.",numbytes);
		return FALSE;
	}
/* exposure time must be greater than zero */
	if(msecs <= 0)
	{
		DSP_Error_Number = 19;
		sprintf(DSP_Error_String,"CCD_DSP_Command_SEX:Illegal msecs '%d'.",msecs);
		return FALSE;
	}
	if(!CCD_DSP_IS_DEINTERLACE_TYPE(deinterlace_type))
	{
		DSP_Error_Number = 20;
		sprintf(DSP_Error_String,"CCD_DSP_Command_SEX:Illegal deinterlace type '%d'.",deinterlace_type);
		return FALSE;
	}
/* initialise variables */
	rem_msecs = msecs;
	DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_EXPOSE;
	if(!DSP_Send_Sex())
	{
		DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_NONE;
		return FALSE;
	}
/* ensure SEX command sent correctly */
	if(!DSP_Get_Reply(CCD_DSP_DON))
	{
		DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_NONE;
		return FALSE;
	}
/* wait until we are about ready to read out */
	while((rem_msecs > READOUT_REMAINING_TIME)&&(DSP_Data.Abort == FALSE))
	{
		sleep(1);
	/* get elapsed time from utility board */
		if(!DSP_Send_Command(CCD_PCI_HCVR_READ_EXPOSURE_TIME,NULL,0))
		{
			CCD_DSP_Warning();
			dsp_msecs += 1000;
			rem_msecs = msecs-dsp_msecs;
		}
		else
		{
			dsp_msecs = DSP_Get_Reply(DSP_ACTUAL_VALUE);
			rem_msecs = msecs-dsp_msecs;
		}
	}/* end while exposing */
	if(DSP_Data.Abort)
	{
		DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_NONE;
		DSP_Error_Number = 21;
		sprintf(DSP_Error_String,"CCD_DSP_Command_SEX:Aborted.");
		return FALSE;
	}
/* send command to read out */
	if(!DSP_Send_Rdi())
	{
		DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_NONE;
		return FALSE;
	}
/* DSP_Image_Transfer saves the exposed image into a filename */
	if(!DSP_Image_Transfer(ncols,nrows,numbytes,deinterlace_type,filename))
		return FALSE;
/* get reply - DON should be returned */
	return(DSP_Get_Reply(CCD_DSP_DON));
}

/**
 * This routine resets the SDSU Controller boards. It sends the PCI reset controller command.
 * It then gets the reply from the interface, which should be SYR.
 * @return The routine returns SYR if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Reset
 * @see #DSP_Get_Reply
 * @see #CCD_DSP_SYR
 */
int CCD_DSP_Command_Reset(void)
{
	DSP_Error_Number = 0;
	if(!DSP_Send_Reset())
		return FALSE;
	/* get reply - DON should be returned */
	return(DSP_Get_Reply(CCD_DSP_SYR));
}

/**
 * Routine to set the utility board options. This is written to the PCI interface 
 * utility options register using a special PCI command. The PCI interface itself writes this to the
 * utility board, memory space X, location 1.
 * Currently only the first bit is used. If it is set the utility board will open the shutter during a 
 * SEX command, otherwise the shutter will remain closed.
 * @param bit_value The bit value of the utility options. 
 * @return The routine returns DON if the operation succeeds, FALSE otherwise.
 * @see #DSP_Send_Set_Util_Options
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_Set_Util_Options(int bit_value)
{
	DSP_Error_Number = 0;
	if(!DSP_Send_Set_Util_Options(bit_value))
		return FALSE;
/* get reply - DON should be returned */
	return(DSP_Get_Reply(CCD_DSP_DON));
}

/**
 * Routine to set the utility board exposure time. This is written to the PCI interface 
 * exposure time register using a special PCI command. The PCI interface itself writes this to the
 * utility board, memory space Y, location 24.
 * @param msecs The exposure time in milliseconds. 
 * @return The routine returns DON if the operation succeeds, FALSE otherwise.
 * @see #DSP_Send_Set_Exposure_Time
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_Set_Exposure_Time(int msecs)
{
	DSP_Error_Number = 0;
	if(!DSP_Send_Set_Exposure_Time(msecs))
		return FALSE;
/* get reply - DON should be returned */
	return(DSP_Get_Reply(CCD_DSP_DON));
}

/**
 * Downloads some DSP code to one of the boards from filename.
 * @param board The board to send the command to.
 * @param filename The filename of compiled DSP commends to send to the board.
 * 	This is usually a .lod file.
 * @return Returns TRUE if the operation succeeds, FALSE if it fails.
 * @see #DSP_Get_Download_Type
 * @see #DSP_Read_Line
 * @see #DSP_Address_Char_To_Mem_Space
 * @see #DSP_Process_Data
 */
int CCD_DSP_Download(enum CCD_DSP_BOARD_ID board_id,char *filename)
{
	FILE *download_fp;
	enum CCD_DSP_MEM_SPACE mem_space;
	int finished,download_board_id,addr,ret_value;
	char buff[255],addr_type;

	DSP_Error_Number = 0;
	if(!CCD_DSP_IS_BOARD_ID(board_id))
	{
		DSP_Error_Number = 22;
		sprintf(DSP_Error_String,"CCD_DSP_Download:Illegal board ID '%d'.",board_id);
		return FALSE;
	}
	/* try to open the file */
	if((download_fp = fopen(filename,"r")) == NULL)
	{
		DSP_Error_Number = 23;
		sprintf(DSP_Error_String,"CCD_DSP_Download:Could not open filename(%s).",
			filename);
		return FALSE;
	}
	/* ensure the file is for the same board as the one we are trying to send a program to */
	if((download_board_id = DSP_Get_Download_Type(download_fp)) == FALSE)
	{
		fclose(download_fp);
		DSP_Error_Number = 24;
		sprintf(DSP_Error_String,"CCD_DSP_Download:Could not get filename type(%s).",
			filename);
		return FALSE;
	}

	if (download_board_id != board_id)
	{
		fclose(download_fp);
		DSP_Error_Number = 25;
		sprintf(DSP_Error_String,"CCD_DSP_Download:Boards do not match(%s,%d,%d).",
			filename,download_board_id,board_id);
		return FALSE;
	}
	finished = FALSE;
	/* send the data to the board until the end of the file is reached 
	** or the operation is aborted */
	while((!finished)&&(!DSP_Data.Abort))
	{
		DSP_Read_Line(download_fp,buff);
		if(strncmp(buff,"_END",4) == 0)
			finished = TRUE;
		else if(sscanf(buff,"_DATA %c %x",&addr_type,&addr) == 2)
		{
			if(!DSP_Address_Char_To_Mem_Space(addr_type,&mem_space))
			{
				fclose(download_fp);
				return FALSE;
			}
			if (addr < DSP_DOWNLOAD_ADDR_MAX)
			{
				if(!DSP_Process_Data(download_fp,board_id,mem_space,addr))
				{
					fclose(download_fp);
					return FALSE;
				}
			}
		}
	}
	fclose(download_fp);
	return(TRUE);
}

/**
 * This routine returns the current stste of the Abort flag.
 * The Abort flag is defined in DSP_Data and is set to true when
 * the user wants to stop execution mid-commend.
 * @return The current Abort status.
 * @see #CCD_DSP_Set_Abort
 */
int CCD_DSP_Get_Abort(void)
{
	return DSP_Data.Abort;
}

/**
 * This routine allows the setting and reseting of the Abort flag.
 * The Abort flag is defined in DSP_Data and is set to true when
 * the user wants to stop execution mid-commend.
 * @return Returns TRUE or FALSE to indicate success/failure.
 * @param value What to set the Abort flag to: either TRUE or FALSE.
 * @see #CCD_DSP_Get_Abort
 * @see #DSP_Data
 */
int CCD_DSP_Set_Abort(int value)
{
	if(!CCD_GLOBAL_IS_BOOLEAN(value))
	{
		DSP_Error_Number = 26;
		sprintf(DSP_Error_String,"CCD_DSP_Set_Abort:Illegal value '%d'.",value);
		return FALSE;
	}
	DSP_Data.Abort = value;
	return TRUE;
}

/**
 * This routine gets the current value of Exposure Status.
 * Exposure_Status is defined in DSP_Data and is one of:
 * <ul>
 * <li>CCD_DSP_EXPOSURE_STATUS_NONE - no exposure in progress
 * <li>CCD_DSP_EXPOSURE_STATUS_CLEAR - the camera is clearing
 * <li>CCD_DSP_EXPOSURE_STATUS_EXPOSE - the camera is exposing
 * <li>CCD_DSP_EXPOSURE_STATUS_READOUT - the ccd is reading out
 * </ul>
 * @return The current status of exposure.
 * @see #CCD_DSP_EXPOSURE_STATUS
 * @see #DSP_Data
 */
enum CCD_DSP_EXPOSURE_STATUS CCD_DSP_Get_Exposure_Status(void)
{
	return DSP_Data.Exposure_Status;
}

/**
 * Get the current value of ccd_dsp's error number.
 * @return The current value of ccd_dsp's error number.
 */
int CCD_DSP_Get_Error_Number(void)
{
	return DSP_Error_Number;
}

/**
 * The error routine that reports any errors occuring in ccd_dsp in a standard way.
 */
void CCD_DSP_Error(void)
{
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(DSP_Error_Number == 0)
		sprintf(DSP_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"CCD_DSP:Error(%d) : %s\n",DSP_Error_Number,DSP_Error_String);
	DSP_Error_Number = 0;
}

/**
 * The error routine that reports any errors occuring in ccd_dsp in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 */
void CCD_DSP_Error_String(char *error_string)
{
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(DSP_Error_Number == 0)
		sprintf(DSP_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"CCD_DSP:Error(%d) : %s\n",DSP_Error_Number,DSP_Error_String);
	DSP_Error_Number = 0;
}

/**
 * The warning routine that reports any warnings occuring in ccd_dsp in a standard way.
 */
void CCD_DSP_Warning(void)
{
	/* if the error number is zero an warning message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an warning to display */
	if(DSP_Error_Number == 0)
		sprintf(DSP_Error_String,"Logic Error:No Warning defined");
	fprintf(stderr,"CCD_DSP:Warning(%d) : %s\n",DSP_Error_Number,DSP_Error_String);
	DSP_Error_Number = 0;
}

/* ----------------------------------------------------------------
**	Internal routines
** ---------------------------------------------------------------- */
/**
 * Internal DSP command to load a DSP application program from EEPROM to DSP memory. Uses DSP_Set_Destination
 * to set the correct board.
 * @param board_id The SDSU CCD Controller board,
 * 	CCD_DSP_TIM_BOARD_ID(timing board) or CCD_DSP_UTIL_BOARD_ID(utility board).
 * @param data The application number to load.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #CCD_DSP_BOARD_ID
 * @see #DSP_Set_Destination
 * @see #DSP_Send_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_LOAD_APPLICATION
 */
static int DSP_Send_Lda(enum CCD_DSP_BOARD_ID board_id,int data)
{
	int argument_list[1];
	int argument_count = 0;

	argument_list[argument_count++] = data;
	if(!DSP_Set_Destination(board_id,argument_count))
		return FALSE;
	return DSP_Send_Command(CCD_PCI_HCVR_LOAD_APPLICATION,argument_list,argument_count);
}

/**
 * Internal DSP command to read data from address address in memory space mem_space on board board_id. 
 * Uses DSP_Set_Destination to set the correct board.
 * @param board_id The SDSU CCD Controller board, one of CCD_DSP_INTERFACE_BOARD_ID(interface),
 * 	CCD_DSP_TIM_BOARD_ID(timing board) or CCD_DSP_UTIL_BOARD_ID(utility board).
 * @param mem_space The memory space on board board_id to read from, of type 
 * <a href="#CCD_DSP_MEM_SPACE">CCD_DSP_MEM_SPACE</a>. One of:
 * 	CCD_DSP_MEM_SPACE_D(DRAM),
 * 	CCD_DSP_MEM_SPACE_P(program),
 * 	CCD_DSP_MEM_SPACE_X(X data),
 * 	CCD_DSP_MEM_SPACE_Y(Y data)
 * 	or CCD_DSP_MEM_SPACE_R(ROM).
 * @param address The memory location to get the data.
 * @return The data held at the specified address, or false if a failure occurs.
 * @see #CCD_DSP_RDM
 * @see #CCD_DSP_BOARD_ID
 * @see #DSP_Set_Destination
 * @see #DSP_Send_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_READ_MEMORY
 */
static int DSP_Send_Rdm(enum CCD_DSP_BOARD_ID board_id,enum CCD_DSP_MEM_SPACE mem_space,int address)
{
	int argument_list[2];
	int argument_count = 0;

	argument_list[argument_count++] = mem_space;
	argument_list[argument_count++] = address;
	if(!DSP_Set_Destination(board_id,argument_count))
		return FALSE;
	return DSP_Send_Command(CCD_PCI_HCVR_READ_MEMORY,argument_list,argument_count);
}

/**
 * Internal DSP command to write data to address address in memory space mem_space to board board_id.
 * Uses DSP_Set_Destination to set the correct board.
 * @param board_id The SDSU CCD Controller board, one of CCD_DSP_INTERFACE_BOARD_ID(interface),
 * 	CCD_DSP_TIM_BOARD_ID(timing board) or CCD_DSP_UTIL_BOARD_ID(utility board).
 * @param mem_space The memory space on board board_id to read from, of type 
 * <a href="#CCD_DSP_MEM_SPACE">CCD_DSP_MEM_SPACE</a>. One of:
 * 	CCD_DSP_MEM_SPACE_D(DRAM),
 * 	CCD_DSP_MEM_SPACE_P(program),
 * 	CCD_DSP_MEM_SPACE_X(X data),
 * 	CCD_DSP_MEM_SPACE_Y(Y data)
 * 	or CCD_DSP_MEM_SPACE_R(ROM).
 * @param address The memory location to put the data.
 * @param data The data to put into the memory location.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #CCD_DSP_WRM
 * @see #CCD_DSP_BOARD_ID
 * @see #DSP_Set_Destination
 * @see #DSP_Send_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_WRITE_MEMORY
 */
static int DSP_Send_Wrm(enum CCD_DSP_BOARD_ID board_id,enum CCD_DSP_MEM_SPACE mem_space,int address,int data)
{
	int argument_list[3];
	int argument_count = 0;

	argument_list[argument_count++] = mem_space;
	argument_list[argument_count++] = address;
	argument_list[argument_count++] = data;
	if(!DSP_Set_Destination(board_id,argument_count))
		return FALSE;
	return DSP_Send_Command(CCD_PCI_HCVR_WRITE_MEMORY,argument_list,argument_count);
}

/**
 * Internal DSP command to test the data link to the SDSU CCD Controller is working correctly.
 * Uses DSP_Set_Destination to set the correct board.
 * @param board_id The board to send the command to. One of
 * 	CCD_DSP_TIM_BOARD_ID(timing board) or CCD_DSP_UTIL_BOARD_ID(utility board).
 * @param data The data to test the link with. This can any 24 bit number.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Set_Destination
 * @see #DSP_Send_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_TEST_DATA_LINK
 */
static int DSP_Send_Tdl(enum CCD_DSP_BOARD_ID board_id,int data)
{
	int argument_list[1];
	int argument_count = 0;

	argument_list[argument_count++] = data;
	if(!DSP_Set_Destination(board_id,argument_count))
		return FALSE;
	return DSP_Send_Command(CCD_PCI_HCVR_TEST_DATA_LINK,argument_list,argument_count);
}

/**
 * Internal DSP command to abort readout of the ccd.
 * This routine calls the interface routine directly, rather than by DSP_Send_Command.
 * This means ancillary calls to the interface such as clearing the reply memory are not done
 * for aborting readout, as they are not necessary.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see ccd_interface.html#CCD_Interface_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_ABORT_READOUT
 */
static int DSP_Send_Abr(void)
{
	int hcvr_command;

	hcvr_command = CCD_PCI_HCVR_ABORT_READOUT;
	if(!CCD_Interface_Command(CCD_PCI_IOCTL_SET_HCVR,&hcvr_command))
	{
		DSP_Error_Number = 27;
		sprintf(DSP_Error_String,"DSP_Send_Abr:Sending command %#x failed.",hcvr_command);
		return FALSE;
	}
	return TRUE;
}

/**
 * Internal DSP command to clear the CCD of any stored charge on it, ready to begin a new exposure.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_CLEAR_ARRAY
 */
static int DSP_Send_Clr(void)
{
	return DSP_Send_Command(CCD_PCI_HCVR_CLEAR_ARRAY,NULL,0);
}

/**
 * Internal DSP command to put the CCD clocks in the readout sequence but not transfering any data. This stops the
 * CCD building up any charge.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Stp
 * @see #DSP_Send_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_RESUME_IDLE_MODE
 */
static int DSP_Send_Idl(void)
{
	return DSP_Send_Command(CCD_PCI_HCVR_RESUME_IDLE_MODE,NULL,0);
}

/**
 * Internal DSP command to immediately readout the ccd. Calls DSP_Set_Destination before sending
 * the read image command, this needs reviewing.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_READ_IMAGE
 */
static int DSP_Send_Rdi(void)
{
/* We need to review whether this set destination call is needed.
** Acording to the Voodoo code it is, according to the DSP code it is not.
*/
	if(!DSP_Set_Destination(CCD_DSP_INTERFACE_BOARD_ID,0))
		return FALSE;
	return DSP_Send_Command(CCD_PCI_HCVR_READ_IMAGE,NULL,0);
}

/**
 * Internal DSP command to set bias voltages.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_SET_BIAS_VOLTAGES
 */
static int DSP_Send_Sbv(void)
{
	return DSP_Send_Command(CCD_PCI_HCVR_SET_BIAS_VOLTAGES,NULL,0);
}

/**
 * Internal DSP command to set the gain values of the video processors. Uses DSP_Set_Destination to set
 * manual command destination to the timing board.
 * @param gain What value to set the gain to. One of :
 *	<dl>
 * 	<dt>CCD_DSP_GAIN_ONE</dt> <dd>Set gain = 1</dd>
 * 	<dt>CCD_DSP_GAIN_TWO</dt> <dd>Set gain = 2</dd>
 * 	<dt>CCD_DSP_GAIN_FOUR</dt> <dd>Set gain = 4.75</dd>
 * 	<dt>CCD_DSP_GAIN_NINE</dt> <dd>Set gain = 9.5</dd>
 * 	</dl>
 * @param speed Sets the speed of the integrators. TRUE is fast integrator speed, FALSE is slow integrator speed.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #CCD_DSP_BOARD_ID
 * @see #CCD_DSP_SGN
 * @see #DSP_Set_Destination
 * @see #DSP_Send_Manual_Command
 */
static int DSP_Send_Sgn(enum CCD_DSP_GAIN gain,int speed)
{
	int argument_list[2];
	int argument_count = 0;

	argument_list[argument_count++] = gain;
	argument_list[argument_count++] = speed;
	if(!DSP_Set_Destination(CCD_DSP_TIM_BOARD_ID,argument_count))
		return FALSE;
	return DSP_Send_Manual_Command(CCD_DSP_SGN,argument_list,argument_count);
}

/**
 * Internal DSP command to come out of idle mode.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Idl
 * @see #DSP_Send_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_STOP_IDLE_MODE
 */
static int DSP_Send_Stp(void)
{
	return DSP_Send_Command(CCD_PCI_HCVR_STOP_IDLE_MODE,NULL,0);
}

/**
 * Routine to set the number of columns to read out. Note this routine does not work via the 
 * Host Command Vector Register like most routines, but directly calls the interface with a special ioctl
 * command.
 * @param ncols Number of columns to read out, after binning has been taken into account.
 * @see #DSP_Clear_Reply_Memory
 * @see ccd_interface.html#CCD_Interface_Command
 * @see ccd_pci.html#CCD_PCI_IOCTL_SET_NCOLS
 */
static int DSP_Send_Set_NCols(int ncols)
{
	if(DSP_Data.Abort)
	{
		DSP_Error_Number = 60;
		sprintf(DSP_Error_String,"DSP_Send_Set_NCols:Aborted.");		
		return FALSE;
	}
/* clear the reply memory */
	if(!DSP_Clear_Reply_Memory())
		return FALSE;
/* send command to interface */
	if(!CCD_Interface_Command(CCD_PCI_IOCTL_SET_NCOLS,&ncols))
	{
		DSP_Error_Number = 61;
		sprintf(DSP_Error_String,"DSP_Send_Set_NCols:Sending command NCols %d failed.",ncols);
		return FALSE;
	}
	return TRUE;
}

/**
 * Routine to set the number of rows to read out. Note this routine does not work via the 
 * Host Command Vector Register like most routines, but directly calls the interface with a special ioctl
 * command.
 * @param nrows Number of rows to read out, after binning has been taken into account.
 * @see #DSP_Clear_Reply_Memory
 * @see ccd_interface.html#CCD_Interface_Command
 * @see ccd_pci.html#CCD_PCI_IOCTL_SET_NROWS
 */
static int DSP_Send_Set_NRows(int nrows)
{
	if(DSP_Data.Abort)
	{
		DSP_Error_Number = 62;
		sprintf(DSP_Error_String,"DSP_Send_Set_NRows:Aborted.");
		return FALSE;
	}
/* clear the reply memory */
	if(!DSP_Clear_Reply_Memory())
		return FALSE;
/* send command to interface */
	if(!CCD_Interface_Command(CCD_PCI_IOCTL_SET_NROWS,&nrows))
	{
		DSP_Error_Number = 63;
		sprintf(DSP_Error_String,"DSP_Send_Set_NRows:Sending command NRows %d failed.",nrows);
		return FALSE;
	}
	return TRUE;
}

/**
 * Internal DSP command to abort the exposure that is currently underway.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_ABORT_EXPOSURE
 */
static int DSP_Send_Aex(void)
{
	return DSP_Send_Command(CCD_PCI_HCVR_ABORT_EXPOSURE,NULL,0);
}

/**
 * Internal DSP command to close the shutter.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Osh
 * @see #DSP_Send_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_CLOSE_SHUTTER
 */
static int DSP_Send_Csh(void)
{
	return DSP_Send_Command(CCD_PCI_HCVR_CLOSE_SHUTTER,NULL,0);
}

/**
 * Internal DSP command to open the shutter.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Csh
 * @see #DSP_Send_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_OPEN_SHUTTER
 */
static int DSP_Send_Osh(void)
{
	return DSP_Send_Command(CCD_PCI_HCVR_OPEN_SHUTTER,NULL,0);
}

/**
 * Internal DSP command to read the temperature from the SDSU utility board.
 * @return Returns true if the command succeeded, false if it failed.
 * @see #DSP_Send_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_READ_ARRAY_TEMPERATURE
 */
static int DSP_Send_Read_Temperature(void)
{
	return DSP_Send_Command(CCD_PCI_HCVR_READ_ARRAY_TEMPERATURE,NULL,0);
}

/**
 * Internal DSP command to set the target temperature on the SDSU utility board.
 * @return Returns true if the command succeeded, false if it failed.
 * @see #DSP_Send_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_SET_ARRAY_TEMPERATURE
 */
static int DSP_Send_Set_Temperature(int adu)
{
	int argument_list[1];
	int argument_count = 0;

	argument_list[argument_count++] = adu;
	return DSP_Send_Command(CCD_PCI_HCVR_SET_ARRAY_TEMPERATURE,argument_list,argument_count);
}

/**
 * Internal DSP command to pause the exposure that is currently underway.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Rex
 * @see #DSP_Send_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_PAUSE_EXPOSURE
 */
static int DSP_Send_Pex(void)
{
	return DSP_Send_Command(CCD_PCI_HCVR_PAUSE_EXPOSURE,NULL,0);
}

/**
 * Internal DSP command to turn the analog power supplies on using the power control board.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_POWER_ON
 */
static int DSP_Send_Pon(void)
{
	return DSP_Send_Command(CCD_PCI_HCVR_POWER_ON,NULL,0);
}

/**
 * Internal DSP command to turn the analog power supplies off using the power control board.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_POWER_OFF
 */
static int DSP_Send_Pof(void)
{
	return DSP_Send_Command(CCD_PCI_HCVR_POWER_OFF,NULL,0);
}

/**
 * Internal DSP command to resume the exposure that is currently underway.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Pex
 * @see #DSP_Send_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_RESUME_EXPOSURE
 */
static int DSP_Send_Rex(void)
{
	return DSP_Send_Command(CCD_PCI_HCVR_RESUME_EXPOSURE,NULL,0);
}

/**
 * Internal DSP command to make the SDSU CCD Controller start an exposure.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_START_EXPOSURE
 */
static int DSP_Send_Sex(void)
{
/* We need to review whether this set destination call is needed.
** Acording to the Voodoo code it is, according to the DSP code it is not.
*/
	if(!DSP_Set_Destination(CCD_DSP_INTERFACE_BOARD_ID,0))
		return FALSE;
	return DSP_Send_Command(CCD_PCI_HCVR_START_EXPOSURE,NULL,0);
}

/**
 * Internal DSP command to reset all the SDSU boards.
 * @return Returns true if sending the command succeeded, false if it failed. It does not return
 * 	SYR, read the reply value to get this value.
 * @see #DSP_Send_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_RESET_CONTROLLER
 */
static int DSP_Send_Reset(void)
{
	return DSP_Send_Command(CCD_PCI_HCVR_RESET_CONTROLLER,NULL,0);
}

/**
 * Internal DSP command to set utility options. This does not set the PCI HCVR (Host Command Vector Register)
 * like most commands, but uses it's own PCI ioctl request number CCD_PCI_IOCTL_SET_UTIL_OPTIONS.
 * @return Returns TRUE if the command was sent without error, FALSE otherwise.
 * @see #DSP_Clear_Reply_Memory
 * @see ccd_pci.html#CCD_PCI_IOCTL_SET_UTIL_OPTIONS
 * @see ccd_interface.html#CCD_Interface_Command
 */
static int DSP_Send_Set_Util_Options(int bit_value)
{
	if(DSP_Data.Abort)
	{
		DSP_Error_Number = 64;
		sprintf(DSP_Error_String,"DSP_Send_Set_Util_Options:Aborted.");
		return FALSE;
	}
/* clear the reply memory */
	if(!DSP_Clear_Reply_Memory())
		return FALSE;
/* send command to interface */
	if(!CCD_Interface_Command(CCD_PCI_IOCTL_SET_UTIL_OPTIONS,&bit_value))
	{
		DSP_Error_Number = 65;
		sprintf(DSP_Error_String,"DSP_Send_Set_Util_Options:Sending utility options %d failed.",bit_value);
		return FALSE;
	}
	return TRUE;
}

/**
 * Internal DSP command to set exposure time. This does not set the PCI HCVR (Host Command Vector Register)
 * like most commands, but uses it's own PCI ioctl request number CCD_PCI_IOCTL_SET_EXPTIME.
 * @return Returns TRUE if the command was sent without error, FALSE otherwise.
 * @see ccd_pci.html#CCD_PCI_IOCTL_SET_EXPTIME
 * @see ccd_interface.html#CCD_Interface_Command
 */
static int DSP_Send_Set_Exposure_Time(int msecs)
{
	if(DSP_Data.Abort)
	{
		DSP_Error_Number = 66;
		sprintf(DSP_Error_String,"DSP_Send_Set_Exposure_Time:Aborted.");
		return FALSE;
	}
/* clear the reply memory */
	if(!DSP_Clear_Reply_Memory())
		return FALSE;
/* send command to interface */
	if(!CCD_Interface_Command(CCD_PCI_IOCTL_SET_EXPTIME,&msecs))
	{
		DSP_Error_Number = 67;
		sprintf(DSP_Error_String,"DSP_Send_Set_Exposure_Time:Sending exposure time %d failed.",msecs);
		return FALSE;
	}
	return TRUE;
}

/**
 * Internal command to clear the reply memory. This sets all the reply memory to -1.
 * @return Returns true if clearing the reply memory succeeded, false if it failed.
 * @see ccd_interface.html#CCD_Interface_Command
 * @see ccd_pci.html#CCD_PCI_IOCTL_CLEAR_REPLY
 */
static int DSP_Clear_Reply_Memory(void)
{
	int retval,value;

	if(CCD_Interface_Command(CCD_PCI_IOCTL_CLEAR_REPLY,&retval) == FALSE)
	{
		DSP_Error_Number = 28;
		sprintf(DSP_Error_String,"DSP_Clear_Reply_Memory:Clear reply failed.");
		return FALSE;
	}
	return TRUE;
}

/**
 * Routine to set the destination board for a command request transmitted to the PCI interface.
 * This Command Destination register is used by the PCI interface for the LDA, TDL, RDM, WRM and
 * manual command invocations.
 * @param board_id The SDSU CCD Controller board to send the command to, one of 
 * 	CCD_DSP_INTERFACE_BOARD_ID(interface),
 * 	CCD_DSP_TIM_BOARD_ID(timing board) or CCD_DSP_UTIL_BOARD_ID(utility board).
 * @param argument_count This is also held in the destination register. This is one for TDL/LDA, two for
 * 	RDM and three for WRM.
 * @see ccd_pci.html#CCD_PCI_IOCTL_SET_DESTINATION
 */
static int DSP_Set_Destination(enum CCD_DSP_BOARD_ID board_id,int argument_count)
{
	int value;

	value = (argument_count << 16)|board_id;
	if(!CCD_Interface_Command(CCD_PCI_IOCTL_SET_DESTINATION,&value))
	{
		DSP_Error_Number = 30;
		sprintf(DSP_Error_String,"DSP_Set_Destination failed.");
		return FALSE;
	}
	return TRUE;
}

/**
 * Internal DSP command that sends a manual command the SDSU CCD Controller. A manual command is a 24 bit
 * 3 letter command (e.g. SGN) that a sent via the PCI interface to one of the controller boards. Most of
 * these commands have been superceeded by SDSU PCI commands, send using DSP_Send_Command. Legacy commands
 * and other old commands not supported directly via the new PCI interface command set can be sent using this
 * routine.
 * <ul>
 * <li>The reply memory is first cleared using DSP_Clear_Reply_Memory.
 * <li>The arguments to the command are passed to the PCI interface using CCD_Interface_Command 
 * 	(CCD_PCI_IOCTL_SET_ARGn).
 * <li>The command is sent using CCD_Interface_Command (CCD_PCI_IOCTL_SET_HCVR).
 * </ul>
 * @param cmdr_command The command number to put in the CMDR(Manual Command Register) register.
 * @param argument_list The list of arguments to be sent to the controller.
 * @param argument_count The number of arguments in the argument_list.
 * @return Returns true if no error occurs. If the command fails returns false.
 * @see ccd_interface.html#CCD_Interface_Command
 * @see #DSP_Clear_Reply_Memory
 * @see #DSP_Send_Command
 */
static int DSP_Send_Manual_Command(int cmdr_command,int *argument_list,int argument_count)
{
	int argument_index;

	if(DSP_Data.Abort)
	{
		DSP_Error_Number = 31;
		sprintf(DSP_Error_String,"DSP_Send_Manual_Command:Aborted.");		
		return FALSE;
	}
/* clear the reply memory */
	if(!DSP_Clear_Reply_Memory())
		return FALSE;
/* put the argument words into data memory */
	for(argument_index = 0;argument_index < argument_count;argument_index++)
	{
	/* This next line only works because CCD_PCI_IOCTL_SET_ARG1 to CCD_PCI_IOCTL_SET_ARG5
	** have contiguous numbering */
		if(!CCD_Interface_Command(CCD_PCI_IOCTL_SET_ARG1+argument_index,&(argument_list[argument_index])))
		{
			DSP_Error_Number = 32;
			sprintf(DSP_Error_String,"DSP_Send_Manual_Command:Set Argument(%d) failed.",argument_index);
			return FALSE;
		}
	}
/* send the command to device driver */
	if(!CCD_Interface_Command(CCD_PCI_IOCTL_SET_CMDR,&cmdr_command))
	{
		DSP_Error_Number = 33;
		sprintf(DSP_Error_String,"DSP_Send_Manual_Command:Sending command %#x failed.",cmdr_command);
		return FALSE;
	}
	return TRUE;
}

/**
 * Internal DSP command that sends a command to the SDSU CCD Controller. The command is a PCI interface type
 * command.
 * <ul>
 * <li>The reply memory is first cleared using DSP_Clear_Reply_Memory.
 * <li>The arguments to the command are passed to the PCI interface using CCD_Interface_Command 
 * 	(CCD_PCI_IOCTL_SET_ARGn).
 * <li>The command is sent using CCD_Interface_Command (CCD_PCI_IOCTL_SET_HCVR).
 * </ul>
 * @param hcvr_command The command number to put in the HCVR(Host Command Vector Register) register.
 * @param argument_list The list of arguments to be sent to the controller.
 * @param argument_count The number of arguments in the argument_list.
 * @return Returns true if no error occurs. If the command fails returns false.
 * @see ccd_interface.html#CCD_Interface_Command
 * @see #DSP_Clear_Reply_Memory
 */
static int DSP_Send_Command(int hcvr_command,int *argument_list,int argument_count)
{
	int argument_index;

	if(DSP_Data.Abort)
	{
		DSP_Error_Number = 34;
		sprintf(DSP_Error_String,"DSP_Send_Command:Aborted.");		
		return FALSE;
	}
/* clear the reply memory */
	if(!DSP_Clear_Reply_Memory())
		return FALSE;
/* put the argument words into data memory */
	for(argument_index = 0;argument_index < argument_count;argument_index++)
	{
	/* This next line works because CCD_PCI_IOCTL_SET_ARG1 to CCD_PCI_IOCTL_SET_ARG5
	** have contiguous numbering */
		if(!CCD_Interface_Command(CCD_PCI_IOCTL_SET_ARG1+argument_index,&(argument_list[argument_index])))
		{
			DSP_Error_Number = 35;
			sprintf(DSP_Error_String,"DSP_Send_Command:Set Argument(%d) failed.",argument_index);
			return FALSE;
		}
	}
/* send the command to device driver */
	if(!CCD_Interface_Command(CCD_PCI_IOCTL_SET_HCVR,&hcvr_command))
	{
		DSP_Error_Number = 36;
		sprintf(DSP_Error_String,"DSP_Send_Command:Sending command %#x failed.",hcvr_command);
		return FALSE;
	}
	return TRUE;
}

/**
 * This routine gets a reply from the SDSU CCD Controller. It checks that the reply is the expected_reply 
 * (unless expected_reply is 
 * <a href="#DSP_ACTUAL_VALUE">DSP_ACTUAL_VALUE</a>, in which case the reply is a value.
 * @param expected_reply What the reply should be. Normally it should be <a href="#CCD_DSP_DON">DON</a>, if an error
 *	occurs the CCD Controller will probably return <a href="#CCD_DSP_ERR">ERR</a> and an error is returned. If the
 * 	special value <a href="#DSP_ACTUAL_VALUE">DSP_ACTUAL_VALUE</a> is passed in no reply chacking is
 * 	performed (for instance, when the reply is a memory value from a RDM command).
 * @return Returns the expected reply value when that value is actually returned. If an error occurs FALSE is
 * 	returned. If an actual value was requested that is returned.
 * @see #CCD_DSP_ERR
 * @see #CCD_DSP_DON
 * @see #DSP_ACTUAL_VALUE
 */
static int DSP_Get_Reply(int expected_reply)
{
	int last_reply;

	/* read the reply from the device driver 
	** Before the command was send DSP_Send_Command clear the reply buffer (to -1).
	** This read of the reply buffer is done in a while until the reply buffer is not -1. */
	last_reply = -1;
	while((last_reply == -1)&&(DSP_Data.Abort == FALSE))
	{
		if(!CCD_Interface_Command(CCD_PCI_IOCTL_GET_REPLY,&last_reply))
		{
			DSP_Error_Number = 38;
			sprintf(DSP_Error_String,"DSP_Get_Reply:CCD_Interface_Command(%#x) failed.",
				CCD_PCI_IOCTL_GET_REPLY);
			return FALSE;
		}
	}
	if(DSP_Data.Abort)
	{
		DSP_Error_Number = 37;
		sprintf(DSP_Error_String,"DSP_Get_Reply:Aborted.");
		return FALSE;
	}

	/* If the expected reply was an actual value we can't test whether this is correct or not
	** so just return it */
	if(expected_reply == DSP_ACTUAL_VALUE)
	{
		return last_reply;
	}
	/* If the last_reply was ERR something went wrong with the last command */
	if(last_reply == CCD_DSP_ERR)
	{
		DSP_Error_Number = 39;
		sprintf(DSP_Error_String,"DSP_Get_Reply:Reply was ERR.");
		return FALSE;
 	}
	if(last_reply == expected_reply)
	{
		return last_reply; 
	}
	else /* the reply is not what we expected */
	{
		DSP_Error_Number = 40;
		sprintf(DSP_Error_String,"DSP_Get_Reply:Unexpected Reply(%#x,%#x).",
			last_reply,expected_reply);
		return FALSE;
	}
}

/**
 * This routine gets a line of data from a file pointer and puts it into a buffer. It is used when
 * downloading a DSP application program from a .lod file.
 * @param fp The file pointer to get input from.
 * @param buff The buffer to put the input into. The buffer must have room for at least 80 characters.
 * @return Returns TRUE if getting the line succeeded, FALSE otherwise.
 */
static int DSP_Read_Line(FILE *fp, char *buff)
{
	if (fgets(buff,80,fp) == NULL)
	{
		perror("error on input");
		strcpy(buff,"");
		return FALSE;
	}
	return TRUE;
}

/**
 * This routine gets which board the .lod file opened using fp is meant for. The routine is called
 * from <a href="#CCD_DSP_Download">CCD_DSP_Download</a>. Note this routine won't work for PCI DSP
 * downloads at the moment.
 * @param fp The file pointer to get the input from.
 * @return Returns which board the .lod file should be sent to (either
 * 	CCD_DSP_TIM_BOARD_ID(timing board) or CCD_DSP_UTIL_BOARD_ID(utility board)), 
 * 	otherwise returns FALSE.
 */
static int DSP_Get_Download_Type(FILE *fp)
{
	char    *cp, buff[255];
	int     id;

	DSP_Read_Line(fp,buff);
	if (strncmp(buff,"_START",6) != 0)
		return FALSE;

	cp = buff+7;
	switch(*cp)
	{
		case 'T':
			id = CCD_DSP_TIM_BOARD_ID;
			break;
		case 'U':
			id = CCD_DSP_UTIL_BOARD_ID;
			break;
		default:
			return FALSE;
	}
	return(id);
}

/**
 * Routine to convert a address type character, read from a '_DATA P 4000' type statement in a .lod file,
 * into a memory space enum value, suitable for passing to a PCI write memory command.
 * @param ch The address type character.
 * @param mem_space The address of a variable to store the converted memory space.
 * @return Returns TRUE if the conversiob succeeded, FALSE if it failed.
 */
static int DSP_Address_Char_To_Mem_Space(char ch,enum CCD_DSP_MEM_SPACE *mem_space)
{
	switch(ch)
	{
		case 'D':
			(*mem_space) = CCD_DSP_MEM_SPACE_D;
			break;
		case 'R':
			(*mem_space) = CCD_DSP_MEM_SPACE_R;
			break;
		case 'P':
			(*mem_space) = CCD_DSP_MEM_SPACE_P;
			break;
		case 'X':
			(*mem_space) = CCD_DSP_MEM_SPACE_X;
			break;
		case 'Y':
			(*mem_space) = CCD_DSP_MEM_SPACE_Y;
			break;
		default:
			(*mem_space) = 0;
			DSP_Error_Number = 41;
			sprintf(DSP_Error_String,"DSP_Address_Char_To_Mem_Space:Illegal value '%c'.",ch);
			break;
	}
	return ((*mem_space) != 0);
}

/**
 * This routine reads DSP program code from file download_fp and writes it to DSP memory
 * using the <a href="#CCD_DSP_WRM">WRM</a> command to board board_id, memory space type address addr
 * @param download_fp The file pointer of the .lod file we are loading the program from.
 * @param board_id The board to send the command to. One of
 * 	CCD_DSP_TIM_BOARD_ID(timing board) or CCD_DSP_UTIL_BOARD_ID(utility board).
 * @param mem_space The memory space to put the data into, of type 
 * 	<a href="#CCD_DSP_MEM_SPACE">CCD_DSP_MEM_SPACE</a>. One of:
 * 	CCD_DSP_MEM_SPACE_D(DRAM),
 * 	CCD_DSP_MEM_SPACE_P(program),
 * 	CCD_DSP_MEM_SPACE_X(X data),
 * 	CCD_DSP_MEM_SPACE_Y(Y data)
 * 	or CCD_DSP_MEM_SPACE_R(ROM).
 * @param addr The address within the memory space.
 * @return Returns TRUE if the operation succeeds, FALSE if it fails.
 */
static int DSP_Process_Data(FILE *download_fp,enum CCD_DSP_BOARD_ID board_id,
	enum CCD_DSP_MEM_SPACE mem_space,int addr)
{ 
	int     finished, value,ret_value;
	char    c;

	finished = FALSE;
	/* while theres data to download and we've not aborted the operation */
	while ((!finished) && (!DSP_Data.Abort))
	{
		/* ignore spaces */
		while ((c = getc(download_fp)) == 32);
		/* if we get an underscore it's probably the start of an _END or _DATA - hence stop */
		if(c == '_')
		{
			ungetc(c, download_fp);
			finished = TRUE;
		}
		/* it it's not a newline it must be actual data */
		else if((c != 10)&&(c != 13))
		{
			/* put the byte back */
			ungetc(c, download_fp);
			/* read the whole word of hexadecimal data */
			fscanf(download_fp, "%x", &value);
			/* try writing it to a board */
			if(!CCD_DSP_Command_WRM(board_id,mem_space,addr,value))
				return FALSE;
			addr++;
		}
	}
	return(TRUE);
}

/**
 * This routine is called for SEX and RDC commands to actually 
 * perform an exposure. 
 * If the board_id is CCD_DSP_UTIL_BOARD_ID,it sends the SEX command and waits
 * tot_msecs milliseconds exposure time. It then sets the interface word width to 1, allocates sufficient memory
 * for the resultant image (numbytes), and reads the image into memory using 
 * <a href="ccd_interface.html#CCD_Interface_Get_Reply_Data">CCD_Interface_Get_Reply_Data</a>. It then resets
 * the interface word width and deinterlaces the image using <a href="#DSP_DeInterlace">DSP_DeInterlace</a>.
 * The image is then saved using <a href="#DSP_Save">DSP_Save</a> to a file and the image data freed.
 * The Exposure_Status variable in <a href="#DSP_Data">DSP_Data</a> is maintained to show the current
 * status of the exposure.
 * If at any point the exposure is aborted using <a href="#CCD_DSP_Set_Abort">CCD_DSP_Set_Abort</a> the routine
 * aborts when it next checks the Abort flag.
 * @param ncols The number of columns the CCD has.
 * @param nrows The number of rows the CCD has.
 * @param numbytes The number of bytes the image will fit into.
 * @param deinterlace_type What deinterlacing algorithm to perform on the raw data.
 * @param filename The filename to save the deinterlaced image to.
 * @return Returns TRUE if the exposure succeeded, FALSE if it failed or was aborted.
 * @see #CCD_DSP_Set_Abort
 * @see #DSP_DeInterlace
 * @see #DSP_Save
 * @see ccd_interface.html#CCD_Interface_Get_Reply_Data
 */
static int DSP_Image_Transfer(int ncols,int nrows,int numbytes,
	enum CCD_DSP_DEINTERLACE_TYPE deinterlace_type,char *filename)
{
	int retval;
	char *file_mem = NULL;
	char *expdata = NULL;

	/* if we have aborted stop here */
	if(DSP_Data.Abort)
	{
		DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_NONE;
		DSP_Error_Number = 42;
		sprintf(DSP_Error_String,"DSP_Image_Transfer:Aborted.");
		return FALSE;
	}
	/* start reading out image */
	DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_READOUT;
	expdata = (char *)malloc(numbytes);
	if(expdata == NULL)
	{
		DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_NONE;
		DSP_Error_Number = 43;
		sprintf(DSP_Error_String,"DSP_Image_Transfer:Memory Allocation Error(%d).",numbytes);
		return FALSE;
	}
	retval = CCD_Interface_Get_Reply_Data(expdata,numbytes);
	if(retval == -1)
	{
		if(expdata != NULL)
			free(expdata);
		DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_NONE;
		perror("DSP_Image_Transfer: Error on read, no data transfered");
		DSP_Error_Number = 44;
		sprintf(DSP_Error_String,"DSP_Image_Transfer:Failed to get reply data.");
		return FALSE;
	}
	else if(retval != numbytes)
	{
		DSP_Error_Number = 45;
		sprintf(DSP_Error_String,"DSP_Image_Transfer:expected %d bytes but received %d bytes.",
			numbytes,retval);
		CCD_DSP_Warning();
	}
	if(DSP_Data.Abort)
	{
		if(expdata != NULL)
			free(expdata);
		DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_NONE;
		DSP_Error_Number = 46;
		sprintf(DSP_Error_String,"DSP_Image_Transfer:Aborted.");
		return FALSE;
	}
	DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_NONE;

	/* Do deinterlacing. The image returned from the boards may not be in the correct order
	** if the CCD was readout from multiple places etc. The deinterlace routine reorders the image
	** so that it is back in the right order.
	** Note if this routine fails it currently returns the interlaced data by returning the file_mem
	** allocated memory area. This is so data gets saved even if deinterlacing fails. It means a
	** memory allocation failure will still cause the expdata returned to be non-null.  */
	file_mem = expdata;
	if((expdata = (char *)DSP_DeInterlace(ncols,nrows,(unsigned short*)file_mem,deinterlace_type)) == NULL)
	{
		if(file_mem == NULL)
			free(file_mem);
		DSP_Error_Number = 47;
		sprintf(DSP_Error_String,"DSP_Image_Transfer:DeInterlace failed.");
		return FALSE;
	}
	/* free the old copy of the data 
	** Note if DSP_DeInterlace failed expdata == file_mem and the data is still interlaced.*/
	if(expdata != file_mem)
		free(file_mem);

	/* if we have aborted stop and return */
	if(DSP_Data.Abort)
	{
		if(expdata != NULL)
			free(expdata);
		DSP_Error_Number = 48;
		sprintf(DSP_Error_String,"DSP_Image_Transfer:Aborted.");
		return FALSE;
	}
	/* save the resultant image to disk */
	if(!DSP_Save(filename,expdata,ncols,nrows,numbytes))
	{
		if(expdata != NULL)
			free(expdata);
		return FALSE;
	}
	/* free image in memory */
	if(expdata != NULL)
		free(expdata);
	return TRUE;
}

/**
 * This routine deinterlaces a raw image read from the ccd. If the ccd has more than one
 * readout port, the data will not be received in row-column order and will need deinterlacing.
 * The deinterlacing algorithms work on the principle that the ccd will read out the data in a 
 * predetermined order depending on the type of readout being implemented. Here's how they look:
 * <pre>                                                                          
 *   split-parallel               split-serial            split-quad         
 *  ----------------            ----------------        ----------------     
 * |     2  ------->|          |        |------>|      |<-----  |  ---->|    
 * |                |          |        |   2   |      |   4    |   3   |    
 * |                |          |        |       |      |        |       |    
 * |_______________ |          |        |       |      |________|_______|    
 * |                |          |        |       |      |        |       |    
 * |                |          |        |       |      |        |       |    
 * |                |          |   1    |       |      |   1    |   2   |    
 * |<--------  1    |          |<------ |       |      |<-----  |  ---->|    
 *  ----------------            ----------------        ----------------     
 * </pre>
 * <em>Note: This routine assumes 
 * <a href="ccd_global.html#CCD_GLOBAL_BYTES_PER_PIXEL">CCD_GLOBAL_BYTES_PER_PIXEL</a>== 2 e.g. 
 * 16 bits per pixel.</em>
 * @param ncols The number of columns on the CCD.
 * @param nrows The number of rows on the CCD.
 * @param old_iptr The interlaced image data received from the CCD.
 * @param deinterlace_type The type of deinterlacing to perform. One of 
 * 	<a href="ccd_setup.html#CCD_DSP_DEINTERLACE_TYPE">CCD_DSP_DEINTERLACE_TYPE</a>:
 * 	CCD_DSP_DEINTERLACE_SINGLE,
 * 	CCD_DSP_DEINTERLACE_SPLIT_PARALLEL,
 * 	CCD_DSP_DEINTERLACE_SPLIT_SERIAL or
 * 	CCD_DSP_DEINTERLACE_SPLIT_QUAD.
 * @return If everything was successful this is a newly allocated area of memory,
 * 	containing the deinterlaced image data. If something failed the pointer to the old_iptr memory area,
 * 	containing the undelinterlaced image, is returned so that the interlaced image data can be saved for
 * 	deinterlacing later.
 */
static unsigned short *DSP_DeInterlace(int ncols,int nrows,unsigned short *old_iptr,
	enum CCD_DSP_DEINTERLACE_TYPE deinterlace_type)
{
	unsigned short *new_iptr;

	/* allocate enough memory to store the deinterlaced image */
	if((new_iptr = (unsigned short *)malloc(ncols*nrows*sizeof(unsigned short))) == NULL)
	{
 		DSP_Error_Number = 49;
		sprintf(DSP_Error_String,"DSP_DeInterlace:Memory Allocation Error(%d,%d),image not deinterlaced.",
			ncols,nrows);
		return(old_iptr);
	}
	switch(deinterlace_type)
	{
		/* SINGLE READOUT 
		** The result is the same as the input. The CCD was readout from one port, so it
		** was readout in order */
		case CCD_DSP_DEINTERLACE_SINGLE:
		{
			memcpy(new_iptr,old_iptr,(size_t)(ncols*nrows*(sizeof(unsigned short))));
			return(new_iptr);
		} /*end single readout*/
		/* SPLIT PARALLEL READOUT */
		case CCD_DSP_DEINTERLACE_SPLIT_PARALLEL:
		{
			int i;

			if (((float)nrows/2) != (int)nrows/2)
			{
 				DSP_Error_Number = 50;
				sprintf(DSP_Error_String,"DSP_DeInterlace:Split Parallel Readout,"
					"nrows not even(%d), image not deinterlaced.",nrows);
				free(new_iptr);
				return(old_iptr);
			}
			for (i=0;i<(ncols*nrows)/2;i++)
			{
				*(new_iptr+i) = *(old_iptr+(2*i));
				*(new_iptr+(ncols*nrows)-i-1) = *(old_iptr+(2*i)+1);
			}
			memcpy(old_iptr,new_iptr,ncols*nrows*sizeof(unsigned short));
			return(new_iptr);
		} /*end split parallel*/
		/* SPLIT SERIAL READOUT */
		case CCD_DSP_DEINTERLACE_SPLIT_SERIAL:
		{
			int i,j,p1,p2,begin,end;
			if ((float)ncols/2 != (int)ncols/2)
        		{
 				DSP_Error_Number = 51;
				sprintf(DSP_Error_String,"DSP_DeInterlace:Split Serial Readout,"
					"ncols not even(%d), image not deinterlaced.",ncols);
				free(new_iptr);
				return(old_iptr);
			}
			for (i=0;i<nrows;i++)
			{		 /* leave in +0 for clarity */
				p1      = i*ncols+0; /*position in raw image */
				p2      = i*ncols+1;
				begin   = i*ncols+0; /*position in interlaced image */
				end     = i*ncols+ncols-1;
				for(j=0;j<ncols;j+=2)
                		{
                			*(new_iptr+begin) = *(old_iptr+p1);
                			*(new_iptr+end) = *(old_iptr+p2);
                			++begin;
                			--end;
                			p1+=2;
                			p2+=2;
                		}
                	}
			memcpy(old_iptr,new_iptr,ncols*nrows*sizeof(unsigned short));
			return(new_iptr);
		} /*end split serial*/
		/* SPLIT QUAD READOUT */
		case CCD_DSP_DEINTERLACE_SPLIT_QUAD:
		{
			int i=0,j=0,counter=0,end=0,begin=0;

			if ((float)ncols/2 != (int)ncols/2 || (float)nrows/2 != (int)nrows/2)
			{
 				DSP_Error_Number = 52;
				sprintf(DSP_Error_String,"DSP_DeInterlace:Split Quad Readout,"
					"ncols or nrows not even(%d,%d), image not deinterlaced.",ncols,nrows);
				free ( new_iptr );
				return(old_iptr);
			}
			while(i<ncols*nrows)
			{
				if(counter%(ncols/2) == 0)
				{
					end     = (ncols*nrows)-(ncols*j)-1;
					begin   = (ncols*j)+0; /* left in 0 for clarity*/
					j++; /*number of completed rows*/
					counter=0; /*reset for next convergece*/
				}
				*(new_iptr+begin+counter)       = *(old_iptr+i++); /*front_row--->  */
				*(new_iptr+begin+ncols-1-counter)   = *(old_iptr+i++); /*front_row<--   */
				*(new_iptr+end-counter)         = *(old_iptr+i++); /*end_row<----   */
				*(new_iptr+end-ncols+1+counter)     = *(old_iptr+i++); /*end_row---->   */
				counter++;
			}
			memcpy(old_iptr,new_iptr,ncols*nrows*sizeof(unsigned short));
			return(new_iptr);
		} /*end split quad readout*/
	}/*end switch*/
	DSP_Error_Number = 53;
	sprintf(DSP_Error_String,"DSP_DeInterlace:Wrong DeInterlace option(%d),Image not deinterlaced.",
		deinterlace_type);
	return(old_iptr);
} /* end deinterlacing */

/* 
** DSP_Save uses a different implementation depending on whether CFITSIO define was defined at compile time.
** If it was we use CFITSIO routines, otherwise we don't.
*/

#ifdef CFITSIO
/**
 * This routine takes some image data and saves it in a file on disc.
 * @param filename The filename to save the data into.
 * @param exposure_data The data to save.
 * @param ncols The number of columns in the image data.
 * @param nrows The number of rows in the image data.
 * @param number_bytes The number of bytes of data to save to disc.
 * @return Returns TRUE if the image is saved successfully, FALSE if it fails.
 */
static int DSP_Save(char *filename,char *exposure_data,int ncols,int nrows,int number_bytes)
{
	fitsfile *fp = NULL;
	int retval=0,status=0;
	long axes_list[2] = { 0,0};
	char buff[32]; /* fits_get_errstatus returns 30 chars max */

	/* try to open file */
	retval = fits_open_file(&fp,filename,READWRITE,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		DSP_Error_Number = 54;
		sprintf(DSP_Error_String,"DSP_Save: File open failed(%s,%d,%s).",filename,status,buff);
		return FALSE;
	}
	/* create image block */
/*diddly this doesn't work here
	axes_list[0] = ncols;
	axes_list[1] = nrows;
	retval = fits_create_img(fp,USHORT_IMG,2,axes_list,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		DSP_Error_Number = diddly;
		sprintf(DSP_Error_String,"DSP_Save: File create image(%s,%d,%s).",filename,status,buff);
		return FALSE;
	}
*/
	/* write the data */
	retval = fits_write_img(fp,TUSHORT,1,number_bytes/sizeof(unsigned short),exposure_data,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fp,&status);
		DSP_Error_Number = 55;
		sprintf(DSP_Error_String,"DSP_Save: File write failed(%s,%d,%s).",filename,status,buff);
		return FALSE;
	}
	retval = fits_close_file(fp,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		DSP_Error_Number = 56;
		sprintf(DSP_Error_String,"DSP_Save: File close failed(%s,%d,%s).",filename,status,buff);
		return FALSE;
	}
	return TRUE;
}
#else
/**
 * This routine takes some image data and saves it in a file on disc.
 * @param filename The filename to save the data into.
 * @param exposure_data The data to save.
 * @param ncols The number of columns in the image data.
 * @param nrows The number of rows in the image data.
 * @param number_bytes The number of bytes of data to save to disc.
 * @return Returns TRUE if the image is saved successfully, FALSE if it fails.
 */
static int DSP_Save(char *filename,char *exposure_data,int ncols,int nrows,int number_bytes)
{
	FILE *fp = NULL;
	int retval,error_number;

	/* try to open file */
	fp = fopen(filename,"rb+");
	if(fp == NULL)
	{
		error_number = errno;
		DSP_Error_Number = 57;
		sprintf(DSP_Error_String,"DSP_Save: File open failed(%s,%d).",filename,error_number);
		return FALSE;
	}
	/* move to end of file */
	retval = fseek(fp,0,SEEK_END);
	if(retval == -1)
	{
		fclose(fp);
		DSP_Error_Number = 58;
		sprintf(DSP_Error_String,"DSP_Save: File seek failed(%s,%d,%s).",filename,errno,strerror(errno));
		return FALSE;
	}
	/* write the data */
	retval = fwrite(exposure_data,sizeof(char),number_bytes,fp);
	if(retval != number_bytes)
	{
		fclose(fp);
		DSP_Error_Number = 59;
		sprintf(DSP_Error_String,"DSP_Save: File write failed(%s,%d,%d).",filename,retval,number_bytes);
		return FALSE;
	}
	fclose(fp);
	return TRUE;
}
#endif

/*
** $Log: not supported by cvs2svn $
** Revision 0.5  2000/02/22 17:38:17  cjm
** Added CCD_DSP_EXPOSURE_STATUS_CLEAR status.
**
** Revision 0.4  2000/02/10 12:01:25  cjm
** Added CCD_DSP_Command_WRM_No_Reply routine for special case of switching off controller replies.
**
** Revision 0.3  2000/02/02 15:50:01  cjm
** Added CCD_DSP_Command_POF routine.
**
** Revision 0.2  2000/01/27 15:38:47  cjm
** Fixed Clear_Reply_Memory so that it no longer checks that retval = -1.
** This appears not to be the case.
**
** Revision 0.1  2000/01/25 14:57:27  cjm
** initial revision (PCI version).
**
*/

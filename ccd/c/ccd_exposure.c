/* ccd_exposure.c -*- mode: Fundamental;-*-
** low level ccd library
** $Header: /home/cjm/cvs/frodospec/ccd/c/ccd_exposure.c,v 0.14 2000-09-25 09:51:28 cjm Exp $
*/
/**
 * ccd_exposure.c contains routines for performing an exposure with the SDSU CCD Controller. There is a
 * routine that does the whole job in one go, or several routines can be called to do parts of an exposure.
 * An exposure can be paused and resumed, or it can be stopped or aborted.
 * @author SDSU, Chris Mottram
 * @version $Revision: 0.14 $
 */
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes
 * for time.
 */
#define _POSIX_SOURCE 1
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes
 * for time.
 */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include "ccd_exposure.h"
#include "ccd_dsp.h"
#include "ccd_setup.h"

/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id: ccd_exposure.c,v 0.14 2000-09-25 09:51:28 cjm Exp $";

/* external variables */

/* internal variables */
/**
 * Variable holding error code of last operation performed by ccd_exposure.
 */
static int Exposure_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 */
static char Exposure_Error_String[CCD_GLOBAL_ERROR_STRING_LENGTH] = "";

/* internal functions */
static int Exposure_Shutter_Control(int value);

/* external functions */
/**
 * This routine sets up ccd_exposure internal variables.
 * It should be called at startup.
 */
void CCD_Exposure_Initialise(void)
{
	Exposure_Error_Number = 0;
/* print some compile time information to stdout */
	fprintf(stdout,"CCD_Exposure_Initialise:%s.\n",rcsid);
}

/**
 * Routine to perform an exposure.
 * <ul>
 * <li>It checks to ensure CCD Setup has been successfully completed using CCD_Setup_Get_Setup_Complete.
 * <li>The controller is told whether to open the shutter or not during the exposure, depending on the value
 * 	of the open_shutter parameter.
 * <li>The length of exposure is sent to the controller.
 * <li>The exposure is performed by calling 
 * 	<a href="ccd_dsp.html#CCD_DSP_Command_SEX">CCD_DSP_Command_SEX</a> to do the exposure.
 *	This then read outs the exposure and saves it to disk.
 * </ul>
 * If the exposure is aborted at any stage the routine returns.
 * @param open_shutter TRUE if the shutter is to be opened over the duration of the exposure, FALSE if the
 * 	shutter should remain closed. The shutter may not want to be opened if a calibration image is
 * 	being taken.
 * @param start_time The time to start the exposure. If both the fields in the <i>struct timespec</i> are zero,
 * 	the exposure can be started at any convenient time.
 * @param exposure_time The length of time to open the shutter for in milliseconds.
 * @param filename The filename to save the exposure into.
 * @return Returns TRUE if the exposure succeeds and the file is saved, returns FALSE if an error
 *	occurs or the exposure is aborted.
 * @see #Exposure_Shutter_Control
 * @see ccd_setup.html#CCD_Setup_Get_Setup_Complete
 * @see ccd_setup.html#CCD_Setup_Get_NCols
 * @see ccd_setup.html#CCD_Setup_Get_NRows
 * @see ccd_setup.html#CCD_Setup_Get_DeInterlace_Type
 * @see ccd_dsp.html#CCD_DSP_Command_Set_Exposure_Time
 * @see ccd_dsp.html#CCD_DSP_Command_SEX
 */
int CCD_Exposure_Expose(int open_shutter,struct timespec start_time,int exposure_time,char *filename)
{
	int ncols;
	int nrows;
	enum CCD_DSP_DEINTERLACE_TYPE deinterlace_type;

	Exposure_Error_Number = 0;
/* reset abort flag */
	CCD_DSP_Set_Abort(FALSE);
/* we shouldn't be able to expose until setup has been successfully completed - check this */
	if(!CCD_Setup_Get_Setup_Complete())
	{
		Exposure_Error_Number = 1;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose:Exposure failed:Setup was not complete");
		return FALSE;
	}
/* check paramater ranges */
	if(!CCD_GLOBAL_IS_BOOLEAN(open_shutter))
	{
		Exposure_Error_Number = 2;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose:Illegal value:open_shutter = %d.",
			open_shutter);
		return FALSE;
	}
	if(exposure_time <= 0)
	{
		Exposure_Error_Number = 3;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose:Illegal value:exposure_time = %d",exposure_time);
		return FALSE;
	}
/* get information from setup that we need to do an exposure */
	ncols = CCD_Setup_Get_NCols();
	nrows = CCD_Setup_Get_NRows();
	deinterlace_type = CCD_Setup_Get_DeInterlace_Type();
/* if we have aborted - stop here */
	if(CCD_DSP_Get_Abort())
	{
		CCD_DSP_Set_Abort(FALSE);
		Exposure_Error_Number = 4;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose:Aborted");
		return FALSE;
	}
/* setup the shutter control bit - which determines whether the SEX command has
** control to open and close the shutter at the appropriate times */
	if(!Exposure_Shutter_Control(open_shutter))
	{
		CCD_DSP_Set_Abort(FALSE);
		return FALSE;
	}
	if(CCD_DSP_Get_Abort())
	{
		CCD_DSP_Set_Abort(FALSE);
		Exposure_Error_Number = 5;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose:Aborted");
		return FALSE;
	}
/* write the time to memory so that SEX can read it */
	if(!CCD_DSP_Command_Set_Exposure_Time(exposure_time))
	{
		CCD_DSP_Set_Abort(FALSE);
		Exposure_Error_Number = 23;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose:Setting exposure time failed.");
		return FALSE;
	}
	if(CCD_DSP_Get_Abort())
	{
		CCD_DSP_Set_Abort(FALSE);
		Exposure_Error_Number = 25;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose:Aborted");
		return FALSE;
	}
/* Send the command to start the exposure, and monitor for completion. */
	if(!CCD_DSP_Command_SEX(start_time,exposure_time,ncols,nrows,deinterlace_type,filename))
	{
		CCD_DSP_Set_Abort(FALSE);
		Exposure_Error_Number = 24;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose:SEX command failed.");
		return FALSE;
	}
	return TRUE;
}

/**
 * Routine to take a bias frame. This is effectively a combination of the CCD_DSP_Command_CLR, CCD_DSP_Command_RDC 
 * and CCD_DSP_Command_RDI routine. A bias frame is taken by clearing the CCD array using a 
 * clear command to clear the array, followed immediately by a 
 * read-out command to read the array into a FITS file.
 * @param filename The filename to save the resultant data (in FITS format) to.
 * @return The routine returns TRUE if the operation was completed successfully, FALSE if it failed.
 * @see ccd_dsp.html#CCD_DSP_Command_CLR
 * @see ccd_dsp.html#CCD_DSP_Command_RDC
 * @see ccd_dsp.html#CCD_DSP_Command_RDI
 */
int CCD_Exposure_Bias(char *filename)
{
	int ncols;
	int nrows;
	enum CCD_DSP_DEINTERLACE_TYPE deinterlace_type;
	int return_value;	/* value returned from function calls */
#if DEBUG == 1
	int debug;
#endif

	Exposure_Error_Number = 0;
/* reset abort flag */
	CCD_DSP_Set_Abort(FALSE);
/* we shouldn't be able to expose until setup has been successfully completed - check this */
	if(!CCD_Setup_Get_Setup_Complete())
	{
		Exposure_Error_Number = 6;
		sprintf(Exposure_Error_String,"Exposure Bias failed:Setup was not complete");
		return FALSE;
	}
/* get information from setup that we need to do an exposure */
	ncols = CCD_Setup_Get_NCols();
	nrows = CCD_Setup_Get_NRows();
	deinterlace_type = CCD_Setup_Get_DeInterlace_Type();
/* if debugging, get PCI/timing board status and print */
#if DEBUG == 1
	debug = CCD_DSP_Command_Read_PCI_Status();	
	fprintf(stdout,"PCI_STATUS = %#x\n",debug);
	fflush(stdout);
#endif
/* clear the ccd */
	if(!CCD_DSP_Command_CLR())
	{
		CCD_DSP_Set_Abort(FALSE);
		Exposure_Error_Number = 7;
		sprintf(Exposure_Error_String,"CCD_Exposure_Bias:Clear array failed.");
		return FALSE;
	}
/* stop idle clocking on the timing board */
	if(!CCD_DSP_Command_STP())
	{
		CCD_DSP_Set_Abort(FALSE);
		Exposure_Error_Number = 8;
		sprintf(Exposure_Error_String,"CCD_Exposure_Bias:Stop Idle clocking failed.");
		return FALSE;
	}
/* remember, here DSP_Data.Exposure_Status has been set. 
** therefore we can't check for abort here. 
** If we did, we would have to IDL the Timing board clocks to stop
** the electronics locking up. */
/* Bias frames do not call start exposure and hence the exposure start time is not set
** It is saved in the FITS file however, so set it manually here. */
	CCD_DSP_Set_Exposure_Start_Time();
/* tell the timing board to readout the ccd */
	if(!CCD_DSP_Command_RDC())
	{
		CCD_DSP_Set_Abort(FALSE);
		Exposure_Error_Number = 29;
		sprintf(Exposure_Error_String,"CCD_Exposure_Bias:Read CCD failed.");
		return FALSE;
	}
/* remember, here DSP_Data.Exposure_Status has been set. 
** therefore we can't check for abort here. 
** If we did, we would have to IDL the Timing board clocks to stop
** the electronics locking up. */
/* read out ccd and save to filename */
	return_value = CCD_DSP_Command_RDI(ncols,nrows,deinterlace_type,TRUE,filename);
	if(return_value == FALSE)
	{
		CCD_DSP_Set_Abort(FALSE);
		Exposure_Error_Number = 9;
		sprintf(Exposure_Error_String,"CCD_Exposure_Bias:Readout failed.");
		return FALSE;
	}
/* re-start idle clocking on the timing board */
/* diddly, what if idling is set to false in the configuration - we don't want to do this */
	if(!CCD_DSP_Command_IDL())
	{
		CCD_DSP_Set_Abort(FALSE);
		Exposure_Error_Number = 10;
		sprintf(Exposure_Error_String,"CCD_Exposure_Bias:Starting Idle clocking failed.");
		return FALSE;
	}
	return TRUE;
}

/**
 * This routine would not normally be called as part of an exposure sequence. It simply opens the shutter by 
 * executing an Open Shutter command.
 * @return The routine returns TRUE if the operation succeeded, FALSE if it fails.
 * @see ccd_dsp.html#CCD_DSP_Command_OSH
 */
int CCD_Exposure_Open_Shutter(void)
/* a seperarate command to the main exposure sequence */
{
	Exposure_Error_Number = 0;
/* reset abort flag */
	CCD_DSP_Set_Abort(FALSE);
	if(!CCD_DSP_Command_OSH())
	{
		CCD_DSP_Set_Abort(FALSE);
		Exposure_Error_Number = 11;
		sprintf(Exposure_Error_String,"CCD_Exposure_Open_Shutter:Open shutter failed.");
		return FALSE;
	}
	return TRUE;
}

/**
 * This routine would not normally be called as part of an exposure sequence. It simply closes the shutter by 
 * executing a close shutter command.
 * @return The routine returns TRUE if the operation succeeded, FALSE if it fails.
 * @see ccd_dsp.html#CCD_DSP_Command_CSH
 */
int CCD_Exposure_Close_Shutter(void)
/* a seperarate command to the main exposure sequence */
{
	Exposure_Error_Number = 0;
/* reset abort flag */
	CCD_DSP_Set_Abort(FALSE);
	if(!CCD_DSP_Command_CSH())
	{
		CCD_DSP_Set_Abort(FALSE);
		Exposure_Error_Number = 12;
		sprintf(Exposure_Error_String,"CCD_Exposure_Close_Shutter:Close shutter failed.");
		return FALSE;
	}
	return TRUE;
}

/**
 * This routine pauses an exposure currently underway by 
 * executing a pause exposure command.
 * The timer is paused until the exposure is resumed.
 * @return The routine returns TRUE if the operation succeeded, FALSE if it fails.
 * @see #CCD_Exposure_Resume
 * @see ccd_dsp.html#CCD_DSP_Command_PEX
 */
int CCD_Exposure_Pause(void)
/* a seperarate command to the main exposure sequence */
{
	Exposure_Error_Number = 0;
/* reset abort flag */
	CCD_DSP_Set_Abort(FALSE);
	if(!CCD_DSP_Command_PEX())
	{
		CCD_DSP_Set_Abort(FALSE);
		Exposure_Error_Number = 13;
		sprintf(Exposure_Error_String,"CCD_Exposure_Pause:Pause command failed.");
		return FALSE;
	}
	return TRUE;
}

/**
 * This routine resumes a paused exposure by 
 * executing a resume exposure command.
 * @return The routine returns TRUE if the operation succeeded, FALSE if it fails.
 * @see #CCD_Exposure_Pause
 * @see ccd_dsp.html#CCD_DSP_Command_REX
 */
int CCD_Exposure_Resume(void)
/* a seperarate command to the main exposure sequence */
{
	Exposure_Error_Number = 0;
/* reset abort flag */
	CCD_DSP_Set_Abort(FALSE);
	if(!CCD_DSP_Command_REX())
	{
		CCD_DSP_Set_Abort(FALSE);
		Exposure_Error_Number = 14;
		sprintf(Exposure_Error_String,"CCD_Exposure_Resume:Resume command failed.");
		return FALSE;
	}
	return TRUE;
}

/**
 * This routine aborts an exposure currenly underway, but not reading out. The current status of an exposure can
 * be found with <a href="ccd_dsp.html#CCD_DSP_Get_Exposure_Status">CCD_DSP_Get_Exposure_Status</a>, and if it
 * CCD_DSP_EXPOSURE_STATUS_EXPOSE this routine should be called.
 * If the exposure status is CCD_DSP_EXPOSURE_STATUS_READOUT
 * <a href="#CCD_Exposure_Abort_Readout">CCD_Exposure_Abort_Readout</a> should be called instead. 
 * This routine executes a AEX command, and sets the Abort 
 * flag to true by calling <a href="ccd_dsp.html#CCD_DSP_Set_Abort">CCD_DSP_Set_Abort(TRUE)</a>.
 * @see #CCD_Exposure_Expose
 * @see #CCD_Exposure_Abort_Readout
 * @see ccd_dsp.html#CCD_DSP_Set_Abort
 * @see ccd_dsp.html#CCD_DSP_EXPOSURE_STATUS
 */
void CCD_Exposure_Abort(void)
{
	Exposure_Error_Number = 0;
	if(CCD_DSP_Get_Exposure_Status() == CCD_DSP_EXPOSURE_STATUS_EXPOSE)
	{
		if(CCD_DSP_Command_AEX() != CCD_DSP_DON)
		{
			Exposure_Error_Number = 15;
			sprintf(Exposure_Error_String,"CCD_Exposure_Abort:AEX command failed.");
			CCD_Exposure_Warning();
		}
		if(CCD_DSP_Command_PCI_PC_Reset() != CCD_DSP_DON)
		{
			Exposure_Error_Number = 26;
			sprintf(Exposure_Error_String,"CCD_Exposure_Abort:PCI board PC reset command failed.");
			CCD_Exposure_Warning();
		}
#ifndef CCD_FLUSH_REPLY_BUFFER_PER_COMMAND
		if(!CCD_DSP_Command_Flush_Reply_Buffer())
		{
			Exposure_Error_Number = 27;
			sprintf(Exposure_Error_String,"CCD_Exposure_Abort:Flushing reply buffer failed.");
			CCD_Exposure_Warning();
		}
#endif
	}
	else
	{
		Exposure_Error_Number = 16;
		sprintf(Exposure_Error_String,"CCD_Exposure_Abort:Exposure Status was not expose");
		CCD_Exposure_Warning();
	}
	CCD_DSP_Set_Abort(TRUE);
}

/**
 * This routine aborts an exposure that is currently reading out. The current status of an exposure can
 * be found with <a href="ccd_dsp.html#CCD_DSP_Get_Exposure_Status">CCD_DSP_Get_Exposure_Status</a>, and if it
 * CCD_DSP_EXPOSURE_STATUS_READOUT this routine should be called.
 * If the exposure status is CCD_DSP_EXPOSURE_STATUS_EXPOSE
 * <a href="#CCD_Exposure_Abort">CCD_Exposure_Abort</a> should be called instead. 
 * This routine calls the <a href="ccd_dsp.html#CCD_DSP_Command_Abr">CCD_DSP_Command_Abr</a> 
 * command routine to tell the SDSU CCD Controller to stop the readout,
 * and sets the Abort flag to true by calling
 * <a href="ccd_dsp.html#CCD_DSP_Set_Abort">CCD_DSP_Set_Abort(TRUE)</a>.
 * @see #CCD_Exposure_Expose
 * @see #CCD_Exposure_Abort
 * @see ccd_dsp.html#CCD_DSP_Set_Abort
 * @see ccd_dsp.html#CCD_DSP_EXPOSURE_STATUS
 */
void CCD_Exposure_Abort_Readout(void)
{
	Exposure_Error_Number = 0;
	if(CCD_DSP_Get_Exposure_Status() == CCD_DSP_EXPOSURE_STATUS_READOUT)
	{
		if(CCD_DSP_Command_ABR() != CCD_DSP_DON)
		{
			Exposure_Error_Number = 17;
			sprintf(Exposure_Error_String,"CCD_Exposure_Abort_Readout:ABR command failed.");
			CCD_Exposure_Warning();
		}
		if(CCD_DSP_Command_PCI_PC_Reset() != CCD_DSP_DON)
		{
			Exposure_Error_Number = 30;
			sprintf(Exposure_Error_String,"CCD_Exposure_Abort_Readout:PCI board PC reset command failed.");
			CCD_Exposure_Warning();
		}
#ifndef CCD_FLUSH_REPLY_BUFFER_PER_COMMAND
		if(!CCD_DSP_Command_Flush_Reply_Buffer())
		{
			Exposure_Error_Number = 31;
			sprintf(Exposure_Error_String,"CCD_Exposure_Abort:Flushing reply buffer failed.");
			CCD_Exposure_Warning();
		}
#endif
	}
	else
	{
		Exposure_Error_Number = 18;
		sprintf(Exposure_Error_String,"CCD_Exposure_Abort:Exposure Status was not readout");
		CCD_Exposure_Warning();
	}
	CCD_DSP_Set_Abort(TRUE);
}

/**
 * This routine is not called as part of the normal exposure sequence. It is used to read out a ccd exposure
 * under manual control or to read out an aborted exposure. Note it currently always sends the RDI command to
 * the controller. 
 * <dl>
 * <dd>It checks to ensure CCD Setup has been successfully completed using CCD_Setup_Get_Setup_Complete.</dd>
 * <dd>It gets the number of rows,columns and the deinterlace type from setup.</dd>
 * <dd>It executes a RDC command to read out the CCD, using 
 * 	<a href="ccd_dsp.html#CCD_DSP_Command_RDI">CCD_DSP_Command_RDI</a>.</dd>
 * </dl>
 * If the readout is aborted at any stage the routine returns.
 * @param filename The filename to save the exposure into.
 * @return Returns TRUE if the readout succeeds and the file is saved, returns FALSE if an error
 *	occurs or the readout is aborted.
 * @see ccd_setup.html#CCD_Setup_Get_Setup_Complete
 * @see ccd_dsp.html#CCD_DSP_Command_RDI
 */
int CCD_Exposure_Read_Out_CCD(char *filename)
/* a seperarate command to the main exposure sequence */
{
	int ncols;
	int nrows;
	enum CCD_DSP_DEINTERLACE_TYPE deinterlace_type;
	int ret_val;

	Exposure_Error_Number = 0;
/* reset abort flag */
	CCD_DSP_Set_Abort(FALSE);
/* we shouldn't be able to expose until setup has been successfully completed - check this */
	if(!CCD_Setup_Get_Setup_Complete())
	{
		Exposure_Error_Number = 19;
		sprintf(Exposure_Error_String,"CCD_Exposure_Read_Out_CCD:Readout failed:Setup was not complete.");
		return FALSE;
	}
	/* get information from setup that we need to read out */
	ncols = CCD_Setup_Get_NCols();
	nrows = CCD_Setup_Get_NRows();
	deinterlace_type = CCD_Setup_Get_DeInterlace_Type();
 	ret_val = CCD_DSP_Command_RDI(ncols,nrows,deinterlace_type,TRUE,filename);
	if(ret_val == FALSE)
	{
		CCD_DSP_Set_Abort(FALSE);
		Exposure_Error_Number = 20;
		sprintf(Exposure_Error_String,"CCD_Exposure_Read_Out_CCD:Readout failed.");
		return FALSE;
	}
	return TRUE;
}

/**
 * Get the current value of the ccd_exposure error number.
 * @return The current value of the ccd_exposure error number.
 */
int CCD_Exposure_Get_Error_Number(void)
{
	return Exposure_Error_Number;
}

/**
 * The error routine that reports any errors occuring in ccd_exposure in a standard way.
 * @see ccd_global.html#CCD_Global_Get_Current_Time_String
 */
void CCD_Exposure_Error(void)
{
	char time_string[32];

	CCD_Global_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Exposure_Error_Number == 0)
		sprintf(Exposure_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s CCD_Exposure:Error(%d) : %s\n",time_string,Exposure_Error_Number,Exposure_Error_String);
	Exposure_Error_Number = 0;
}

/**
 * The error routine that reports any errors occuring in ccd_exposure in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 * @see ccd_global.html#CCD_Global_Get_Current_Time_String
 */
void CCD_Exposure_Error_String(char *error_string)
{
	char time_string[32];

	CCD_Global_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Exposure_Error_Number == 0)
		sprintf(Exposure_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s CCD_Exposure:Error(%d) : %s\n",time_string,
		Exposure_Error_Number,Exposure_Error_String);
	Exposure_Error_Number = 0;
}

/**
 * The warning routine that reports any warnings occuring in ccd_exposure in a standard way.
 * @see ccd_global.html#CCD_Global_Get_Current_Time_String
 */
void CCD_Exposure_Warning(void)
{
	char time_string[32];

	CCD_Global_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an warning message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an warning to display */
	if(Exposure_Error_Number == 0)
		sprintf(Exposure_Error_String,"Logic Error:No Warning defined");
	fprintf(stderr,"%s CCD_Exposure:Warning(%d) : %s\n",time_string,Exposure_Error_Number,Exposure_Error_String);
	Exposure_Error_Number = 0;
}

/* ----------------------------------------------------------------
**	Internal routines
** ---------------------------------------------------------------- */
/**
 * Internal exposure routine that sets the shutter bit. The SDSU SEX command looks
 * at this bit to determine whether to open and close the shutter whilst performing an exposure.
 * @param open_shutter If it is TRUE, the SDSU SEX command will open/close the 
 * 	shutter at the relevant places during an exposure. If it is FALSE, the shutter will remain in it's 
 * 	current state throughout the exposure.
 * @return Returns TRUE if the operation succeeded, FALSE if it fails.
 * @see #CCD_Exposure_Expose
 * @see ccd_dsp.html#CCD_DSP_Command_RDM
 * @see ccd_dsp.html#CCD_DSP_Command_WRM
 * @see ccd_dsp.html#CCD_DSP_CONTROLLER_STATUS_OPEN_SHUTTER_BIT
 */
static int Exposure_Shutter_Control(int open_shutter)
{
	int current_status;
#if DEBUG == 1
	int debug;
#endif

/* check parameter */
	if(!CCD_GLOBAL_IS_BOOLEAN(open_shutter))
	{
		Exposure_Error_Number = 21;
		sprintf(Exposure_Error_String,"Exposure_Shutter_Control:Illegal open_shutter value:%d.",
			open_shutter);
		return FALSE;
	}
/* if debugging, get PCI/timing board status and print */
#if DEBUG == 1
	debug = CCD_DSP_Command_Read_PCI_Status();	
	fprintf(stdout,"PCI_STATUS = %#x\n",debug);
	fflush(stdout);
#endif
/* get current controller status */
	current_status = CCD_DSP_Command_RDM(CCD_DSP_TIM_BOARD_ID,CCD_DSP_MEM_SPACE_X,0);
	if((current_status == 0)&&(CCD_DSP_Get_Error_Number() != 0))
	{
		Exposure_Error_Number = 28;
		sprintf(Exposure_Error_String,"Exposure_Shutter_Control: Reading timing status failed.");
		return FALSE;
	}
/* set or clear open shutter bit */
	if(open_shutter)
		current_status |= CCD_DSP_CONTROLLER_STATUS_OPEN_SHUTTER_BIT;
	else
		current_status &= ~(CCD_DSP_CONTROLLER_STATUS_OPEN_SHUTTER_BIT);
/* write new controller status value */
	if(CCD_DSP_Command_WRM(CCD_DSP_TIM_BOARD_ID,CCD_DSP_MEM_SPACE_X,0,current_status) != CCD_DSP_DON)
	{
		Exposure_Error_Number = 22;
		sprintf(Exposure_Error_String,"Exposure_Shutter_Control: Writing shutter bit failed.");
		return FALSE;
	}
	return TRUE;
}

/*
** $Log: not supported by cvs2svn $
** Revision 0.13  2000/07/14 16:25:44  cjm
** Backup.
**
** Revision 0.12  2000/07/11 10:42:24  cjm
** Removed CCD_Exposure_Flush_CCD.
**
** Revision 0.11  2000/06/20 12:53:07  cjm
** CCD_DSP_Command_Sex now automatically calls CCD_DSP_Command_RDI.
**
** Revision 0.10  2000/06/19 08:48:34  cjm
** Backup.
**
** Revision 0.9  2000/06/13 17:14:13  cjm
** Changes to make Ccs agree with voodoo.
**
** Revision 0.8  2000/05/23 10:34:46  cjm
** Added call to CCD_DSP_Set_Exposure_Start_Time in CCD_Exposure_Bias,
** so that bias frames now have an exposure start time set,
** which gives a sensible value for DATE, DATE-OBS and UTSTART in FITS files.
**
** Revision 0.7  2000/05/10 14:37:52  cjm
** Removed number of bytes parameter from CCD_DSP_Command_RDI.
**
** Revision 0.6  2000/04/13 13:17:36  cjm
** Added current time to error routines.
**
** Revision 0.5  2000/03/13 12:30:17  cjm
** Removed duplicate CCD_DSP_Set_Abort(FALSE) in CCD_Exposure_Bias.
**
** Revision 0.4  2000/02/28 19:13:01  cjm
** Backup.
**
** Revision 0.3  2000/02/22 16:05:21  cjm
** Changed call structure to CCD_DSP_Set_Abort.
**
** Revision 0.2  2000/02/01 17:50:01  cjm
** Changed references to CCD_Setup_Setup_CCD to CCD_Setup_Get_Setup_Complete.
**
** Revision 0.1  2000/01/25 14:57:27  cjm
** initial revision (PCI version).
**
*/

/* ccd_exposure.c
** low level ccd library
** $Header: /home/cjm/cvs/frodospec/ccd/c/ccd_exposure.c,v 0.26 2003-12-08 15:04:00 cjm Exp $
*/
/**
 * ccd_exposure.c contains routines for performing an exposure with the SDSU CCD Controller. There is a
 * routine that does the whole job in one go, or several routines can be called to do parts of an exposure.
 * An exposure can be paused and resumed, or it can be stopped or aborted.
 * @author SDSU, Chris Mottram
 * @version $Revision: 0.26 $
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
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#ifndef _POSIX_TIMERS
#include <sys/time.h>
#endif
#include <time.h>
#include "ccd_exposure.h"
#include "ccd_dsp.h"
#include "ccd_setup.h"
#ifdef CFITSIO
#include "fitsio.h"
#endif
#ifdef SLALIB
#include "slalib.h"
#endif /* SLALIB */
#ifdef NGATASTRO
#include "ngat_astro.h"
#include "ngat_astro_mjd.h"
#endif /* NGATASTRO */

/* hash definitions */
/**
 * Memory address on the SDSU Timing Board, X memory space, which holds the controller status.
 * The CCD_DSP_CONTROLLER_STATUS_OPEN_SHUTTER_BIT holds whether to open the shutter when
 * a SEX command is issued to the controller.
 * @see ccd_dsp.html#CCD_DSP_CONTROLLER_STATUS_OPEN_SHUTTER_BIT
 */
#define EXPOSURE_ADDRESS_CONTROLLER_STATUS		(0x0)
/**
 * Bits used when getting the HSTR status.
 */
#define EXPOSURE_HSTR_HTF_BITS				(0x38)
/**
 * Number used to determine how long we keep getting the same number of readout pixels
 * returned before we timeout. This number depeends on the sleep in the loop.
 * This is by default 1 second, which makes this number the number of seconds
 * we don't read out more pixels before we time out.
 */
#define EXPOSURE_READ_TIMEOUT                           (0x5)
/**
 * The number of milliseconds before the controller stops exposing and starts reading out,
 * that we switch the exposure status from EXPOSING to READOUT. This is done early as we 
 * only check the HSTR status every second, and RDM/TDL/RET/WRM check the exposure status
 * to determine whether it is safe. It is not safe to call RDM/TDl/RET/WRM when
 * the HSTR is in readout mode, so we change exposure state early. Note the value
 * of this define should be greater than the sleep in the exposure loop.
 */
#define EXPOSURE_DEFAULT_READOUT_REMAINING_TIME       	(1500)
/**
 * The default amount of time before we are due to start an exposure, that a CLEAR_ARRAY command should be sent to
 * the controller. This time is in seconds, and must be greater than the time the CLEAR_ARRAY command takes to
 * clock all accumulated charge off the CCD (approx 5 seconds for a 2kx2k EEV42-40).
 */
#define EXPOSURE_DEFAULT_START_EXPOSURE_CLEAR_TIME	(10)
/**
 * The default amount of time, in milliseconds, before the desired start of exposure that we should send the
 * START_EXPOSURE command, to allow for transmission delay.
 */
#define EXPOSURE_DEFAULT_START_EXPOSURE_OFFSET_TIME	(2)

/* structure */
/**
 * Structure used to hold local data to ccd_exposure.
 * <dl>
 * <dt>Exposure_Status</dt> <dd>Whether an operation is being performed to CLEAR, EXPOSE or READOUT the CCD.</dd>
 * <dt>Start_Exposure_Clear_Time</dt> <dd>The amount of time before we are due to start an exposure, 
 * 	that a CLEAR_ARRAY command should be sent to the controller. This time is in seconds, 
 * 	and must be greater than the time the CLEAR_ARRAY command takes to clock all accumulated charge off the CCD 
 * 	(approx 5 seconds for a 2kx2k EEV42-40).</dd>
 * <dt>Start_Exposure_Offset_Time</dt> <dd>The amount of time, in milliseconds, before the desired start of 
 * 	exposure that we should send the START_EXPOSURE command, to allow for transmission delay.</dd>
 * <dt>Readout_Remaining_Time</dt> <dd>Amount of time, in milleseconds,
 * 	remaining for an exposure when we change status to READOUT, to stop RDM/TDL/WRMs affecting the readout.</dd>
 * <dt>Exposure_Length</dt> <dd>The last exposure length to be set.</dd>
 * <dt>Exposure_Start_Time</dt> <dd>The time stamp when the START_EXPOSURE command was sent to the controller.</dd>
 * </dl>
 * @see ccd_dsp.html#CCD_EXPOSURE_STATUS
 */
struct Exposure_Struct
{
	enum CCD_EXPOSURE_STATUS Exposure_Status;
	int Start_Exposure_Clear_Time;
	int Start_Exposure_Offset_Time;
	int Readout_Remaining_Time;
	int Exposure_Length;
	struct timespec Exposure_Start_Time;
};

/* external variables */

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id: ccd_exposure.c,v 0.26 2003-12-08 15:04:00 cjm Exp $";

/**
 * Variable holding error code of last operation performed by ccd_exposure.
 */
static int Exposure_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 */
static char Exposure_Error_String[CCD_GLOBAL_ERROR_STRING_LENGTH] = "";
/**
 * Data holding the current status of ccd_exposure. This is statically initialised to the following:
 * <dl>
 * <dt>Exposure_Status</dt> <dd>CCD_EXPOSURE_STATUS_NONE</dd>
 * <dt>Start_Exposure_Clear_Time</dt> <dd>EXPOSURE_DEFAULT_START_EXPOSURE_CLEAR_TIME</dd>
 * <dt>Start_Exposure_Offset_Time</dt> <dd>EXPOSURE_DEFAULT_START_EXPOSURE_OFFSET_TIME</dd>
 * <dt>Readout_Remaining_Time</dt> <dd>EXPOSURE_DEFAULT_READOUT_REMAINING_TIME</dd>
 * <dt>Exposure_Length</dt> <dd>0</dd>
 * <dt>Exposure_Start_Time</dt> <dd>{0L,0L}</dd>
 * </dl>
 * @see #Exposure_Struct
 * @see #CCD_EXPOSURE_STATUS
 * @see #EXPOSURE_DEFAULT_START_EXPOSURE_CLEAR_TIME
 * @see #EXPOSURE_DEFAULT_START_EXPOSURE_OFFSET_TIME
 * @see #EXPOSURE_DEFAULT_READOUT_REMAINING_TIME
 */
static struct Exposure_Struct Exposure_Data = 
{
	CCD_EXPOSURE_STATUS_NONE,
	EXPOSURE_DEFAULT_START_EXPOSURE_CLEAR_TIME,
	EXPOSURE_DEFAULT_START_EXPOSURE_OFFSET_TIME,
	EXPOSURE_DEFAULT_READOUT_REMAINING_TIME,
	0,
	{0L,0L},
};

/* internal functions */
static int Exposure_Shutter_Control(int value);
static int Exposure_Expose_Post_Readout_Full_Frame(unsigned short *exposure_data,char *filename);
static int Exposure_Expose_Post_Readout_Window(unsigned short *exposure_data,
					       char **filename_listfilename_list,int filename_count);
/* we should provide an alternative for these two routines if the library is not using short ints. */
#if CCD_GLOBAL_BYTES_PER_PIXEL == 2
static void Exposure_Byte_Swap(unsigned short *svalues,long nvals);
static int Exposure_DeInterlace(int ncols,int nrows,unsigned short *old_iptr,
	enum CCD_DSP_DEINTERLACE_TYPE deinterlace_type);
#else
#error CCD_GLOBAL_BYTES_PER_PIXEL uses illegal value.
#endif
static int Exposure_Save(char *filename,unsigned short *exposure_data,int ncols,int nrows);
static void Exposure_TimeSpec_To_Date_String(struct timespec time,char *time_string);
static void Exposure_TimeSpec_To_Date_Obs_String(struct timespec time,char *time_string);
static void Exposure_TimeSpec_To_UtStart_String(struct timespec time,char *time_string);
static int Exposure_TimeSpec_To_Mjd(struct timespec time,int leap_second_correction,double *mjd);

/* external functions */
/**
 * This routine sets up ccd_exposure internal variables.
 * It should be called at startup.
 * @see #Exposure_Data
 */
void CCD_Exposure_Initialise(void)
{
	Exposure_Error_Number = 0;
	Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_NONE;
	Exposure_Data.Start_Exposure_Clear_Time = EXPOSURE_DEFAULT_START_EXPOSURE_CLEAR_TIME;
	Exposure_Data.Start_Exposure_Offset_Time = EXPOSURE_DEFAULT_START_EXPOSURE_OFFSET_TIME;
	Exposure_Data.Readout_Remaining_Time = EXPOSURE_DEFAULT_READOUT_REMAINING_TIME;
	Exposure_Data.Exposure_Length = 0;
	Exposure_Data.Exposure_Start_Time.tv_sec = 0;
	Exposure_Data.Exposure_Start_Time.tv_nsec = 0;
/* print some compile time information to stdout */
	fprintf(stdout,"CCD_Exposure_Initialise:%s.\n",rcsid);
#ifdef CCD_EXPOSURE_BYTE_SWAP
	fprintf(stdout,"CCD_Exposure_Initialise:Image data is byte swapped by the application.\n");
#else
	fprintf(stdout,"CCD_Exposure_Initialise:Image data is byte swapped by the device driver.\n");
#endif
#ifdef CFITSIO
	fprintf(stdout,"CCD_Exposure_Initialise:Using CFITSIO.\n");
#else
	fprintf(stdout,"CCD_Exposure_Initialise:NOT Using CFITSIO.\n");
#endif
}

/**
 * Routine to perform an exposure.
 * <ul>
 * <li>It checks to ensure CCD Setup has been successfully completed using CCD_Setup_Get_Setup_Complete.
 * <li>The controller is told whether to open the shutter or not during the exposure, depending on the value
 * 	of the open_shutter parameter.
 * <li>The length of exposure is sent to the controller using CCD_DSP_Command_SET.
 * <li>A sleep is executed until it is nearly (Exposure_Data.Start_Exposure_Clear_Time) time to start the exposure.
 * <li>The array is cleared using CCD_DSP_Command_CLR.
 * <li>The exposure is started by calling CCD_DSP_Command_SEX.
 * <li>Enter a loop, until the readout is completed:
 * 	<ul>
 * 	<li>Get the Host Status Transfer Register value, using CCD_DSP_Command_Get_HSTR.
 * 	<li>If we are not reading out, and have more than Exposure_Data.Readout_Remaining_Time milliseconds 
 * 		left of exposure, use CCD_DSP_Command_RET to get the current elapsed exposure time.
 * 	<li>If the exposure length minus the current elapsed exposure time is less than 
 * 		Exposure_Data.Readout_Remaining_Time milliseconds, switch exposure status to READOUT.
 * 	<li>If we are in readout mode, use CCD_DSP_Command_Get_Readout_Progress to get how many pixels
 * 		we have read out.
 * 	<li>Check to see if we have finished reading out.
 * 	<li>Check to see whether we have been aborted.
 *	</ul>
 * <li>Get a pointer to the read out reply data, using CCD_Interface_Get_Reply_Data.
 * <li>If byte swapping is enabled, the data is byte swapped with Exposure_Byte_Swap.
 * <li>If we are reading out a full frame, call Exposure_Expose_Post_Readout_Full_Frame. Otherwise call
 *     Exposure_Expose_Post_Readout_Window.
 * </ul>
 * The Exposure_Data.Exposure_Status is changed to reflect the operation being performed on the CCD.
 * If the exposure is aborted at any stage the routine returns.
 * @param clear_array An integer representing a boolean. This should be set to TRUE if we wish to
 * 	manually clear the array before the exposure starts, FALSE if we do not. This is usually TRUE.
 * @param open_shutter TRUE if the shutter is to be opened over the duration of the exposure, FALSE if the
 * 	shutter should remain closed. The shutter may not want to be opened if a calibration image is
 * 	being taken.
 * @param start_time The time to start the exposure. If both the fields in the <i>struct timespec</i> are zero,
 * 	the exposure can be started at any convenient time.
 * @param exposure_time The length of time to open the shutter for in milliseconds. This must be greater than zero,
 * 	and less than the maximum exposure length CCD_DSP_EXPOSURE_MAX_LENGTH.
 * @param filename_list A list of filenames to save the exposure into. This is normally of length 1,unless 
 *        we are windowing, in which case there will be one filename for each window.
 * @param filename_count The number of filenames in the filename_list.
 * @return Returns TRUE if the exposure succeeds and the file is saved, returns FALSE if an error
 *	occurs or the exposure is aborted.
 * @see #EXPOSURE_HSTR_HTF_BITS
 * @see #CCD_EXPOSURE_HSTR_READOUT
 * @see #CCD_EXPOSURE_HSTR_BIT_SHIFT
 * @see #EXPOSURE_READ_TIMEOUT
 * @see #Exposure_Data
 * @see #Exposure_Shutter_Control
 * @see #Exposure_Byte_Swap
 * @see #Exposure_Expose_Post_Readout_Full_Frame
 * @see #Exposure_Expose_Post_Readout_Window
 * @see ccd_setup.html#CCD_Setup_Get_Setup_Complete
 * @see ccd_setup.html#CCD_Setup_Get_Window_Flags
 * @see ccd_setup.html#CCD_Setup_Get_Readout_Pixel_Count
 * @see ccd_setup.html#CCD_Setup_Get_DeInterlace_Type
 * @see ccd_dsp.html#CCD_DSP_Command_CLR
 * @see ccd_dsp.html#CCD_DSP_Command_SET
 * @see ccd_dsp.html#CCD_DSP_Command_SEX
 * @see ccd_dsp.html#CCD_DSP_Command_Get_HSTR
 * @see ccd_dsp.html#CCD_DSP_Command_RET
 * @see ccd_dsp.html#CCD_DSP_Command_Get_Readout_Progress
 * @see ccd_dsp.html#CCD_DSP_EXPOSURE_MAX_LENGTH
 * @see ccd_interface.html#CCD_Interface_Get_Reply_Data
 */
int CCD_Exposure_Expose(int clear_array,int open_shutter,struct timespec start_time,int exposure_time,
			char **filename_list,int filename_count)
{
	struct timespec sleep_time,current_time;
#ifndef _POSIX_TIMERS
	struct timeval gtod_current_time;
#endif
	unsigned short *exposure_data = NULL;
	int elapsed_exposure_time,done;
	int retval,status,window_flags;
	int expected_pixel_count,current_pixel_count,last_pixel_count,readout_timeout_count;

	Exposure_Error_Number = 0;
#if LOGGING > 0
	CCD_Global_Log_Format(CCD_GLOBAL_LOG_BIT_EXPOSURE,"CCD_Exposure_Expose(clear_array=%d,open_shutter=%d,"
			      "start_time_sec=%ld,exposure_time=%d,filename_count=%d) started.",
			      clear_array,open_shutter,start_time.tv_sec,exposure_time,filename_count);
#endif
/* reset abort flag */
	CCD_DSP_Set_Abort(FALSE);
/* we shouldn't be able to expose until setup has been successfully completed - check this */
	if(!CCD_Setup_Get_Setup_Complete())
	{
		Exposure_Error_Number = 1;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose:Exposure failed:Setup was not complete");
		return FALSE;
	}
/* check parameter ranges */
	if(!CCD_GLOBAL_IS_BOOLEAN(clear_array))
	{
		Exposure_Error_Number = 6;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose:Illegal value:clear_array = %d.",
			clear_array);
		return FALSE;
	}
	if(!CCD_GLOBAL_IS_BOOLEAN(open_shutter))
	{
		Exposure_Error_Number = 2;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose:Illegal value:open_shutter = %d.",
			open_shutter);
		return FALSE;
	}
	if((exposure_time < 0)||(exposure_time > CCD_DSP_EXPOSURE_MAX_LENGTH))
	{
		Exposure_Error_Number = 3;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose:Illegal value:exposure_time = %d",exposure_time);
		return FALSE;
	}
	if(filename_count < 0)
	{
		Exposure_Error_Number = 7;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose:Illegal value:filename_count = %d",filename_count);
		return FALSE;
	}
	window_flags = CCD_Setup_Get_Window_Flags();
	if((window_flags == 0)&&(filename_count > 1))
	{
		Exposure_Error_Number = 8;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose:Too many filenames for window_flags %d:"
			"filename_count = %d",window_flags,filename_count);
		return FALSE;
	}
/* get information from setup that we need to do an exposure */
	expected_pixel_count = CCD_Setup_Get_Readout_Pixel_Count();
	if(expected_pixel_count <= 0)
	{
		Exposure_Error_Number = 9;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose:Illegal expected pixel count '%d'.",
			expected_pixel_count);
		return FALSE;
	}
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
#if LOGGING > 4
	CCD_Global_Log_Format(CCD_GLOBAL_LOG_BIT_EXPOSURE,"CCD_Exposure_Expose():Setting shutter control(%d).",
			      open_shutter);
#endif
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
#if LOGGING > 4
	CCD_Global_Log_Format(CCD_GLOBAL_LOG_BIT_EXPOSURE,"CCD_Exposure_Expose():Setting exposure length(%d).",
			      exposure_time);
#endif
	if(!CCD_DSP_Command_SET(exposure_time))
	{
		CCD_DSP_Set_Abort(FALSE);
		Exposure_Error_Number = 23;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose:Setting exposure time failed.");
		return FALSE;
	}
	Exposure_Data.Exposure_Length = exposure_time;
	if(CCD_DSP_Get_Abort())
	{
		CCD_DSP_Set_Abort(FALSE);
		Exposure_Error_Number = 25;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose:Aborted");
		return FALSE;
	}
/* initialise variables */
/* We will use the start_time parameter to determine when to start the exposure IF 
** it's seconds are greater then zero */ 
/* do the clear array a few seconds before the exposure is due to start */
	if(start_time.tv_sec > 0)
	{
		Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_WAIT_START;
		done = FALSE;
		while(done == FALSE)
		{
#ifdef _POSIX_TIMERS
			clock_gettime(CLOCK_REALTIME,&current_time);
#else
			gettimeofday(&gtod_current_time,NULL);
			current_time.tv_sec = gtod_current_time.tv_sec;
			current_time.tv_nsec = gtod_current_time.tv_usec*CCD_GLOBAL_ONE_MICROSECOND_NS;
#endif
#if LOGGING > 4
			CCD_Global_Log_Format(CCD_GLOBAL_LOG_BIT_EXPOSURE,
				       "CCD_Exposure_Expose():Waiting for exposure start time (%ld,%ld).",
				       current_time.tv_sec,start_time.tv_sec);
#endif
		/* if we've time, sleep for a second */
			if((start_time.tv_sec - current_time.tv_sec) > Exposure_Data.Start_Exposure_Clear_Time)
			{
				sleep_time.tv_sec = 1;
				sleep_time.tv_nsec = 0;
				nanosleep(&sleep_time,NULL);
			}
			else
				done = TRUE;
		/* check - have we been aborted? */
			if(CCD_DSP_Get_Abort())
				done = TRUE;
		}/* end while */
	}
	if(CCD_DSP_Get_Abort())
	{
		Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_NONE;
		Exposure_Error_Number = 37;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose:Aborted.");
		return FALSE;
	}
/* clear the array */
	if(clear_array)
	{
#if LOGGING > 4
		CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,"CCD_Exposure_Expose():Clearing CCD array.");
#endif
		Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_CLEAR;
		if(!CCD_DSP_Command_CLR())
		{
			Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_NONE;
			Exposure_Error_Number = 38;
			sprintf(Exposure_Error_String,"CCD_Exposure_Expose:Clear Array failed.");
			return FALSE;
		}
	}
/* Send the command to start the exposure, and monitor for completion. */
#if LOGGING > 4
	CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,"CCD_Exposure_Expose():Starting Exposure.");
#endif
	/* Exposure status is set in CCD_DSP_Command_SEX, as this routine sleeps before starting
	** the exposure. */
	if(!CCD_DSP_Command_SEX(start_time,exposure_time))
	{
		Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_NONE;
		CCD_DSP_Set_Abort(FALSE);
		Exposure_Error_Number = 39;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose:SEX command failed(%ld,%ld,%d).",
			start_time.tv_sec,start_time.tv_nsec,exposure_time);
		return FALSE;
	}
/* wait while the exposure is taken and read out */
	done = FALSE;
        elapsed_exposure_time = 0;
	current_pixel_count = 0;
	last_pixel_count = 0;
	while(done == FALSE)
	{
#if LOGGING > 4
		CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,
			       "CCD_Exposure_Expose():Getting Host Status Transfer Register.");
#endif
		if(!CCD_DSP_Command_Get_HSTR(&status))
		{
			Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_NONE;
			CCD_DSP_Set_Abort(FALSE);
			Exposure_Error_Number = 40;
			sprintf(Exposure_Error_String,"CCD_Exposure_Expose:Getting HSTR failed.");
			return FALSE;
		}
#if LOGGING > 9
		CCD_Global_Log_Format(CCD_GLOBAL_LOG_BIT_EXPOSURE,"CCD_Exposure_Expose():HSTR is %#x.",status);
#endif
		status = (status & EXPOSURE_HSTR_HTF_BITS) >> CCD_EXPOSURE_HSTR_BIT_SHIFT;
#if LOGGING > 9
		CCD_Global_Log_Format(CCD_GLOBAL_LOG_BIT_EXPOSURE,"CCD_Exposure_Expose():HSTR reply bits %#x.",status);
#endif
		if(status != CCD_EXPOSURE_HSTR_READOUT)
		{
			/* are we about to start reading out? */
			if((exposure_time - elapsed_exposure_time) >= Exposure_Data.Readout_Remaining_Time)
			{
#if LOGGING > 4
				CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,
				       "CCD_Exposure_Expose():Getting Elapsed exposure time.");
#endif
				/* get elapsed time from controller */
				elapsed_exposure_time = CCD_DSP_Command_RET();
#if LOGGING > 9
				CCD_Global_Log_Format(CCD_GLOBAL_LOG_BIT_EXPOSURE,
				       "CCD_Exposure_Expose():Elapsed exposure time is %#x.",elapsed_exposure_time);
#endif
				if (elapsed_exposure_time < 0)
					elapsed_exposure_time = 0;
				if(elapsed_exposure_time == 0)
				{
					if(CCD_DSP_Get_Error_Number() != 0)
						CCD_DSP_Error();
				}
			}/* end if there is over Exposure_Data.Readout_Remaining_Time milliseconds of exposure left */
			if((Exposure_Data.Exposure_Status == CCD_EXPOSURE_STATUS_EXPOSE)&&
			   ((exposure_time - elapsed_exposure_time) < Exposure_Data.Readout_Remaining_Time))
			{
				/* Here we change the Exposure status to PRE_READOUT, when there
				** is less than Exposure_Data.Readout_Remaining_Time milliseconds of 
				** exposure time left. 
				** The exposure status is checked in WRM,RDM,TDL and RET commands, 
				** so we can't send these commands when in readout mode. 
				** We switch to exposure readout Exposure_Data.Readout_Remaining_Time milliseconds 
				** early as we sleep for a second at the bottom of the loop, and the HSTR status
				** may change before we check it again. */
				Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_PRE_READOUT;
#if LOGGING > 4
				CCD_Global_Log_Format(CCD_GLOBAL_LOG_BIT_EXPOSURE,
						      "CCD_Exposure_Expose():Exposure Status "
						      "changed to PRE_READOUT %d milliseconds before readout starts.",
						      (exposure_time - elapsed_exposure_time));
#endif
			}/* end if there  is less than Exposure_Data.Readout_Remaining_Time milliseconds 
			 ** of exposure time left */
		}/* end if HSTR status is not readout */
		/* Testing whether the status is CCD_EXPOSURE_HSTR_READOUT can fail to be detected, 
		** if it is in this state for less than one second (i.e. dual amplifier readout with binning 4)
		** We could try the following test to get round this:
		**    if(Exposure_Data.Exposure_Status == CCD_EXPOSURE_STATUS_PRE_READOUT and
		**       exposure_time - elapsed_exposure_time < 0)
		**         Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_READOUT
		** However this won't work as we go into PRE_READOUT (which stops updating elapsed_exposure_time)
		** 1500 ms before the exposure fails.
		** See below for solution.
		*/
		if(status == CCD_EXPOSURE_HSTR_READOUT)
		{
#if LOGGING > 4
			CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,
				       "CCD_Exposure_Expose():HSTR Status is READOUT.");
#endif
			/* is this the first time through the loop we have detected readout mode? */
			if(Exposure_Data.Exposure_Status != CCD_EXPOSURE_STATUS_READOUT)
			{
				Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_READOUT;
#if LOGGING > 4
				CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,
					       "CCD_Exposure_Expose():Exposure Status changed to READOUT(HSTR).");
#endif
			}
		}
		/* We want to get the readout progress after we have moved into exposure status readout.
		** We want to continue getting readout progress after the HSTR status has come out of readout mode,
		** to get the progress of the last few bytes read out whilst we were sleeping. 
		** We used to only get readout progress when:
		** (Exposure_Data.Exposure_Status == CCD_EXPOSURE_STATUS_READOUT), (i.e. during and after
		** we had detected HSTR register status to be CCD_EXPOSURE_HSTR_READOUT)
		** However we can miss detecting readout mode, if the whole readout takes less than 1 second.
		*/
#if LOGGING > 4
		CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,
			       "CCD_Exposure_Expose():Getting Readout Progress.");
#endif
		last_pixel_count = current_pixel_count;
		if(!CCD_DSP_Command_Get_Readout_Progress(&current_pixel_count))
		{
			Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_NONE;
			CCD_DSP_Set_Abort(FALSE);
			Exposure_Error_Number = 41;
			sprintf(Exposure_Error_String,"CCD_Exposure_Expose:Get Readout Progress failed.");
			return FALSE;
		}
#if LOGGING > 9
		CCD_Global_Log_Format(CCD_GLOBAL_LOG_BIT_EXPOSURE,
				      "CCD_Exposure_Expose():Readout progress is %#x.",current_pixel_count);
#endif
		/* If the current pixel count is greater than zero, we must be reading out, right? 
	        ** Correct, see IIA in START_EXPOSURE (timCCDmisc.asm), INITIALIZE_NUMBER_OF_PIXELS in pciboot.asm,
		** READ_NUMBER_OF_PIXELS_READ (0x8075) in pciboot.asm, and READ_PIXEL_COUNT/ASTROPCI_GET_PROGRESS in
		** astropci.c. */
		if(current_pixel_count > 0)
		{
			/* is this the first time through the loop we have detected readout mode? */
			if(Exposure_Data.Exposure_Status != CCD_EXPOSURE_STATUS_READOUT)
			{
				Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_READOUT;
#if LOGGING > 4
				CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,
				    "CCD_Exposure_Expose():Exposure Status changed to READOUT(current_pixel_count).");
#endif
			}
		}
		/* We can only have a readout timeout, if we are in readout mode. */
		if(Exposure_Data.Exposure_Status == CCD_EXPOSURE_STATUS_READOUT)
		{
			/* has the pixel count changed. If not, increment timeout */
			if(current_pixel_count == last_pixel_count)
				readout_timeout_count++;
			else
				readout_timeout_count = 0;
			/* have we timed out? If so, exit loop. */
			if(readout_timeout_count == EXPOSURE_READ_TIMEOUT)
			{
#if LOGGING > 9
				CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,
				       "CCD_Exposure_Expose():Readout timeout has occured.");
#endif
				done = TRUE;
			}
		} 
		/* check - have we been aborted? */
		if(CCD_DSP_Get_Abort())
			done = TRUE;
		/* check - all pixels read out? */
		if(current_pixel_count >= expected_pixel_count)
		{
#if LOGGING > 9
			CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,"CCD_Exposure_Expose():Readout completed.");
#endif
			done = TRUE;
		}
		/* sleep for a bit */
		sleep_time.tv_sec = 1;
		sleep_time.tv_nsec = 0;
		nanosleep(&sleep_time,NULL);
	}/* end while not done */
	/* did we abort? */
	if(CCD_DSP_Get_Abort())
	{
		Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_NONE;
		Exposure_Error_Number = 42;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose:Aborted.");
		return FALSE;
	}
	/* did the readout time out?*/
	if(readout_timeout_count == EXPOSURE_READ_TIMEOUT)
	{
		Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_NONE;
		Exposure_Error_Number = 43;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose:Readout timed out.");
		return FALSE;
	}
#if LOGGING > 4
	CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,"CCD_Exposure_Expose():Getting reply data.");
#endif
	/* get data */
	if(!CCD_Interface_Get_Reply_Data(&exposure_data))
	{
		Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_NONE;
		Exposure_Error_Number = 44;
		sprintf(Exposure_Error_String,"CCD_Exposure_Expose:Failed to get reply data.");
		return FALSE;
	}
	Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_POST_READOUT;
/* byte swap to get into right order */
#ifdef CCD_EXPOSURE_BYTE_SWAP
#if LOGGING > 4
	CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,"CCD_Exposure_Expose():Byte swapping.");
#endif
	Exposure_Byte_Swap(exposure_data,expected_pixel_count);
#endif
/* post-readout processing depends on whether we are windowing or not. */
	if(window_flags == 0)
	{
		if(Exposure_Expose_Post_Readout_Full_Frame(exposure_data,filename_list[0]) == FALSE)
		{
			Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_NONE;
			return FALSE;
		}
	}
	else
	{
		if(Exposure_Expose_Post_Readout_Window(exposure_data,filename_list,filename_count) == FALSE)
		{
			Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_NONE;
			return FALSE;
		}
	}
/* reset exposure status */
	Exposure_Data.Exposure_Status = CCD_EXPOSURE_STATUS_NONE;
#if LOGGING > 0
	CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,"CCD_Exposure_Expose() returned TRUE.");
#endif
	return TRUE;
}

/**
 * Routine to take a bias frame. Calls CCD_Exposure_Expose with clear_array TRUE, open_shutter FALSE and 
 * zero exposure length. Note assumes single readout filename, will not work if setup is windowed.
 * @param filename The filename to save the resultant data (in FITS format) to.
 * @return The routine returns TRUE if the operation was completed successfully, FALSE if it failed.
 */
int CCD_Exposure_Bias(char *filename)
{
	struct timespec start_time;
	char *filename_list[1];

	start_time.tv_sec = 0;
	start_time.tv_nsec = 0;
	filename_list[0] = filename;
	return CCD_Exposure_Expose(TRUE,FALSE,start_time,0,filename_list,1);
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
#if LOGGING > 0
	CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,"CCD_Exposure_Pause() started.");
#endif
/* reset abort flag */
	CCD_DSP_Set_Abort(FALSE);
	if(!CCD_DSP_Command_PEX())
	{
		CCD_DSP_Set_Abort(FALSE);
		Exposure_Error_Number = 13;
		sprintf(Exposure_Error_String,"CCD_Exposure_Pause:Pause command failed.");
		return FALSE;
	}
#if LOGGING > 0
	CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,"CCD_Exposure_Pause() returned TRUE.");
#endif
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
#if LOGGING > 0
	CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,"CCD_Exposure_Resume() started.");
#endif
/* reset abort flag */
	CCD_DSP_Set_Abort(FALSE);
	if(!CCD_DSP_Command_REX())
	{
		CCD_DSP_Set_Abort(FALSE);
		Exposure_Error_Number = 14;
		sprintf(Exposure_Error_String,"CCD_Exposure_Resume:Resume command failed.");
		return FALSE;
	}
#if LOGGING > 0
	CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,"CCD_Exposure_Resume() returned TRUE.");
#endif
	return TRUE;
}

/**
 * This routine aborts an exposure currenly underway, but not reading out. The current status of an exposure can
 * be found with CCD_Exposure_Get_Exposure_Status, and if it
 * CCD_EXPOSURE_STATUS_EXPOSE this routine should be called.
 * If the exposure status is CCD_EXPOSURE_STATUS_READOUT CCD_Exposure_Abort_Readout should be called instead. 
 * This routine executes a AEX command, and sets the Abort 
 * flag to true by calling CCD_DSP_Set_Abort(TRUE).
 * @return Returns TRUE if the abort succeeds  returns FALSE if an error occurs.
 * @see #Exposure_Data
 * @see #CCD_Exposure_Expose
 * @see #CCD_Exposure_Abort_Readout
 * @see #CCD_Exposure_Get_Exposure_Status
 * @see #CCD_Exposure_Abort_Readout
 * @see #CCD_DSP_Set_Abort
 * @see ccd_dsp.html#CCD_DSP_Set_Abort
 * @see ccd_dsp.html#CCD_EXPOSURE_STATUS
 */
int CCD_Exposure_Abort(void)
{
	Exposure_Error_Number = 0;
#if LOGGING > 0
	CCD_Global_Log_Format(CCD_GLOBAL_LOG_BIT_EXPOSURE,"CCD_Exposure_Abort() started with exposure status %d.",
		       Exposure_Data.Exposure_Status);
#endif
	if(CCD_DSP_Command_AEX() != CCD_DSP_DON)
	{
		Exposure_Error_Number = 15;
		sprintf(Exposure_Error_String,"CCD_Exposure_Abort:AEX command failed.");
		return FALSE;
	}
	/* diddly - should we be doing this here? */
	CCD_DSP_Set_Abort(TRUE);
#if LOGGING > 0
	CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,"CCD_Exposure_Abort() finished.");
#endif
	return TRUE;
}

/**
 * This routine aborts an exposure that is currently reading out. The current status of an exposure can
 * be found with Exposure_Data.Exposure_Status, and if it
 * CCD_EXPOSURE_STATUS_READOUT this routine should be called.
 * If the exposure status is CCD_EXPOSURE_STATUS_EXPOSE CCD_Exposure_Abort should be called instead. 
 * This routine calls the CCD_DSP_Command_ABR 
 * command routine to tell the SDSU CCD Controller to stop the readout,
 * and sets the Abort flag to true by calling CCD_DSP_Set_Abort(TRUE).
 * @return Returns TRUE if the abort succeeds  returns FALSE if an error occurs.
 * @see #Exposure_Data
 * @see #CCD_Exposure_Expose
 * @see #CCD_Exposure_Abort
 * @see ccd_dsp.html#CCD_DSP_Command_ABR 
 * @see ccd_dsp.html#CCD_DSP_Set_Abort
 * @see ccd_dsp.html#CCD_EXPOSURE_STATUS
 */
int CCD_Exposure_Abort_Readout(void)
{
	Exposure_Error_Number = 0;
#if LOGGING > 0
	CCD_Global_Log_Format(CCD_GLOBAL_LOG_BIT_EXPOSURE,
			      "CCD_Exposure_Abort_Readout() started with exposure status %d.",
			      Exposure_Data.Exposure_Status);
#endif
	if(CCD_DSP_Command_ABR() != CCD_DSP_DON)
	{
		Exposure_Error_Number = 17;
		sprintf(Exposure_Error_String,"CCD_Exposure_Abort_Readout:ABR command failed.");
		return FALSE;
	}
	/* diddly - should we be doing this here? */
	CCD_DSP_Set_Abort(TRUE);
#if LOGGING > 0
	CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,"CCD_Exposure_Abort_Readout() finished.");
#endif
	return TRUE;
}

/**
 * This routine is not called as part of the normal exposure sequence. It is used to read out a ccd exposure
 * under manual control or to read out an aborted exposure. 
 * If the readout is aborted at any stage the routine returns.
 * Note assumes single readout filename, will not work if setup is windowed.
 * This routine just calls CCD_Exposure_Expose, with clear_array FALSE (to prevent destruction of the image),
 * open_shutter FALSE, and an exposure length of zero.
 * @param filename The filename to save the exposure into.
 * @return Returns TRUE if the readout succeeds and the file is saved, returns FALSE if an error
 *	occurs or the readout is aborted.
 * @see #CCD_Exposure_Expose
 */
int CCD_Exposure_Read_Out_CCD(char *filename)
{
	struct timespec start_time;
	char *filename_list[1];

	start_time.tv_sec = 0;
	start_time.tv_nsec = 0;
	filename_list[0] = filename;
	return CCD_Exposure_Expose(FALSE,FALSE,start_time,0,filename_list,1);
}

/**
 * Routine to set the current value of the exposure status.
 * @param status The exposure status.
 * @see #Exposure_Data
 * @see #CCD_EXPOSURE_STATUS
 * @see #CCD_EXPOSURE_IS_STATUS
 */
int CCD_Exposure_Set_Exposure_Status(enum CCD_EXPOSURE_STATUS status)
{
	if(!CCD_EXPOSURE_IS_STATUS(status))
	{
		Exposure_Error_Number = 72;
		sprintf(Exposure_Error_String,"CCD_Exposure_Set_Exposure_Status:Status illegal value (%d).",status);
		return FALSE;
	}
	Exposure_Data.Exposure_Status = status;
	return TRUE;
}


/**
 * This routine gets the current value of Exposure Status.
 * Exposure_Status is defined in Exposure_Data.
 * @return The current status of exposure.
 * @see #CCD_EXPOSURE_STATUS
 * @see #Exposure_Data
 */
enum CCD_EXPOSURE_STATUS CCD_Exposure_Get_Exposure_Status(void)
{
	return Exposure_Data.Exposure_Status;
}

/**
 * This routine gets the current value of Exposure Length.
 * @return The last exposure length.
 * @see #Exposure_Data
 */
int CCD_Exposure_Get_Exposure_Length(void)
{
	return Exposure_Data.Exposure_Length;
}

/**
 * This routine gets the time stamp for the start of the exposure.
 * @return The time stamp for the start of the exposure.
 * @see #Exposure_Data
 */
struct timespec CCD_Exposure_Get_Exposure_Start_Time(void)
{
	return Exposure_Data.Exposure_Start_Time;
}

/**
 * Routine to set how many seconds before an exposure is due to start we wish to send the CLEAR_ARRAY
 * command to the controller.
 * @param time The time in seconds. This should be greater than the time the CLEAR_ARRAY command takes to
 * 	clock all accumulated charge off the CCD (approx 5 seconds for a 2kx2k EEV42-40).
 * @see #Exposure_Data
 */
void CCD_Exposure_Set_Start_Exposure_Clear_Time(int time)
{
	Exposure_Data.Start_Exposure_Clear_Time = time;
}

/**
 * Routine to get the current setting for how many seconds before an exposure is due to start we wish 
 * to send the CLEAR_ARRAY command to the controller.
 * @return The time, in seconds.
 * @see #Exposure_Data
 */
int CCD_Exposure_Get_Start_Exposure_Clear_Time(void)
{
	return Exposure_Data.Start_Exposure_Clear_Time;
}

/**
 * Routine to set the amount of time, in milliseconds, before the desired start of exposure that we should send the
 * START_EXPOSURE command, to allow for transmission delay.
 * @param time The time, in milliseconds.
 * @see #Exposure_Data
 */
void CCD_Exposure_Set_Start_Exposure_Offset_Time(int time)
{
	Exposure_Data.Start_Exposure_Offset_Time = time;
}

/**
 * Routine to get the amount of time, in milliseconds, before the desired start of exposure that we should send the
 * START_EXPOSURE command, to allow for transmission delay.
 * @return The time, in milliseconds.
 * @see #Exposure_Data
 */
int CCD_Exposure_Get_Start_Exposure_Offset_Time(void)
{
	return Exposure_Data.Start_Exposure_Offset_Time;
}

/**
 * Routine to set the amount of time, in milleseconds, remaining for an exposure when we change status to READOUT, 
 * to stop RDM/TDL/WRMs affecting the readout.
 * @param time The time, in milliseconds. Note, because the exposure time is read every second, it is best
 * 	not have have this constant an exact multiple of 1000.
 * @see #Exposure_Data
 */
void CCD_Exposure_Set_Readout_Remaining_Time(int time)
{
	Exposure_Data.Readout_Remaining_Time = time;
}

/**
 * Routine to get the amount of time, in milliseconds, remaining for an exposure when we change status to READOUT, 
 * to stop RDM/TDL/WRMs affecting the readout.
 * @return The time, in milliseconds.
 * @see #Exposure_Data
 */
int CCD_Exposure_Get_Readout_Remaining_Time(void)
{
	return Exposure_Data.Readout_Remaining_Time;
}

/**
 * Routine to set the Exposure_Start_Time of Exposure_Data, to the current time of the real time clock.
 * clock_gettime or gettimeofday is used, depending on whether _POSIX_TIMERS is defined.
 * @see #Exposure_Data
 */
void CCD_Exposure_Set_Exposure_Start_Time(void)
{
#ifndef _POSIX_TIMERS
	struct timeval gtod_current_time;
#endif

#ifdef _POSIX_TIMERS
	clock_gettime(CLOCK_REALTIME,&(Exposure_Data.Exposure_Start_Time));
#else
	gettimeofday(&gtod_current_time,NULL);
	Exposure_Data.Exposure_Start_Time.tv_sec = gtod_current_time.tv_sec;
	Exposure_Data.Exposure_Start_Time.tv_nsec = gtod_current_time.tv_usec*CCD_GLOBAL_ONE_MICROSECOND_NS;
#endif
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
 * @see #EXPOSURE_ADDRESS_CONTROLLER_STATUS
 * @see #CCD_Exposure_Expose
 * @see ccd_dsp.html#CCD_DSP_Command_RDM
 * @see ccd_dsp.html#CCD_DSP_Command_WRM
 * @see ccd_dsp.html#CCD_DSP_CONTROLLER_STATUS_OPEN_SHUTTER_BIT
 */
static int Exposure_Shutter_Control(int open_shutter)
{
	int current_status;

#if LOGGING > 4
	CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,"Exposure_Shutter_Control:Started.");
#endif
/* check parameter */
	if(!CCD_GLOBAL_IS_BOOLEAN(open_shutter))
	{
		Exposure_Error_Number = 21;
		sprintf(Exposure_Error_String,"Exposure_Shutter_Control:Illegal open_shutter value:%d.",
			open_shutter);
		return FALSE;
	}
/* get current controller status */
	current_status = CCD_DSP_Command_RDM(CCD_DSP_TIM_BOARD_ID,CCD_DSP_MEM_SPACE_X,
					     EXPOSURE_ADDRESS_CONTROLLER_STATUS);
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
	if(CCD_DSP_Command_WRM(CCD_DSP_TIM_BOARD_ID,CCD_DSP_MEM_SPACE_X,
			       EXPOSURE_ADDRESS_CONTROLLER_STATUS,current_status) != CCD_DSP_DON)
	{
		Exposure_Error_Number = 22;
		sprintf(Exposure_Error_String,"Exposure_Shutter_Control: Writing shutter bit failed.");
		return FALSE;
	}
#if LOGGING > 4
	CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,"Exposure_Shutter_Control:Completed.");
#endif
	return TRUE;
}

#if CCD_GLOBAL_BYTES_PER_PIXEL == 2
/**
 * Swap the bytes in the input unsigned short integers: ( 0 1 -> 1 0 ).
 * Based on CFITSIO's ffswap2 routine. This routine only works for CCD_GLOBAL_BYTES_PER_PIXEL == 2.
 * @param svalues A list of unsigned short values to byte swap.
 * @param nvals The number of values in svalues.
 */
static void Exposure_Byte_Swap(unsigned short *svalues,long nvals)
{
	register char *cvalues;
	register long i;

/* equivalence an array of 2 bytes with a short */
	union u_tag
	{
		char cvals[2];
		unsigned short sval;
	} u;
#if LOGGING > 4
	CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,"Exposure_Byte_Swap:Started.");
#endif
/* copy the initial pointer value */
	cvalues = (char *) svalues;

	for (i = 0; i < nvals;)
	{
	/* copy next short to temporary buffer */
		u.sval = svalues[i++];
	/* copy the 2 bytes to output in turn */
		*cvalues++ = u.cvals[1];
		*cvalues++ = u.cvals[0];
	}
#if LOGGING > 4
	CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,"Exposure_Byte_Swap:Completed.");
#endif
}
#else
#error Exposure_Byte_Swap not defined for this value of CCD_GLOBAL_BYTES_PER_PIXEL.
#endif

/**
 * Post-Readout operations on a full frame exposure,
 * <ul>
 * <li>The number of columns and rows are retrieved from setup.
 * <li>The data is de-interlaced using Exposure_DeInterlace.
 * <li>The data is saved to disc using Exposure_Save.
 * </ul>
 * @param exposure_data The data read out from the CCD.
 * @param filename The FITS filename (which should already contain relevant headers), in which to write 
 *        the image data.
 * @return The routine returns TRUE if it suceeded, and FALSE if it fails.
 * @see #Exposure_DeInterlace
 * @see #Exposure_Save
 * @see ccd_setup.html#CCD_Setup_Get_NCols
 * @see ccd_setup.html#CCD_Setup_Get_NRows
 */
static int Exposure_Expose_Post_Readout_Full_Frame(unsigned short *exposure_data,char *filename)
{
	enum CCD_DSP_DEINTERLACE_TYPE deinterlace_type;
	int ncols,nrows;

/* get setup details */
	ncols = CCD_Setup_Get_NCols();
	nrows = CCD_Setup_Get_NRows();
	deinterlace_type = CCD_Setup_Get_DeInterlace_Type();
/* number of columns must be a positive number */
	if(ncols <= 0)
	{
		Exposure_Error_Number = 27;
		sprintf(Exposure_Error_String,"Exposure_Expose_Post_Readout_Full_Frame:Illegal ncols '%d'.",ncols);
		return FALSE;
	}
/* number of rows must be a positive number */
	if(nrows <= 0)
	{
		Exposure_Error_Number = 31;
		sprintf(Exposure_Error_String,"Exposure_Expose_Post_Readout_Full_Frame:Illegal nrows '%d'.",nrows);
		return FALSE;
	}
	if(!CCD_DSP_IS_DEINTERLACE_TYPE(deinterlace_type))
	{
		Exposure_Error_Number = 36;
		sprintf(Exposure_Error_String,"Exposure_Expose_Post_Readout_Full_Frame:"
			"Illegal deinterlace type '%d'.",deinterlace_type);
		return FALSE;
	}
/* Do deinterlacing. The image returned from the boards may not be in the correct order
** if the CCD was readout from multiple places etc. The deinterlace routine reorders the image
** so that it is back in the right order. */
#if LOGGING > 4
	CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,"Exposure_Expose_Post_Readout_Full_Frame:De-Interlacing.");
#endif
	if(!Exposure_DeInterlace(ncols,nrows,exposure_data,deinterlace_type))
		return FALSE;
/* if we have aborted stop and return */
	if(CCD_DSP_Get_Abort())
	{
		Exposure_Error_Number = 45;
		sprintf(Exposure_Error_String,"Exposure_Expose_Post_Readout_Full_Frame:Aborted.");
		return FALSE;
	}
/* save the resultant image to disk */
#if LOGGING > 4
	CCD_Global_Log_Format(CCD_GLOBAL_LOG_BIT_EXPOSURE,"Exposure_Expose_Post_Readout_Full_Frame:"
			      "Saving to filename %s.",filename);
#endif
	if(!Exposure_Save(filename,exposure_data,ncols,nrows))
		return FALSE;
	return TRUE; 
}

/**
 * Post-Readout operations on a windowed exposure.
 * <ul>
 * <li>We get necessary setup data (window flags and deinterlace type).
 * <li>We go though the list of windows, looking for active windows.
 * <li>We retrieve setup data for active windows (width,height and pixel_count).
 * <li>We allocate space for a subimage array of the required size, and copy the relevant exposure data
 *     (applying the necessary exposure data index offset) into it.
 * <li>We call Exposure_DeInterlace to de-interlace the sub-image.
 * <li>We check whether we should be aborting.
 * <li>We save the sub-image to the relevant filename.
 * <li>We increment the exposure data index offset by the number of pixels in the sub-image.
 * <li>We increment the filename index.
 * <li>We free the sub-image data.
 * </ul>
 * @param exposure_data The data read out from the CCD.
 * @param filename_list The list of FITS filenames (which should already contain relevant headers), in which to write 
 *        the image data. Each window of data is saved in a separate file.
 * @return The routine returns TRUE if it suceeded, and FALSE if it fails.
 * @see #Exposure_DeInterlace
 * @see #Exposure_Save
 * @see ccd_setup.html#CCD_SETUP_WINDOW_COUNT
 * @see ccd_setup.html#CCD_Setup_Get_Window_Flags
 * @see ccd_setup.html#CCD_Setup_Get_DeInterlace_Type
 * @see ccd_setup.html#CCD_Setup_Get_Window_Width
 * @see ccd_setup.html#CCD_Setup_Get_Window_Height
 * @see ccd_setup.html#CCD_Setup_Get_Window_Pixel_Count
 * @see ccd_global.html#CCD_GLOBAL_BYTES_PER_PIXEL
 * @see ccd_dsp.html#CCD_DSP_IS_DEINTERLACE_TYPE
 * @see ccd_dsp.html#CCD_DSP_Get_Abort
 */
static int Exposure_Expose_Post_Readout_Window(unsigned short *exposure_data,
					       char **filename_list,int filename_count)
{
	enum CCD_DSP_DEINTERLACE_TYPE deinterlace_type;
	unsigned short *subimage_data = NULL;
	int exposure_data_index = 0;
	int window_number,window_flags,window_count,filename_index;
	int ncols,nrows,pixel_count;

	/* get setup data */
	window_flags = CCD_Setup_Get_Window_Flags();
	deinterlace_type = CCD_Setup_Get_DeInterlace_Type();
	if(!CCD_DSP_IS_DEINTERLACE_TYPE(deinterlace_type))
	{
		Exposure_Error_Number = 10;
		sprintf(Exposure_Error_String,"Exposure_Expose_Post_Readout_Window:"
			"Illegal deinterlace type '%d'.",deinterlace_type);
		return FALSE;
	}
	/* go through list of windows */
	exposure_data_index = 0;
	filename_index = 0;
	for(window_number = 0;window_number < CCD_SETUP_WINDOW_COUNT; window_number++)
	{
		/* look for windows that have been read out (are in use). */
		/* Note, relies on CCD_SETUP_WINDOW_ONE == (1<<0),
		** CCD_SETUP_WINDOW_TWO	== (1<<1),
		** CCD_SETUP_WINDOW_THREE == (1<<2) and
		** CCD_SETUP_WINDOW_FOUR == (1<<3) */
		if(window_flags&(1<<window_number))
		{
			ncols = CCD_Setup_Get_Window_Width(window_number);
			nrows = CCD_Setup_Get_Window_Height(window_number);
			pixel_count = CCD_Setup_Get_Window_Pixel_Count(window_number);
			if(filename_index >= filename_count)
			{
				Exposure_Error_Number = 16;
				sprintf(Exposure_Error_String,"Exposure_Expose_Post_Readout_Window:"
					"Filename index greater than count.",filename_index,filename_count);
				return FALSE;
			}
#if LOGGING > 4
			CCD_Global_Log_Format(CCD_GLOBAL_LOG_BIT_EXPOSURE,"Exposure_Expose_Post_Readout_Window:"
			      "Window %d(%s) active:ncols = %d,nrows = %d,pixel_count = %d.",
					      window_number,filename_list[filename_index],ncols,nrows,pixel_count);
#endif
			subimage_data = (unsigned short*)malloc(pixel_count*CCD_GLOBAL_BYTES_PER_PIXEL);
			if(subimage_data == NULL)
			{
				Exposure_Error_Number = 18;
				sprintf(Exposure_Error_String,"Exposure_Expose_Post_Readout_Window:"
					"SubImage Data was NULL (%d,%d).",window_number,pixel_count);
				return FALSE;
			}
			memcpy(subimage_data,exposure_data+exposure_data_index,pixel_count*CCD_GLOBAL_BYTES_PER_PIXEL);
#if LOGGING > 4
			CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,
				       "Exposure_Expose_Post_Readout_Window:De-Interlacing.");
#endif
			if(!Exposure_DeInterlace(ncols,nrows,subimage_data,deinterlace_type))
			{
				free(subimage_data);
				return FALSE;
			}
/* if we have aborted stop and return */
			if(CCD_DSP_Get_Abort())
			{
				free(subimage_data);
				Exposure_Error_Number = 19;
				sprintf(Exposure_Error_String,"Exposure_Expose_Post_Readout_Window:Aborted.");
				return FALSE;
			}
/* save the resultant image to disk */
#if LOGGING > 4
			CCD_Global_Log_Format(CCD_GLOBAL_LOG_BIT_EXPOSURE,"Exposure_Expose_Post_Readout_Window:"
			      "Saving to filename %s.",filename_list[filename_index]);
#endif
			if(!Exposure_Save(filename_list[filename_index],subimage_data,ncols,nrows))
			{
				free(subimage_data);
				return FALSE;
			}
			/* increment index into exposure data to start of next window. */
			exposure_data_index += pixel_count;
			/* increment index iff this window is active - only active window filenames in filename_list */
			filename_index++;
			/* free subimage */
			free(subimage_data);
		}
	}
	return TRUE;
}

#if CCD_GLOBAL_BYTES_PER_PIXEL == 2
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
 * <em>Note: This routine assumes CCD_GLOBAL_BYTES_PER_PIXEL == 2 e.g. 16 bits per pixel.</em>
 * @param ncols The number of columns on the CCD.
 * @param nrows The number of rows on the CCD.
 * @param old_iptr The interlaced image data received from the CCD. Once deinterlaced the image data is copied back
 * 	in this memory area.
 * @param deinterlace_type The type of deinterlacing to perform. One of CCD_DSP_DEINTERLACE_TYPE:
 * 	CCD_DSP_DEINTERLACE_SINGLE,
 * 	CCD_DSP_DEINTERLACE_SPLIT_PARALLEL,
 * 	CCD_DSP_DEINTERLACE_SPLIT_SERIAL or
 * 	CCD_DSP_DEINTERLACE_SPLIT_QUAD.
 * @return If everything was successful TRUE is returned, otherwise FALSE is returned.
 * @see ccd_global.html#CCD_GLOBAL_BYTES_PER_PIXEL
 * @see ccd_setup.html#CCD_DSP_DEINTERLACE_TYPE
 */
static int Exposure_DeInterlace(int ncols,int nrows,unsigned short *old_iptr,
	enum CCD_DSP_DEINTERLACE_TYPE deinterlace_type)
{
	unsigned short *new_iptr = NULL;

#if LOGGING > 4
	CCD_Global_Log_Format(CCD_GLOBAL_LOG_BIT_EXPOSURE,"Exposure_DeInterlace:Started with type %d.",
			      deinterlace_type);
#endif
	switch(deinterlace_type)
	{
		/* SINGLE READOUT 
		** The result is the same as the input. The CCD was readout from one port, so it
		** was readout in order */
		case CCD_DSP_DEINTERLACE_SINGLE:
		{
			return TRUE;
		} /*end single readout*/
		/* SPLIT PARALLEL READOUT */
		case CCD_DSP_DEINTERLACE_SPLIT_PARALLEL:
		{
			int i;

			if(((float)nrows/2) != (int)nrows/2)
			{
 				Exposure_Error_Number = 46;
				sprintf(Exposure_Error_String,"Exposure_DeInterlace:Split Parallel Readout,"
					"nrows not even(%d), image not deinterlaced.",nrows);
				return FALSE;
			}
		/* allocate enough memory to store the deinterlaced image */
			if((new_iptr = (unsigned short *)malloc(ncols*nrows*CCD_GLOBAL_BYTES_PER_PIXEL)) == NULL)
			{
		 		Exposure_Error_Number = 47;
				sprintf(Exposure_Error_String,"Exposure_DeInterlace:Memory Allocation Error(%d,%d),"
					"image not deinterlaced.",ncols,nrows);
				return FALSE;
			}
			for(i=0;i<(ncols*nrows)/2;i++)
			{
				*(new_iptr+i) = *(old_iptr+(2*i));
				*(new_iptr+(ncols*nrows)-i-1) = *(old_iptr+(2*i)+1);
			}
			memcpy(old_iptr,new_iptr,ncols*nrows*CCD_GLOBAL_BYTES_PER_PIXEL);
			free(new_iptr);
			return TRUE;
		} /*end split parallel*/
		/* SPLIT SERIAL READOUT */
		case CCD_DSP_DEINTERLACE_SPLIT_SERIAL:
		{
			int i,j,p1,p2,begin,end;

			if ((float)ncols/2 != (int)ncols/2)
        		{
 				Exposure_Error_Number = 48;
				sprintf(Exposure_Error_String,"Exposure_DeInterlace:Split Serial Readout,"
					"ncols not even(%d), image not deinterlaced.",ncols);
				return FALSE;
			}
		/* allocate enough memory to store the deinterlaced image */
			if((new_iptr = (unsigned short *)malloc(ncols*nrows*CCD_GLOBAL_BYTES_PER_PIXEL)) == NULL)
			{
		 		Exposure_Error_Number = 49;
				sprintf(Exposure_Error_String,"Exposure_DeInterlace:Memory Allocation Error(%d,%d),"
					"image not deinterlaced.",ncols,nrows);
				return FALSE;
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
			free(new_iptr);
			return TRUE;
		} /*end split serial*/
		/* SPLIT QUAD READOUT */
		case CCD_DSP_DEINTERLACE_SPLIT_QUAD:
		{
			int i=0,j=0,counter=0,end=0,begin=0;

			if((float)ncols/2 != (int)ncols/2 || (float)nrows/2 != (int)nrows/2)
			{
 				Exposure_Error_Number = 50;
				sprintf(Exposure_Error_String,"Exposure_DeInterlace:Split Quad Readout,"
					"ncols or nrows not even(%d,%d), image not deinterlaced.",ncols,nrows);
				return FALSE;
			}
		/* allocate enough memory to store the deinterlaced image */
			if((new_iptr = (unsigned short *)malloc(ncols*nrows*CCD_GLOBAL_BYTES_PER_PIXEL)) == NULL)
			{
		 		Exposure_Error_Number = 51;
				sprintf(Exposure_Error_String,"Exposure_DeInterlace:Memory Allocation Error(%d,%d),"
					"image not deinterlaced.",ncols,nrows);
				return FALSE;
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
			free(new_iptr);
			return TRUE;
		} /*end split quad readout*/
	}/*end switch*/
	Exposure_Error_Number = 52;
	sprintf(Exposure_Error_String,"Exposure_DeInterlace:Wrong DeInterlace option(%d),Image not deinterlaced.",
		deinterlace_type);
	return FALSE;
} /* end deinterlacing */
#else
#error Exposure_DeInterlace not defined for this value of CCD_GLOBAL_BYTES_PER_PIXEL.
#endif

/* 
** Exposure_Save uses a different implementation depending on whether CFITSIO define was defined at compile time.
** If it was we use CFITSIO routines, otherwise we don't.
*/

#ifdef CFITSIO
/**
 * This routine takes some image data and saves it in a file on disc. It also updates the 
 * DATE-OBS FITS keyword to the value saved just before the SEX command was sent to the controller.
 * @param filename The filename to save the data into.
 * @param exposure_data The data to save.
 * @param ncols The number of columns in the image data.
 * @param nrows The number of rows in the image data.
 * @return Returns TRUE if the image is saved successfully, FALSE if it fails.
 * @see #Exposure_TimeSpec_To_Date_String
 * @see #Exposure_TimeSpec_To_Date_Obs_String
 * @see #Exposure_TimeSpec_To_UtStart_String
 * @see #Exposure_TimeSpec_To_Mjd
 */
static int Exposure_Save(char *filename,unsigned short *exposure_data,int ncols,int nrows)
{
	fitsfile *fp = NULL;
	int retval=0,status=0;
	char buff[32]; /* fits_get_errstatus returns 30 chars max */
	char exposure_start_time_string[64];
	double mjd;

#if LOGGING > 4
	CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,"Exposure_Save:Started.");
#endif
	/* try to open file */
	retval = fits_open_file(&fp,filename,READWRITE,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		Exposure_Error_Number = 53;
		sprintf(Exposure_Error_String,"Exposure_Save: File open failed(%s,%d,%s).",filename,status,buff);
		return FALSE;
	}
	/* write the data */
	retval = fits_write_img(fp,TUSHORT,1,ncols*nrows,exposure_data,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fp,&status);
		Exposure_Error_Number = 54;
		sprintf(Exposure_Error_String,"Exposure_Save: File write failed(%s,%d,%s).",filename,status,buff);
		return FALSE;
	}
/* update DATE keyword */
	Exposure_TimeSpec_To_Date_String(Exposure_Data.Exposure_Start_Time,exposure_start_time_string);
	retval = fits_update_key(fp,TSTRING,"DATE",exposure_start_time_string,NULL,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fp,&status);
		Exposure_Error_Number = 55;
		sprintf(Exposure_Error_String,"Exposure_Save: Updating DATE failed(%s,%d,%s).",filename,status,buff);
		return FALSE;
	}
/* update DATE-OBS keyword */
	Exposure_TimeSpec_To_Date_Obs_String(Exposure_Data.Exposure_Start_Time,exposure_start_time_string);
	retval = fits_update_key(fp,TSTRING,"DATE-OBS",exposure_start_time_string,NULL,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fp,&status);
		Exposure_Error_Number = 56;
		sprintf(Exposure_Error_String,"Exposure_Save: Updating DATE-OBS failed(%s,%d,%s).",filename,
			status,buff);
		return FALSE;
	}
/* update UTSTART keyword */
	Exposure_TimeSpec_To_UtStart_String(Exposure_Data.Exposure_Start_Time,exposure_start_time_string);
	retval = fits_update_key(fp,TSTRING,"UTSTART",exposure_start_time_string,NULL,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fp,&status);
		Exposure_Error_Number = 57;
		sprintf(Exposure_Error_String,"Exposure_Save: Updating UTSTART failed(%s,%d,%s).",filename,
			status,buff);
		return FALSE;
	}
/* update MJD keyword */
/* note leap second correction not implemented yet (always FALSE). */
	if(!Exposure_TimeSpec_To_Mjd(Exposure_Data.Exposure_Start_Time,FALSE,&mjd))
		return FALSE;
	retval = fits_update_key_fixdbl(fp,"MJD",mjd,6,NULL,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fp,&status);
		Exposure_Error_Number = 58;
		sprintf(Exposure_Error_String,"Exposure_Save: Updating MJD failed(%.2f,%s,%d,%s).",mjd,filename,
			status,buff);
		return FALSE;
	}
/* close file */
	retval = fits_close_file(fp,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		Exposure_Error_Number = 59;
		sprintf(Exposure_Error_String,"Exposure_Save: File close failed(%s,%d,%s).",filename,status,buff);
		return FALSE;
	}
#if LOGGING > 4
	CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,"Exposure_Save:Completed.");
#endif
	return TRUE;
}
#else
/**
 * This routine takes some image data and saves it in a file on disc.
 * This routine does not update the DATE-OBS keyword, unlike the CFITSIO routine.
 * @param filename The filename to save the data into.
 * @param exposure_data The data to save.
 * @param ncols The number of columns in the image data.
 * @param nrows The number of rows in the image data.
 * @return Returns TRUE if the image is saved successfully, FALSE if it fails.
 */
static int Exposure_Save(char *filename,unsigned short *exposure_data,int ncols,int nrows)
{
	FILE *fp = NULL;
	int retval,error_number,nitems;

#if LOGGING > 4
	CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,"Exposure_Save:Started.");
#endif
	/* try to open file */
	fp = fopen(filename,"rb+");
	if(fp == NULL)
	{
		error_number = errno;
		Exposure_Error_Number = 60;
		sprintf(Exposure_Error_String,"Exposure_Save: File open failed(%s,%d).",filename,error_number);
		return FALSE;
	}
	/* move to end of file */
	retval = fseek(fp,0,SEEK_END);
	if(retval == -1)
	{
		fclose(fp);
		Exposure_Error_Number = 61;
		sprintf(Exposure_Error_String,"Exposure_Save: File seek failed(%s,%d,%s).",filename,errno,
			strerror(errno));
		return FALSE;
	}
	/* write the data */
	nitems = nrows*ncols;
	retval = fwrite(exposure_data,CCD_GLOBAL_BYTES_PER_PIXEL,nitems,fp);
	if(retval != nitems)
	{
		fclose(fp);
		Exposure_Error_Number = 62;
		sprintf(Exposure_Error_String,"Exposure_Save: File write failed(%s,%d,%d).",filename,retval,nitems);
		return FALSE;
	}
	fclose(fp);
#if LOGGING > 4
	CCD_Global_Log(CCD_GLOBAL_LOG_BIT_EXPOSURE,"Exposure_Save:Completed.");
#endif
	return TRUE;
}
#endif

/**
 * Routine to convert a timespec structure to a DATE sytle string to put into a FITS header.
 * This uses gmtime and strftime to format the string. The resultant string is of the form:
 * <b>CCYY-MM-DD</b>, which is equivalent to %Y-%m-%d passed to strftime.
 * @param time The time to convert.
 * @param time_string The string to put the time representation in. The string must be at least
 * 	12 characters long.
 */
static void Exposure_TimeSpec_To_Date_String(struct timespec time,char *time_string)
{
	struct tm *tm_time = NULL;

	tm_time = gmtime(&(time.tv_sec));
	strftime(time_string,12,"%Y-%m-%d",tm_time);
}

/**
 * Routine to convert a timespec structure to a DATE-OBS sytle string to put into a FITS header.
 * This uses gmtime and strftime to format most of the string, and tags the milliseconds on the end.
 * The resultant form of the string is <b>CCYY-MM-DDTHH:MM:SS.sss</b>.
 * @param time The time to convert.
 * @param time_string The string to put the time representation in. The string must be at least
 * 	24 characters long.
 * @see ccd_global.html#CCD_GLOBAL_ONE_MILLISECOND_NS
 */
static void Exposure_TimeSpec_To_Date_Obs_String(struct timespec time,char *time_string)
{
	struct tm *tm_time = NULL;
	char buff[32];
	int milliseconds;

	tm_time = gmtime(&(time.tv_sec));
	strftime(buff,32,"%Y-%m-%dT%H:%M:%S.",tm_time);
	milliseconds = (((double)time.tv_nsec)/((double)CCD_GLOBAL_ONE_MILLISECOND_NS));
	sprintf(time_string,"%s%03d",buff,milliseconds);
}

/**
 * Routine to convert a timespec structure to a UTSTART sytle string to put into a FITS header.
 * This uses gmtime and strftime to format most of the string, and tags the milliseconds on the end.
 * @param time The time to convert.
 * @param time_string The string to put the time representation in. The string must be at least
 * 	14 characters long.
 * @see ccd_global.html#CCD_GLOBAL_ONE_MILLISECOND_NS
 */
static void Exposure_TimeSpec_To_UtStart_String(struct timespec time,char *time_string)
{
	struct tm *tm_time = NULL;
	char buff[16];
	int milliseconds;

	tm_time = gmtime(&(time.tv_sec));
	strftime(buff,16,"%H:%M:%S.",tm_time);
	milliseconds = (((double)time.tv_nsec)/((double)CCD_GLOBAL_ONE_MILLISECOND_NS));
	sprintf(time_string,"%s%03d",buff,milliseconds);
}

/**
 * Routine to convert a timespec structure to a Modified Julian Date (decimal days) to put into a FITS header.
 * <p>If SLALIB is defined, this uses slaCldj to get the MJD for zero hours, 
 * and then adds hours/minutes/seconds/milliseconds on the end as a decimal.
 * <p>If NGATASTRO is defined, this uses NGAT_Astro_Timespec_To_MJD to get the MJD.
 * <p>If neither SLALIB or NGATASTRO are defined at compile time, this routine should throw an error
 * when compiling.
 * <p>This routine is still wrong for last second of the leap day, as gmtime will return 1st second of the next day.
 * Also note the passed in leap_second_correction should change at midnight, when the leap second occurs.
 * None of this should really matter, 1 second will not affect the MJD for several decimal places.
 * @param time The time to convert.
 * @param leap_second_correction A number representing whether a leap second will occur. This is normally zero,
 * 	which means no leap second will occur. It can be 1, which means the last minute of the day has 61 seconds,
 *	i.e. there are 86401 seconds in the day. It can be -1,which means the last minute of the day has 59 seconds,
 *	i.e. there are 86399 seconds in the day.
 * @param mjd The address of a double to store the calculated MJD.
 * @return The routine returns TRUE if it succeeded, FALSE if it fails. 
 *         slaCldj and NGAT_Astro_Timespec_To_MJD can fail.
 */
static int Exposure_TimeSpec_To_Mjd(struct timespec time,int leap_second_correction,double *mjd)
{
#ifdef SLALIB
	struct tm *tm_time = NULL;
	int year,month,day;
	double seconds_in_day = 86400.0;
	double elapsed_seconds;
	double day_fraction;
#endif
	int retval;

#ifdef SLALIB
/* check leap_second_correction in range */
/* convert time to ymdhms*/
	tm_time = gmtime(&(time.tv_sec));
/* convert tm_time data to format suitable for slaCldj */
	year = tm_time->tm_year+1900; /* tm_year is years since 1900 : slaCldj wants full year.*/
	month = tm_time->tm_mon+1;/* tm_mon is 0..11 : slaCldj wants 1..12 */
	day = tm_time->tm_mday;
/* call slaCldj to get MJD for 0hr */
	slaCldj(year,month,day,mjd,&retval);
	if(retval != 0)
	{
		Exposure_Error_Number = 63;
		sprintf(Exposure_Error_String,"Exposure_TimeSpec_To_Mjd:slaCldj(%d,%d,%d) failed(%d).",year,month,
			day,retval);
		return FALSE;
	}
/* how many seconds were in the day */
	seconds_in_day = 86400.0;
	seconds_in_day += (double)leap_second_correction;
/* calculate the number of elapsed seconds in the day */
	elapsed_seconds = (double)tm_time->tm_sec + (((double)time.tv_nsec) / 1.0E+09);
	elapsed_seconds += ((double)tm_time->tm_min) * 60.0;
	elapsed_seconds += ((double)tm_time->tm_hour) * 3600.0;
/* calculate day fraction */
	day_fraction = elapsed_seconds / seconds_in_day;
/* add day_fraction to mjd */
	(*mjd) += day_fraction;
#else
#ifdef NGATASTRO
	retval = NGAT_Astro_Timespec_To_MJD(time,leap_second_correction,mjd);
	if(retval == FALSE)
	{
		Exposure_Error_Number = 64;
		sprintf(Exposure_Error_String,"Exposure_TimeSpec_To_Mjd:NGAT_Astro_Timespec_To_MJD failed.\n");
		/* concatenate NGAT Astro library error onto Exposure_Error_String */
		NGAT_Astro_Error_String(Exposure_Error_String+strlen(Exposure_Error_String));
		return FALSE;
	}
#else
#error Neither NGATASTRO or SLALIB are defined: No library defined for MJD calculation.
#endif
#endif
	return TRUE;
}

/*
** $Log: not supported by cvs2svn $
** Revision 0.25  2003/11/04 14:42:00  cjm
** Minor MJD fixes based on errors in Linux versions of the code.
**
** Revision 0.24  2003/03/26 15:44:48  cjm
** Added windowing code.
**
** Revision 0.23  2003/03/04 17:09:53  cjm
** Added NGAT_Astro call.
**
** Revision 0.22  2002/12/16 16:49:36  cjm
** Fixed problems with status during an exposure, so that it only goes into
** PRE_READOUT at the correct time.
** Removed Error routines resetting error number to zero.
**
** Revision 0.21  2002/11/08 12:13:19  cjm
** CCD_DSP_Command_SEX now changes the exposure status to READOUT immediately,
** if the exposure length is small enough.
**
** Revision 0.20  2002/11/07 19:13:39  cjm
** Changes to make library work with SDSU version 1.7 DSP code.
**
** Revision 0.19  2001/06/04 14:38:00  cjm
** Changed DEBUG to LOGGING.
** Added readout process priority changes and memory locking code.
**
** Revision 0.18  2001/02/16 09:55:18  cjm
** Added more detail to error messages.
**
** Revision 0.17  2001/02/09 18:30:40  cjm
** comment spelling.
**
** Revision 0.16  2001/02/05 14:30:09  cjm
** Added checks to CCD_Exposure_Bias to STP/IDL called
** only when Idling was configured at startup.
**
** Revision 0.15  2001/01/23 18:21:27  cjm
** Added check for maximum exposure length CCD_DSP_EXPOSURE_MAX_LENGTH.
**
** Revision 0.14  2000/09/25 09:51:28  cjm
** Changes to use with v1.4 SDSU DSP code.
**
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

/* ccd_dsp.c
** ccd library
** $Header: /home/cjm/cvs/frodospec/ccd/c/ccd_dsp.c,v 0.56 2011-01-17 10:57:54 cjm Exp $
*/
/**
 * ccd_dsp.c contains all the SDSU CCD Controller commands. Commands are passed to the 
 * controller using the <a href="ccd_interface.html">CCD_Interface_</a> calls.
 * @author SDSU, Chris Mottram
 * @version $Revision: 0.56 $
 */
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes
 * for clock_gettime.
 */
#define _POSIX_SOURCE 1
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes
 * for clock_gettime.
 */
#define _POSIX_C_SOURCE 199309L

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#ifndef _POSIX_TIMERS
#include <sys/time.h>
#endif
#ifdef CCD_DSP_MUTEXED
#include <pthread.h>
#endif
#include "log_udp.h"
#include "ccd_global.h"
#include "ccd_interface.h"
#include "ccd_interface_private.h"
#include "ccd_pci.h"
#include "ccd_dsp.h"
#include "ccd_dsp_private.h"
#include "ccd_exposure.h"

/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id: ccd_dsp.c,v 0.56 2011-01-17 10:57:54 cjm Exp $";

/* defines */
/**
 * Special value to pass into DSP_Check_Reply as the expected reply parameter, indicating
 * that the reply from the command should be an actual value rather then (usually) CCD_DSP_DON.
 * @see #DSP_Check_Reply
 * @see #CCD_DSP_DON
 */
#define	DSP_ACTUAL_VALUE 		-1 /* flag indicating return value of DSP command is to be returned as data */

/* structure */
/**
 * Structure used to hold local data to ccd_dsp.
 * <dl>
 * <dt>Mutex</dt> <dd>Optionally compiled mutex locking for sending commands and getting replies from the 
 *    controller.</dd>
 * </dl>
 */
struct DSP_Struct
{
#ifdef CCD_DSP_MUTEXED
      pthread_mutex_t Mutex;
#endif
};

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
 * Data holding the current status of ccd_dsp. This is statically initialised to the following:
 * <dl>
 * <dt>Mutex</dt> <dd>If compiled in, PTHREAD_MUTEX_INITIALIZER</dd>
 * </dl>
 * @see #DSP_Struct
 */
static struct DSP_Struct DSP_Data = 
{
#ifdef CCD_DSP_MUTEXED
      PTHREAD_MUTEX_INITIALIZER
#endif
};

/* internal functions */
static int DSP_Send_Lda(char *class,char *source,CCD_Interface_Handle_T* handle,
			enum CCD_DSP_BOARD_ID board_id,int data,int *reply_value);
static int DSP_Send_Wrm(char *class,char *source,CCD_Interface_Handle_T* handle,
			enum CCD_DSP_BOARD_ID board_id,enum CCD_DSP_MEM_SPACE mem_space,int address,int data,
	int *reply_value);
static int DSP_Send_Rdm(char *class,char *source,CCD_Interface_Handle_T* handle,
			enum CCD_DSP_BOARD_ID board_id,enum CCD_DSP_MEM_SPACE mem_space,int address,int *reply_value);
static int DSP_Send_Tdl(char *class,char *source,CCD_Interface_Handle_T* handle,
			enum CCD_DSP_BOARD_ID board_id,int data,int *reply_value);

static int DSP_Send_Abr(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value);
static int DSP_Send_Clr(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value);
static int DSP_Send_Rdc(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value);
static int DSP_Send_Idl(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value);
static int DSP_Send_Sbv(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value);
static int DSP_Send_Sgn(char *class,char *source,CCD_Interface_Handle_T* handle,
			enum CCD_DSP_GAIN gain,int speed,int *reply_value);
static int DSP_Send_Sos(char *class,char *source,CCD_Interface_Handle_T* handle,
			enum CCD_DSP_AMPLIFIER amplifier,int *reply_value);
static int DSP_Send_Ssp(char *class,char *source,CCD_Interface_Handle_T* handle,
			int y_offset,int x_offset,int bias_x_offset,int *reply_value);
static int DSP_Send_Sss(char *class,char *source,CCD_Interface_Handle_T* handle,
			int bias_width,int box_width,int box_height,int *reply_value);
static int DSP_Send_Stp(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value);

static int DSP_Send_Aex(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value);
static int DSP_Send_Csh(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value);
static int DSP_Send_Osh(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value);
static int DSP_Send_Pex(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value);
static int DSP_Send_Pon(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value);
static int DSP_Send_Pof(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value);
static int DSP_Send_Rex(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value);
static int DSP_Send_Sex(char *class,char *source,CCD_Interface_Handle_T* handle,
			struct timespec start_time,int exposure_length, int *reply_value);
static int DSP_Send_Reset(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value);
static int DSP_Send_Rcc(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value);
static int DSP_Send_PCI_Download(char *class,char *source,CCD_Interface_Handle_T* handle);
static int DSP_Send_PCI_Download_Wait(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value);
static int DSP_Send_PCI_PC_Reset(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value);
static int DSP_Send_Set(char *class,char *source,CCD_Interface_Handle_T* handle,int msecs,int *reply_value);
static int DSP_Send_Ret(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value);

static int DSP_Send_Manual_Command(char *class,char *source,CCD_Interface_Handle_T* handle,
				   enum CCD_DSP_BOARD_ID board_id,int command,
				   int *argument_list,int argument_count,int *reply_value);
static int DSP_Send_Command(char *class,char *source,CCD_Interface_Handle_T* handle,
			    int hcvr_command,int *reply_value);
static int DSP_Check_Reply(char *class,char *source,int reply,int expected_reply);

#ifdef CCD_DSP_MUTEXED
static int DSP_Mutex_Lock(CCD_Interface_Handle_T* handle);
static int DSP_Mutex_Unlock(CCD_Interface_Handle_T* handle);
#endif
static char *DSP_Manual_Command_To_String(int manual_command);

/* external functions */

/**
 * This routine sets up ccd_dsp internal variables.
 * It should be called at startup.
 * The mutex is <b>NOT</b> initialised, this is statically initialised only. This allows us to call
 * this routine more than once without having to destroy the mutex in between.
 * @return Return TRUE if initialisation is successful, FALSE if it wasn't.
 * @see #DSP_DEFAULT_START_EXPOSURE_CLEAR_TIME
 * @see #DSP_DEFAULT_START_EXPOSURE_OFFSET_TIME
 * @see #DSP_DEFAULT_READOUT_REMAINING_TIME
 */
int CCD_DSP_Initialise(void)
{
	DSP_Error_Number = 0;
/* don't intialise the mutex here, it is statically initialised, once only */
/* print some compile time information to stdout */
	fprintf(stdout,"CCD_DSP_Initialise:%s.\n",rcsid);
#ifdef CCD_DSP_MUTEXED
	fprintf(stdout,"CCD_DSP_Initialise:SDSU controller commands are mutexed.\n");
#else
	fprintf(stdout,"CCD_DSP_Initialise:SDSU controller commands are NOT mutexed.\n");
#endif
#ifdef _POSIX_TIMERS
	fprintf(stdout,"CCD_DSP_Initialise:Using Posix Timers (clock_gettime).\n");
#else
	fprintf(stdout,"CCD_DSP_Initialise:Using Unix Timers (gettimeofday).\n");
#endif
#ifdef CCD_DSP_UTIL_EXPOSURE_CHECK
	fprintf(stdout,"CCD_DSP_Initialise:Reject util board communications during exposure.\n");
#else
	fprintf(stdout,"CCD_DSP_Initialise:Allow util board communications during exposure.\n");
#endif
	fflush(stdout);
	return TRUE;
}

/**
 * Routine to initialise the dsp data in the interface handle. The data is initialsied as follows:
 * <dl>
 * <dt>Abort</dt> <dd>FALSE</dd>
 * </dl>
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @see ccd_dsp_private.html#CCD_DSP_Struct
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
void CCD_DSP_Data_Initialise(CCD_Interface_Handle_T* handle)
{
	handle->DSP_Data.Abort = FALSE;
}

/* Boot commands */
/**
 * This routine executes the LoaD Application (LDA) command on a 
 * SDSU Controller board. This
 * causes some DSP application code to be loaded from (EEP)ROM into DSP memory for the controller to execute.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param board_id The SDSU CCD Controller board to send the command to, one of 
 * 	CCD_DSP_INTERFACE_BOARD_ID(interface),
 *	CCD_DSP_TIM_BOARD_ID(timing board) or CCD_DSP_UTIL_BOARD_ID(utility board).
 * @param application_number The number of the application on (EEP)ROM to load.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Lda
 * @see #DSP_Check_Reply
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_LDA(char *class,char *source,CCD_Interface_Handle_T* handle,
			enum CCD_DSP_BOARD_ID board_id,int application_number)
{
	int retval;

	DSP_Error_Number = 0;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_LDA(%d,%d) started.",
		board_id,application_number);
#endif
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
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
	if(!DSP_Send_Lda(class,source,handle,board_id,application_number,&retval))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock(handle))
		return FALSE;
#endif
	/* check reply - should be DON */
	if(DSP_Check_Reply(class,source,retval,CCD_DSP_DON) != CCD_DSP_DON)
		return FALSE;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_LDA(%d,%d) returned %d.",
		board_id,application_number,retval);
#endif
	return retval;
}

/**
 * This routine executes the ReaD Memory (RDM) command on a SDSU Controller board. 
 * This gets the value of a word of memory, location specified by board,memory space and address, and returns
 * it's value.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * The routine checks that the exposure status is non-zero if we are reading from the utility board, 
 * as this involves sending a DSP command to the utility board which cannot be undertaken during an exposure.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param board_id The SDSU CCD Controller board to send the command to, one of 
 * 	CCD_DSP_INTERFACE_BOARD_ID(interface),
 *	CCD_DSP_TIM_BOARD_ID(timing board) or CCD_DSP_UTIL_BOARD_ID(utility board).
 * @param mem_space The memory space on board board_id to read from, of type 
 * <a href="#CCD_DSP_MEM_SPACE">CCD_DSP_MEM_SPACE</a>. One of:
 * 	CCD_DSP_MEM_SPACE_P(program),
 * 	CCD_DSP_MEM_SPACE_X(X data),
 * 	CCD_DSP_MEM_SPACE_Y(Y data)
 * 	or CCD_DSP_MEM_SPACE_R(ROM).
 * @param address The memory address to read from.
 * @return The routine returns the actual value at the memory location or zero if an error occurs. If zero is
 * 	returned this can either mean that memory address contains zero OR an error occured. It can be
 *	determined properly if an error occured by looking at 
 * 	DSP_Error_Number, it it is zero the memory location contains zero, if it is non-zero than an error occured.
 * @see #DSP_Send_Rdm
 * @see #DSP_Check_Reply
 * @see #DSP_ACTUAL_VALUE
 * @see #DSP_Error_Number
 * @see ccd_exposure.html#CCD_Exposure_Get_Exposure_Status
 * @see ccd_exposure.html#CCD_EXPOSURE_STATUS
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_RDM(char *class,char *source,CCD_Interface_Handle_T* handle,
			enum CCD_DSP_BOARD_ID board_id,enum CCD_DSP_MEM_SPACE mem_space,int address)
{
	int retval;

	DSP_Error_Number = 0;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,
			      "CCD_DSP_Command_RDM(handle=%p,board_id=%d,mem_space=%d,address=%#x) started.",
			      handle,board_id,mem_space,address);
#endif
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
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
/* Version 1.3: We can only read memory on the utility board when we are not exposing.
** Version 1.4: We can read memory on the utility board when we are exposing.
** Version 1.7: We can only read memory on the utility board when we are not reading out.
*/
#ifdef CCD_DSP_UTIL_EXPOSURE_CHECK
#if CCD_DSP_UTIL_EXPOSURE_CHECK == 1
	if((board_id == CCD_DSP_UTIL_BOARD_ID)&&
	   (CCD_Exposure_Get_Exposure_Status(handle) != CCD_EXPOSURE_STATUS_NONE)&&
	   (CCD_Exposure_Get_Exposure_Status(handle) != CCD_EXPOSURE_STATUS_WAIT_START)&&
	   (CCD_Exposure_Get_Exposure_Status(handle) != CCD_EXPOSURE_STATUS_POST_READOUT))
#elif CCD_DSP_UTIL_EXPOSURE_CHECK == 2
	if ((CCD_Exposure_Get_Exposure_Status(handle) == CCD_EXPOSURE_STATUS_PRE_READOUT)||
	   (CCD_Exposure_Get_Exposure_Status(handle) == CCD_EXPOSURE_STATUS_READOUT))
#elif CCD_DSP_UTIL_EXPOSURE_CHECK == 3
	if ((CCD_Exposure_Get_Exposure_Status(handle) == CCD_EXPOSURE_STATUS_WAIT_START)||
	    (CCD_Exposure_Get_Exposure_Status(handle) == CCD_EXPOSURE_STATUS_CLEAR)||
	    (CCD_Exposure_Get_Exposure_Status(handle) == CCD_EXPOSURE_STATUS_EXPOSE)||
	    (CCD_Exposure_Get_Exposure_Status(handle) == CCD_EXPOSURE_STATUS_PRE_READOUT)||
	    (CCD_Exposure_Get_Exposure_Status(handle) == CCD_EXPOSURE_STATUS_READOUT))
#endif
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		DSP_Error_Number = 64; /* this error code is checked for in the Java layer */
		sprintf(DSP_Error_String,"CCD_DSP_Command_RDM failed:Illegal Exposure Status (%d) when"
			" reading from the utility board.", CCD_Exposure_Get_Exposure_Status(handle));
		return FALSE;
	}
#endif
	if(!DSP_Send_Rdm(class,source,handle,board_id,mem_space,address,&retval))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock(handle))
		return FALSE;
#endif
	/* check reply - actual value of memory location returned so this does nothing! */
	DSP_Check_Reply(class,source,retval,DSP_ACTUAL_VALUE);
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_RDM(%d,%d,%d) returned %d(%#x).",
		board_id,mem_space,address,retval,retval);
#endif
	return retval;
}

/**
 * This routine Tests the Data Link (TDL) on a SDSU Controller board. 
 * This ensures the host computer can communicate with the board.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * The routine checks that the exposure status is non-zero if we are writing to the utility board, 
 * as this involves sending a DSP command to the utility board which cannot be undertaken during an exposure.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param board_id The SDSU CCD Controller board to send the command to, one of 
 * 	CCD_DSP_INTERFACE_BOARD_ID(interface),
 *	CCD_DSP_TIM_BOARD_ID(timing board) or CCD_DSP_UTIL_BOARD_ID(utility board).
 * @param data The data value to send to the boards to test the data connection.
 * @return The routine returns the data value if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Tdl
 * @see #DSP_Check_Reply
 * @see ccd_exposure.html#CCD_Exposure_Get_Exposure_Status
 * @see ccd_exposure.html#CCD_EXPOSURE_STATUS
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_TDL(char *class,char *source,CCD_Interface_Handle_T* handle,
			enum CCD_DSP_BOARD_ID board_id,int data)
{
	int retval;

	DSP_Error_Number = 0;
	/* check - is board_id a legal value */
	if(!CCD_DSP_IS_BOARD_ID(board_id))
	{
		DSP_Error_Number = 6;
		sprintf(DSP_Error_String,"CCD_DSP_Command_TDL:Illegal board ID '%d'.",board_id);
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
/* Version 1.3: We can only TDL on the utility board when we are not exposing.
** Version 1.4: We can TDL on the utility board when we are exposing.
** Version 1.7: We can only TDL on the utility board when we are not reading out.
*/
#ifdef CCD_DSP_UTIL_EXPOSURE_CHECK
#if CCD_DSP_UTIL_EXPOSURE_CHECK == 1
	if((board_id == CCD_DSP_UTIL_BOARD_ID)&&
	   (CCD_Exposure_Get_Exposure_Status(handle) != CCD_EXPOSURE_STATUS_NONE)&&
	   (CCD_Exposure_Get_Exposure_Status(handle) != CCD_EXPOSURE_STATUS_WAIT_START)&&
	   (CCD_Exposure_Get_Exposure_Status(handle) != CCD_EXPOSURE_STATUS_POST_READOUT))
#elif CCD_DSP_UTIL_EXPOSURE_CHECK == 2
	if ((CCD_Exposure_Get_Exposure_Status(handle) == CCD_EXPOSURE_STATUS_PRE_READOUT)||
	   (CCD_Exposure_Get_Exposure_Status(handle) == CCD_EXPOSURE_STATUS_READOUT))
#elif CCD_DSP_UTIL_EXPOSURE_CHECK == 3
	if ((CCD_Exposure_Get_Exposure_Status(handle) == CCD_EXPOSURE_STATUS_WAIT_START)||
	    (CCD_Exposure_Get_Exposure_Status(handle) == CCD_EXPOSURE_STATUS_CLEAR)||
	    (CCD_Exposure_Get_Exposure_Status(handle) == CCD_EXPOSURE_STATUS_EXPOSE)||
	    (CCD_Exposure_Get_Exposure_Status(handle) == CCD_EXPOSURE_STATUS_PRE_READOUT)||
	    (CCD_Exposure_Get_Exposure_Status(handle) == CCD_EXPOSURE_STATUS_READOUT))
#endif
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		DSP_Error_Number = 65; /* this error code is checked for in the Java layer */
		sprintf(DSP_Error_String,"CCD_DSP_Command_TDL failed:Illegal Exposure Status (%d) when"
			" testing the utility board.",CCD_Exposure_Get_Exposure_Status(handle));
		return FALSE;
	}
#endif
	if(!DSP_Send_Tdl(class,source,handle,board_id,data,&retval))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock(handle))
		return FALSE;
#endif
	/* check reply - data value sent should be returned */
	if(DSP_Check_Reply(class,source,retval,data) != data)
		return FALSE;
	return retval;
}

/**
 * This routine executes the WRite Memory (WRM) command on a SDSU Controller board.
 * This sets the value of a word of memory, it's location specified by board,memory space and address.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * The routine checks that the exposure status is non-zero if we are writing to the utility board, 
 * as this involves sending a DSP command to the utility board which cannot be undertaken during an exposure.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param board_id The SDSU CCD Controller board to send the command to, one of 
 * 	CCD_DSP_INTERFACE_BOARD_ID(interface),
 *	CCD_DSP_TIM_BOARD_ID(timing board) or CCD_DSP_UTIL_BOARD_ID(utility board).
 * @param mem_space The memory space on board board_id to read from, of type 
 * <a href="#CCD_DSP_MEM_SPACE">CCD_DSP_MEM_SPACE</a>. One of:
 * 	CCD_DSP_MEM_SPACE_P(program),
 * 	CCD_DSP_MEM_SPACE_X(X data),
 * 	CCD_DSP_MEM_SPACE_Y(Y data)
 * 	or CCD_DSP_MEM_SPACE_R(ROM).
 * @param address The memory address to write data to.
 * @param data The data value to write to the memory address.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Wrm
 * @see #DSP_Check_Reply
 * @see ccd_exposure.html#CCD_Exposure_Get_Exposure_Status
 * @see ccd_exposure.html#CCD_EXPOSURE_STATUS
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_WRM(char *class,char *source,CCD_Interface_Handle_T* handle,
			enum CCD_DSP_BOARD_ID board_id,enum CCD_DSP_MEM_SPACE mem_space,int address,int data)
{
	int retval;

	DSP_Error_Number = 0;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,
			      "CCD_DSP_Command_WRM(handle=%p,board_id=%d,mem_space=%d,address=%#x,data=%#x) started.",
			      handle,board_id,mem_space,address,data);
#endif
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
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
/* Version 1.3: We can only write memory on the utility board when we are not exposing.
** Version 1.4: We can write memory on the utility board when we are exposing.
** Version 1.7: We can only write memory on the utility board when we are not reading out.
*/
#ifdef CCD_DSP_UTIL_EXPOSURE_CHECK
#if CCD_DSP_UTIL_EXPOSURE_CHECK == 1
	if((board_id == CCD_DSP_UTIL_BOARD_ID)&&
	   (CCD_Exposure_Get_Exposure_Status(handle) != CCD_EXPOSURE_STATUS_NONE)&&
	   (CCD_Exposure_Get_Exposure_Status(handle) != CCD_EXPOSURE_STATUS_WAIT_START)&&
	   (CCD_Exposure_Get_Exposure_Status(handle) != CCD_EXPOSURE_STATUS_POST_READOUT))
#elif CCD_DSP_UTIL_EXPOSURE_CHECK == 2
	if ((CCD_Exposure_Get_Exposure_Status(handle) == CCD_EXPOSURE_STATUS_PRE_READOUT)||
	   (CCD_Exposure_Get_Exposure_Status(handle) == CCD_EXPOSURE_STATUS_READOUT))
#elif CCD_DSP_UTIL_EXPOSURE_CHECK == 3
	if ((CCD_Exposure_Get_Exposure_Status(handle) == CCD_EXPOSURE_STATUS_WAIT_START)||
	    (CCD_Exposure_Get_Exposure_Status(handle) == CCD_EXPOSURE_STATUS_CLEAR)||
	    (CCD_Exposure_Get_Exposure_Status(handle) == CCD_EXPOSURE_STATUS_EXPOSE)||
	    (CCD_Exposure_Get_Exposure_Status(handle) == CCD_EXPOSURE_STATUS_PRE_READOUT)||
	    (CCD_Exposure_Get_Exposure_Status(handle) == CCD_EXPOSURE_STATUS_READOUT))
#endif
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		DSP_Error_Number = 91; /* this error code is checked for in the Java layer */
		sprintf(DSP_Error_String,"CCD_DSP_Command_WRM failed:Illegal Exposure Status (%d) when"
			" writing to the utility board.",CCD_Exposure_Get_Exposure_Status(handle));
		return FALSE;
	}
#endif
	if(!DSP_Send_Wrm(class,source,handle,board_id,mem_space,address,data,&retval))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock(handle))
		return FALSE;
#endif
	/* check reply - DON should be returned */
	if(DSP_Check_Reply(class,source,retval,CCD_DSP_DON) != CCD_DSP_DON)
		return FALSE;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,
			      "CCD_DSP_Command_WRM(%d,%d,%#x,%#x) returned %s(%#x).",
			      board_id,mem_space,address,data,DSP_Manual_Command_To_String(retval),retval);
#endif
	return retval;
}

/* timing board commands */
/**
 * This routine executes the ABort Readout (ABR) command on a SDSU Controller board.
 * If the SDSU CCD Controller is currently reading out the CCD, it is stopped immediately.
 * The command waits for a 'DON' message to be returned from the timing board. This is returned
 * after the PCI and timing boards have stopped the flow of readout data.
 * This routine is not mutexed, the Abort Readout command should be sent whilst a mutexed read is underway from the 
 * controller.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Abr
 * @see #DSP_Check_Reply
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_ABR(char *class,char *source,CCD_Interface_Handle_T* handle)
{
	int retval;

	DSP_Error_Number = 0;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_ABR(handle=%p) started.",handle);
#endif
	if(!DSP_Send_Abr(class,source,handle,&retval))
		return FALSE;
	/* check reply - DON should be returned */
	if(DSP_Check_Reply(class,source,retval,CCD_DSP_DON) != CCD_DSP_DON)
		return FALSE;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_ABR(handle=%p) returned %#x.",
			      handle,retval);
#endif
	return retval;
}

/**
 * This routine executes the CLeaR (CLR) command on a SDSU Controller board. This
 * clocks out any stored charge on the CCD, leaving the CCD ready for an exposure.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Clr
 * @see #DSP_Check_Reply
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_CLR(char *class,char *source,CCD_Interface_Handle_T* handle)
{
	int retval;

	DSP_Error_Number = 0;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_CLR(handle=%p) started.",handle);
#endif
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
	if(!DSP_Send_Clr(class,source,handle,&retval))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock(handle))
		return FALSE;
#endif
	/* check reply - DON should be returned */
	if(DSP_Check_Reply(class,source,retval,CCD_DSP_DON) != CCD_DSP_DON)
		return FALSE;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_CLR(handle=%p) finished.",handle);
#endif
	return CCD_DSP_DON;
}

/**
 * This routine executes the ReaD Ccd (RDC) command on a SDSU Controller board. This
 * sends the RDC command to the timing board, which starts the CCD reading out. This command is normally
 * issused internally on the timing board during a SEX command. 
 * The Exposure_Status variable is maintained to show the current status of the exposure.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #CCD_DSP_Command_SEX
 * @see #DSP_Send_Rdc
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_RDC(char *class,char *source,CCD_Interface_Handle_T* handle)
{
	int retval;

	DSP_Error_Number = 0;
#if LOGGING > 4
	CCD_Global_Log(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_RDC() started.");
#endif
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
/* set exposure status */
	if(!DSP_Send_Rdc(class,source,handle,&retval))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
/* no reply is generated for a RDC command. */
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock(handle))
	{
		return FALSE;
	}
#endif
#if LOGGING > 4
	CCD_Global_Log(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_RDC() finished.");
#endif
	return CCD_DSP_DON;
}

/**
 * This routine executes the IDLe (IDL) command on a SDSU Controller board. This
 * puts the clocks in the readout sequence, but does not transfer the clocked data to prevent charge from 
 * building up on the CCD.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #CCD_DSP_Command_STP
 * @see #DSP_Send_Idl
 * @see #DSP_Check_Reply
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_IDL(char *class,char *source,CCD_Interface_Handle_T* handle)
{
	int retval;

	DSP_Error_Number = 0;
#if LOGGING > 4
	CCD_Global_Log(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_IDL() started.");
#endif
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
	if(!DSP_Send_Idl(class,source,handle,&retval))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock(handle))
		return FALSE;
#endif
/* check reply - DON should be returned */
	if(DSP_Check_Reply(class,source,retval,CCD_DSP_DON) != CCD_DSP_DON)
		return FALSE;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_IDL() returned %#x.",retval);
#endif
	return retval;
}

/**
 * This routine executes the Set Bias Voltage (SBV) command on a 
 * SDSU Controller board. This
 * sets the voltage of the video processor DC bias and clock driver DACs from information in DSP memory.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Sbv
 * @see #DSP_Check_Reply
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_SBV(char *class,char *source,CCD_Interface_Handle_T* handle)
{
	int retval;

	DSP_Error_Number = 0;
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
	if(!DSP_Send_Sbv(class,source,handle,&retval))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock(handle))
		return FALSE;
#endif
	/* check reply - DON should be returned */
	if(DSP_Check_Reply(class,source,retval,CCD_DSP_DON) != CCD_DSP_DON)
		return FALSE;
	return retval;
}

/**
 * SGN command routine. Timing board command that means Set GaiN. 
 * This sets the gains of all the video processors.
 * The integrator speed is also set using this command to slow or fast.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param gain The gain to set the video processors to. One of:
 * 	CCD_DSP_GAIN_ONE(one),CCD_DSP_GAIN_TWO(two),CCD_DSP_GAIN_FOUR(4.75) and
 * 	CCD_DSP_GAIN_NINE(9.5).
 * @param speed The integrator speed to set the video processors to. Either 0 or 1.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Sgn
 * @see #DSP_Check_Reply
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_SGN(char *class,char *source,CCD_Interface_Handle_T* handle,enum CCD_DSP_GAIN gain,int speed)
{
	int retval;

	DSP_Error_Number = 0;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,
			      "CCD_DSP_Command_SGN(handle=%p,gain=%#x,speed=%d) started.",
			      handle,gain,speed);
#endif
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
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
	if(!DSP_Send_Sgn(class,source,handle,gain,speed,&retval))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock(handle))
		return FALSE;
#endif
	/* check reply - DON should be returned */
	if(DSP_Check_Reply(class,source,retval,CCD_DSP_DON) != CCD_DSP_DON)
		return FALSE;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,
			      "CCD_DSP_Command_SGN(handle=%p,gain=%#x,speed=%d) returned %s(%#x).",
			      handle,gain,speed,DSP_Manual_Command_To_String(retval),retval);
#endif
	return retval;
}

/**
 * SOS command routine. Timing board command that means Set Output Source. 
 * This sets which video amplifier to read the CCD chip out from.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param amplifier The amplifier to use when reading out the CCD. One of:
 * 	CCD_DSP_AMPLIFIER_LEFT, CCD_DSP_AMPLIFIER_RIGHT or CCD_DSP_AMPLIFIER_BOTH.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Sos
 * @see #DSP_Check_Reply
 * @see #CCD_DSP_AMPLIFIER
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_SOS(char *class,char *source,CCD_Interface_Handle_T* handle,enum CCD_DSP_AMPLIFIER amplifier)
{
	int retval;

	DSP_Error_Number = 0;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,
			      "CCD_DSP_Command_SOS(handle=%p,amplifier=%#x) started.",handle,amplifier);
#endif
	if(!CCD_DSP_IS_AMPLIFIER(amplifier))
	{
		DSP_Error_Number = 89;
		sprintf(DSP_Error_String,"CCD_DSP_Command_SOS:Illegal amplifier '%d'.",amplifier);
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
	if(!DSP_Send_Sos(class,source,handle,amplifier,&retval))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock(handle))
		return FALSE;
#endif
	/* check reply - DON should be returned */
	if(DSP_Check_Reply(class,source,retval,CCD_DSP_DON) != CCD_DSP_DON)
		return FALSE;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,
			      "CCD_DSP_Command_SOS(handle=%p,amplifier=%#x) returned %s(%#x).",
			      handle,amplifier,DSP_Manual_Command_To_String(retval),retval);
#endif
	return retval;
}

/**
 * SSP command routine. Timing board command that means Set Subarray Position. 
 * This sets the position of a subarray box on the chip. This should be called after
 * CCD_DSP_Command_SSS, which resets the number of subarray boxes to zero.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param y_offset The number of rows (parallel) to clear AFTER THE LAST BOX (in pixels).
 * @param x_offset The number of columns (serial) to clear from the left hand edge of the chip (in pixels).
 * @param bias_x_offset The number of columns (serial) gap to leave between the right hand side of
 *        the subarray box and the start of the bias strip (in pixels).
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Ssp
 * @see #DSP_Check_Reply
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_SSP(char *class,char *source,CCD_Interface_Handle_T* handle,
			int y_offset,int x_offset,int bias_x_offset)
{
	int retval;

	DSP_Error_Number = 0;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,
			      "CCD_DSP_Command_SSP(handle=%p,y_offset=%d,x_offset=%d,bias_x_offset=%d) started.",
			      handle,y_offset,x_offset,bias_x_offset);
#endif
	if(y_offset < 0)
	{
		DSP_Error_Number = 28;
		sprintf(DSP_Error_String,"CCD_DSP_Command_SSP:Illegal Y Offset '%d'.",y_offset);
		return FALSE;
	}
	if(x_offset < 0)
	{
		DSP_Error_Number = 30;
		sprintf(DSP_Error_String,"CCD_DSP_Command_SSP:Illegal X Offset '%d'.",x_offset);
		return FALSE;
	}
	if(bias_x_offset < 0)
	{
		DSP_Error_Number = 32;
		sprintf(DSP_Error_String,"CCD_DSP_Command_SSP:Illegal Bias X Offset '%d'.",bias_x_offset);
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
	if(!DSP_Send_Ssp(class,source,handle,y_offset,x_offset,bias_x_offset,&retval))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock(handle))
		return FALSE;
#endif
	/* check reply - DON should be returned */
	if(DSP_Check_Reply(class,source,retval,CCD_DSP_DON) != CCD_DSP_DON)
		return FALSE;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_SSP() returned %s(%#x).",
			      DSP_Manual_Command_To_String(retval),retval);
#endif
	return retval;
}

/**
 * SSS command routine. Timing board command that means Set Subarray Size. 
 * This sets the width and heights of all the subarray boxes.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param bias_width The width of the bias strip (in pixels).
 * @param box_width The width of the subarray box (in pixels).
 * @param box_height The height of the subarray box (in pixels).
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Sss
 * @see #DSP_Check_Reply
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_SSS(char *class,char *source,CCD_Interface_Handle_T* handle,
			int bias_width,int box_width,int box_height)
{
	int retval;

	DSP_Error_Number = 0;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,
			      "CCD_DSP_Command_SSS(handle=%p,bias_width=%d,box_width=%d,box_height=%d) started.",
			      handle,bias_width,box_width,box_height);
#endif
	if(bias_width < 0)
	{
		DSP_Error_Number = 21;
		sprintf(DSP_Error_String,"CCD_DSP_Command_SSS:Illegal bias width '%d'.",bias_width);
		return FALSE;
	}
	if(box_width < 0)
	{
		DSP_Error_Number = 25;
		sprintf(DSP_Error_String,"CCD_DSP_Command_SSS:Illegal box width '%d'.",box_width);
		return FALSE;
	}
	if(box_height < 0)
	{
		DSP_Error_Number = 27;
		sprintf(DSP_Error_String,"CCD_DSP_Command_SSS:Illegal box height '%d'.",box_height);
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
	if(!DSP_Send_Sss(class,source,handle,bias_width,box_width,box_height,&retval))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock(handle))
		return FALSE;
#endif
	/* check reply - DON should be returned */
	if(DSP_Check_Reply(class,source,retval,CCD_DSP_DON) != CCD_DSP_DON)
		return FALSE;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_SSS() returned %s(%#x).",
			      DSP_Manual_Command_To_String(retval),retval);
#endif
	return retval;
}

/**
 * This routine executes the SToP (STP) command on the timing board. This
 * stops the clocks clocking the readout sequence.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #CCD_DSP_Command_IDL
 * @see #DSP_Send_Stp
 * @see #DSP_Check_Reply
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_STP(char *class,char *source,CCD_Interface_Handle_T* handle)
{
	int retval;

	DSP_Error_Number = 0;
#if LOGGING > 4
	CCD_Global_Log(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_STP() started.");
#endif
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
	if(!DSP_Send_Stp(class,source,handle,&retval))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock(handle))
		return FALSE;
#endif
	/* check reply - DON should be returned */
	if(DSP_Check_Reply(class,source,retval,CCD_DSP_DON) != CCD_DSP_DON)
		return FALSE;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_STP() returned %#x.",retval);
#endif
	return retval;
}

/**
 * This routine executes the Abort EXposure (AEX) command on a 
 * SDSU utility board. If an
 * exposure is currently underway this is stopped by closing the shutter and putting the CCD in idle mode.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Aex
 * @see #DSP_Check_Reply
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_AEX(char *class,char *source,CCD_Interface_Handle_T* handle)
{
	int retval;

	DSP_Error_Number = 0;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_AEX(handle=%p) started.",handle);
#endif
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
	if(!DSP_Send_Aex(class,source,handle,&retval))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock(handle))
		return FALSE;
#endif
	/* check reply - DON should be returned */
	if(DSP_Check_Reply(class,source,retval,CCD_DSP_DON) != CCD_DSP_DON)
		return FALSE;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_AEX(handle=%p) returned %s(%#x).",
			      handle,DSP_Manual_Command_To_String(retval),retval);
#endif
	return retval;
}

/**
 * This routine executes the Close SHutter (CSH) command on the SDSU utility board.
 * This closes the shutter.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Csh
 * @see #DSP_Check_Reply
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_CSH(char *class,char *source,CCD_Interface_Handle_T* handle)
{
	int retval;

	DSP_Error_Number = 0;
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
	if(!DSP_Send_Csh(class,source,handle,&retval))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock(handle))
		return FALSE;
#endif
	/* check reply - DON should be returned */
	if(DSP_Check_Reply(class,source,retval,CCD_DSP_DON) != CCD_DSP_DON)
		return FALSE;
	return retval;
}

/**
 * This routine executes the Open SHutter (OSH) command on the SDSU utility board.
 * This opens the shutter.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Osh
 * @see #DSP_Check_Reply
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_OSH(char *class,char *source,CCD_Interface_Handle_T* handle)
{
	int retval;

	DSP_Error_Number = 0;
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
	if(!DSP_Send_Osh(class,source,handle,&retval))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock(handle))
		return FALSE;
#endif
	/* check reply - DON should be returned */
	if(DSP_Check_Reply(class,source,retval,CCD_DSP_DON) != CCD_DSP_DON)
		return FALSE;
	return retval;
}

/**
 * This routine executes the Pause EXposure (PEX) command on the
 * SDSU utility board. This closes the shutter and stops the timer.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #CCD_DSP_Command_REX
 * @see #DSP_Send_Pex
 * @see #DSP_Check_Reply
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_PEX(char *class,char *source,CCD_Interface_Handle_T* handle)
{
	int retval;

	DSP_Error_Number = 0;
#if LOGGING > 4
	CCD_Global_Log(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_PEX() started.");
#endif
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
	if(!DSP_Send_Pex(class,source,handle,&retval))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock(handle))
		return FALSE;
#endif
	/* check reply - DON should be returned */
	if(DSP_Check_Reply(class,source,retval,CCD_DSP_DON) != CCD_DSP_DON)
		return FALSE;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_PEX() returned %#x.",retval);
#endif
	return retval;
}

/**
 * This routine executes the Power ON (PON) command on a SDSU Controller board.
 * This turns the analog power on safely, using the power control board.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Pon
 * @see #DSP_Check_Reply
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_PON(char *class,char *source,CCD_Interface_Handle_T* handle)
{
	int retval;

	DSP_Error_Number = 0;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_PON(handle=%p) started.",handle);
#endif
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
	if(!DSP_Send_Pon(class,source,handle,&retval))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock(handle))
		return FALSE;
#endif
	/* check reply - DON should be returned */
	if(DSP_Check_Reply(class,source,retval,CCD_DSP_DON) != CCD_DSP_DON)
		return FALSE;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_PON(handle=%p) returned %s(%#x).",
			      handle,DSP_Manual_Command_To_String(retval),retval);
#endif
	return retval;
}

/**
 * This routine executes the Power OFF (POF) command on a SDSU Controller board.
 * This turns the analog power off safely, using the power control board.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Pof
 * @see #DSP_Check_Reply
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_POF(char *class,char *source,CCD_Interface_Handle_T* handle)
{
	int retval;

	DSP_Error_Number = 0;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_POF(handle=%p) started.",handle);
#endif
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
	if(!DSP_Send_Pof(class,source,handle,&retval))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock(handle))
		return FALSE;
#endif
	/* check reply - DON should be returned */
	if(DSP_Check_Reply(class,source,retval,CCD_DSP_DON) != CCD_DSP_DON)
		return FALSE;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_POF(handle=%p) returned %s(%#x).",
			      handle,DSP_Manual_Command_To_String(retval),retval);
#endif
	return retval;
}

/**
 * This routine executes the Resume EXposure (REX) command on a 
 * SDSU Controller board. This opens the shutter and restarts the timer.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #CCD_DSP_Command_PEX
 * @see #DSP_Send_Rex
 * @see #DSP_Check_Reply
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_REX(char *class,char *source,CCD_Interface_Handle_T* handle)
{
	int retval;

	DSP_Error_Number = 0;
#if LOGGING > 4
	CCD_Global_Log(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_REX() started.");
#endif
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
	if(!DSP_Send_Rex(class,source,handle,&retval))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock(handle))
		return FALSE;
#endif
	/* check reply - DON should be returned */
	if(DSP_Check_Reply(class,source,retval,CCD_DSP_DON) != CCD_DSP_DON)
		return FALSE;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_REX() returned %#x.",retval);
#endif
	return retval;
}

/**
 * This routine executes the Start EXposure (SEX) command on a SDSU Controller board.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param start_time The time to start the exposure. If the tv_sec field of the structure is zero,
 * 	we can start the exposure at any convenient time.
 * @param exposure_length The length of exposure we are about to start. Passed to DSP_Send_Sex.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #CCD_DSP_EXPOSURE_MAX_LENGTH
 * @see #DSP_Send_Sex
 * @see #DSP_Check_Reply
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_SEX(char *class,char *source,CCD_Interface_Handle_T* handle,
			struct timespec start_time,int exposure_length)
{
	int retval;

	DSP_Error_Number = 0;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,
			      "CCD_DSP_Command_SEX(handle=%p,exposure_length=%d) started.",handle,exposure_length);
#endif
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
	if(!DSP_Send_Sex(class,source,handle,start_time,exposure_length,&retval))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
       	if(!DSP_Mutex_Unlock(handle))
		return FALSE;
#endif
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_SEX(handle=%p) returned %#x.",
			      handle,retval);
#endif
	if(DSP_Check_Reply(class,source,retval,CCD_DSP_DON) != CCD_DSP_DON)
		return FALSE;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_SEX(handle=%p) returned DON.",
			      handle);
#endif
	return retval;
}

/**
 * This routine resets the SDSU Controller boards. It sends the PCI reset controller command.
 * It then checks the reply from the interface, which should be SYR.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @return The routine returns SYR if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Reset
 * @see #DSP_Check_Reply
 * @see #CCD_DSP_SYR
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_Reset(char *class,char *source,CCD_Interface_Handle_T* handle)
{
	int retval;

	DSP_Error_Number = 0;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_Reset(handle=%p) started.",handle);
#endif
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
	if(!DSP_Send_Reset(class,source,handle,&retval))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock(handle))
		return FALSE;
#endif
	/* check reply - SYR should be returned */
	if(DSP_Check_Reply(class,source,retval,CCD_DSP_SYR) != CCD_DSP_SYR)
		return FALSE;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_Reset(handle=%p) returned %s(%#x).",
			      handle,DSP_Manual_Command_To_String(retval),retval);
#endif
	return retval;
}

/**
 * Routine to get the current value of the Host Interface Status Register (HSTR).
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param value The address of an integer to store the HSTR value.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @return The routine returns TRUE if the operation succeeds, FALSE otherwise.
 * @see ccd_pci.html#CCD_PCI_IOCTL_GET_HSTR
 * @see ccd_interface.html#CCD_Interface_Command
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_Get_HSTR(char *class,char *source,CCD_Interface_Handle_T* handle,int *value)
{
	DSP_Error_Number = 0;
	if(value == NULL)
	{
		DSP_Error_Number = 10;
		sprintf(DSP_Error_String,"CCD_DSP_Command_Get_HSTR:value was NULL.");
		return FALSE;
	}
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_Get_HSTR(handle=%p) started.",
			      handle);
#endif
	(*value) = 0;
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
	if(!CCD_Interface_Command(handle,CCD_PCI_IOCTL_GET_HSTR,value))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		DSP_Error_Number = 11;
		sprintf(DSP_Error_String,"CCD_DSP_Command_Get_HSTR:Sending Get HSTR failed.");
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock(handle))
		return FALSE;
#endif
	return TRUE;
}

/**
 * Routine to get the current progress of the readout. Returns the number of pixels read out.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param value The address of an integer to store the HSTR value.
 * @return The routine returns TRUE if the operation succeeds, FALSE otherwise.
 * @see ccd_pci.html#CCD_PCI_IOCTL_GET_PROGRESS
 * @see ccd_interface.html#CCD_Interface_Command
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_Get_Readout_Progress(char *class,char *source,CCD_Interface_Handle_T* handle,int *value)
{
	DSP_Error_Number = 0;
	if(value == NULL)
	{
		DSP_Error_Number = 12;
		sprintf(DSP_Error_String,"CCD_DSP_Command_Get_Readout_Progress:value was NULL.");
		return FALSE;
	}
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,
			      "CCD_DSP_Command_Get_Readout_Progress(handle=%p) started.",handle);
#endif
	(*value) = 0;
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
	if(!CCD_Interface_Command(handle,CCD_PCI_IOCTL_GET_PROGRESS,value))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		DSP_Error_Number = 13;
		sprintf(DSP_Error_String,"CCD_DSP_Command_Get_Readout_Progress:Sending Get Progress failed.");
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock(handle))
		return FALSE;
#endif
	return TRUE;
}

/**
 * Routine to read the controller configuration word.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param value The address on an integer to store the returned controller configuration word. 
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #DSP_Send_Rcc
 * @see #DSP_Check_Reply
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_RCC(char *class,char *source,CCD_Interface_Handle_T* handle,int *value)
{
	DSP_Error_Number = 0;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_RCC(handle=%p) started.",handle);
#endif
	if(value == NULL)
	{
		DSP_Error_Number = 19;
		sprintf(DSP_Error_String,"CCD_DSP_Command_RCC:value was NULL.");
		return FALSE;
	}
	(*value) = 0;
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
	if(!DSP_Send_Rcc(class,source,handle,value))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock(handle))
		return FALSE;
#endif
/* check reply - the controller config value should be returned */
	if(DSP_Check_Reply(class,source,(*value),DSP_ACTUAL_VALUE) != DSP_ACTUAL_VALUE)
		return FALSE;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_RCC(handle=%p) returned %#x.",
			      handle,(*value));
#endif
	return TRUE;
}

/**
 * Routine to tell the SDSU PCI card to prepare for it's DSP code to be downloaded.
 * This command is unusual, as we don't wait for a DON to be returned, as we have to download
 * the DSP code before this occurs. For this reason, the command is <b>NOT</b> mutexed either.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @return The routine returns TRUE if the operation succeeds, FALSE otherwise.
 * @see #DSP_Send_PCI_Download
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_PCI_Download(char *class,char *source,CCD_Interface_Handle_T* handle)
{
	DSP_Error_Number = 0;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_PCI_Download(handle=%p) started.",
			      handle);
#endif
	if(!DSP_Send_PCI_Download(class,source,handle))
		return FALSE;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_PCI_Download(handle=%p) finished.",
			      handle);
#endif
	return TRUE;
}

/**
 * Routine to tell the SDSU PCI card that the DSP code has been downloaded to the SDSU PCI card
 * and to initialise itself ready for normal operation. The replt value is checked using DSP_Check_Reply.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @return The routine returns DON if the operation succeeds, FALSE otherwise.
 * @see #DSP_Send_PCI_Download_Wait
 * @see #DSP_Check_Reply
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_PCI_Download_Wait(char *class,char *source,CCD_Interface_Handle_T* handle)
{
	int retval;

	DSP_Error_Number = 0;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,
			      "CCD_DSP_Command_PCI_Download_Wait(handle=%p) started.",handle);
#endif
	if(DSP_Send_PCI_Download_Wait(class,source,handle,&retval) != TRUE)
		return FALSE;
	if(DSP_Check_Reply(class,source,retval,CCD_DSP_DON) != CCD_DSP_DON)
		return FALSE;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,
			      "CCD_DSP_Command_PCI_Download_Wait(handle=%p) returned %#x.",handle,retval);
#endif
	return retval;
}

/**
 * Routine to reset the PCI board's program counter, to stop PCI lockups occuring.
 * This command is <b>not</b> mutexed, as it can be called whilst other commands are in operation,
 * to reset a command that has caused the PCI interface to appear to stop.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @return The routine returns DON if the operation succeeds, FALSE otherwise.
 * @see #DSP_Send_PCI_PC_Reset
 * @see #DSP_Check_Reply
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_PCI_PC_Reset(char *class,char *source,CCD_Interface_Handle_T* handle)
{
	int retval;

	DSP_Error_Number = 0;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_PCI_PC_Reset(handle=%p) started.",
			      handle);
#endif
	if(!DSP_Send_PCI_PC_Reset(class,source,handle,&retval))
		return FALSE;
/* check reply - DON should be returned */
	if(DSP_Check_Reply(class,source,retval,CCD_DSP_DON) != CCD_DSP_DON)
		return FALSE;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,
			      "CCD_DSP_Command_PCI_PC_Reset(handle=%p) returned %#x.",handle,retval);
#endif
	return retval;
}

/**
 * Routine to set the exposure time. 
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param msecs The exposure time in milliseconds. 
 * @return The routine returns DON if the operation succeeds, FALSE otherwise.
 * @see #DSP_Send_Set
 * @see #DSP_Check_Reply
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_SET(char *class,char *source,CCD_Interface_Handle_T* handle,int msecs)
{
	int retval;

	DSP_Error_Number = 0;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_SET(handle=%p,msecs=%d) started.",
			      handle,msecs);
#endif
/* exposure time  must be a positive/zero number */
	if(msecs < 0)
	{
		DSP_Error_Number = 29;
		sprintf(DSP_Error_String,"CCD_DSP_Command_SET:Illegal msecs '%d'.",msecs);
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
	if(!DSP_Send_Set(class,source,handle,msecs,&retval))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock(handle))
		return FALSE;
#endif
/* check reply - DON should be returned */
	if(DSP_Check_Reply(class,source,retval,CCD_DSP_DON) != CCD_DSP_DON)
		return FALSE;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,
			      "CCD_DSP_Command_SET(handle=%p,msecs=%d) returned %s(%#x).",
			      handle,msecs,DSP_Manual_Command_To_String(retval),retval);
#endif
	return retval;
}

/**
 * Routine to get the amount of time the utility board has had the shutter open for, i.e. the
 * amount of time an exposure has been underway. 
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @return If an error has occured or an exposure is not taking place, FALSE is returned. Otherwise
 * 	the amount of time an exposure has been underway is returned, in milliseconds.
 * @see #DSP_Send_Ret
 * @see #DSP_Check_Reply
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
int CCD_DSP_Command_RET(char *class,char *source,CCD_Interface_Handle_T* handle)
{
	int retval;

	DSP_Error_Number = 0;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_RET(handle=%p) started.",handle);
#endif
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock(handle))
		return FALSE;
#endif
/* Version 1.7: We can only read elapsed exposure time when we are not reading out.
*/
#ifdef CCD_DSP_UTIL_EXPOSURE_CHECK
#if CCD_DSP_UTIL_EXPOSURE_CHECK == 1
	/* no test in this mode - we can still call RET when exposing. */
#elif CCD_DSP_UTIL_EXPOSURE_CHECK == 2
	if ((CCD_Exposure_Get_Exposure_Status(handle) == CCD_EXPOSURE_STATUS_PRE_READOUT)||
	   (CCD_Exposure_Get_Exposure_Status(handle) == CCD_EXPOSURE_STATUS_READOUT))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		DSP_Error_Number = 17; /* this error code is checked for in the Java layer */
		sprintf(DSP_Error_String,"CCD_DSP_Command_RET failed:Illegal Exposure Status (%d) when"
			" reading from the utility board.", CCD_Exposure_Get_Exposure_Status(handle));
		return FALSE;
	}
#elif CCD_DSP_UTIL_EXPOSURE_CHECK == 3
	/* no test in this mode - we can still call RET when exposing. */
#endif
#endif
	if(!DSP_Send_Ret(class,source,handle,&retval))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock(handle))
		return FALSE;
#endif
/* check reply - the exposure time in milliseconds returned so this does nothing! */
	if(DSP_Check_Reply(class,source,retval,DSP_ACTUAL_VALUE) != retval)
		return FALSE;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Command_Ret(handle=%p) returned %d (%#x).",
			      handle,retval,retval);
#endif
	return retval;
}

/**
 * This routine returns the current stste of the Abort flag.
 * The Abort flag is defined in DSP_Data and is set to true when
 * the user wants to stop execution mid-commend.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @return The current Abort status.
 * @see #CCD_DSP_Set_Abort
 */
int CCD_DSP_Get_Abort(CCD_Interface_Handle_T* handle)
{
	return handle->DSP_Data.Abort;
}

/**
 * This routine allows the setting and reseting of the Abort flag.
 * The Abort flag is defined in DSP_Data and is set to true when
 * the user wants to stop execution mid-commend.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param value What to set the Abort flag to: either TRUE or FALSE.
 * @return Returns TRUE or FALSE to indicate success/failure.
 * @see #CCD_DSP_Get_Abort
 * @see #DSP_Data
 */
int CCD_DSP_Set_Abort(char *class,char *source,CCD_Interface_Handle_T* handle,int value)
{
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Set_Abort(handle=%p,value=%d) started.",
			      handle,value);
#endif
	if(!CCD_GLOBAL_IS_BOOLEAN(value))
	{
		DSP_Error_Number = 26;
		sprintf(DSP_Error_String,"CCD_DSP_Set_Abort:Illegal value '%d'.",value);
		return FALSE;
	}
	handle->DSP_Data.Abort = value;
#if LOGGING > 4
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CCD_DSP_Set_Abort(handle=%p,value=%d) returned.",
			      handle,value);
#endif
	return TRUE;
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
 * @see ccd_global.html#CCD_Global_Get_Current_Time_String
 */
void CCD_DSP_Error(void)
{
	char time_string[32];

	CCD_Global_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(DSP_Error_Number == 0)
		sprintf(DSP_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s CCD_DSP:Error(%d) : %s\n",time_string,DSP_Error_Number,DSP_Error_String);
}

/**
 * The error routine that reports any errors occuring in ccd_dsp in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 * @see ccd_global.html#CCD_Global_Get_Current_Time_String
 */
void CCD_DSP_Error_String(char *error_string)
{
	char time_string[32];

	CCD_Global_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(DSP_Error_Number == 0)
		sprintf(DSP_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s CCD_DSP:Error(%d) : %s\n",time_string,
		DSP_Error_Number,DSP_Error_String);
}

/**
 * The warning routine that reports any warnings occuring in ccd_dsp in a standard way.
 * @see ccd_global.html#CCD_Global_Get_Current_Time_String
 */
void CCD_DSP_Warning(void)
{
	char time_string[32];

	CCD_Global_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an warning message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an warning to display */
	if(DSP_Error_Number == 0)
		sprintf(DSP_Error_String,"Logic Error:No Warning defined");
	fprintf(stderr,"%s CCD_DSP:Warning(%d) : %s\n",time_string,DSP_Error_Number,DSP_Error_String);
}

/* ----------------------------------------------------------------
**	Internal routines
** ---------------------------------------------------------------- */
/**
 * Internal DSP command to load a DSP application program from EEPROM to DSP memory.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param board_id The SDSU CCD Controller board,
 * 	CCD_DSP_TIM_BOARD_ID(timing board) or CCD_DSP_UTIL_BOARD_ID(utility board).
 * @param data The application number to load.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #CCD_DSP_BOARD_ID
 * @see #DSP_Send_Manual_Command
 * @see #CCD_DSP_LDA
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_Lda(char *class,char *source,CCD_Interface_Handle_T* handle,
			enum CCD_DSP_BOARD_ID board_id,int data,int *reply_value)
{
	int argument_list[1];
	int argument_count = 0;

	argument_list[argument_count++] = data;
	if(DSP_Send_Manual_Command(class,source,handle,board_id,CCD_DSP_LDA,argument_list,argument_count,
				   reply_value) != TRUE)
		return FALSE;
	return TRUE;
}

/**
 * Internal DSP command to read data from address address in memory space mem_space on board board_id. 
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param board_id The SDSU CCD Controller board, one of CCD_DSP_INTERFACE_BOARD_ID(interface),
 * 	CCD_DSP_TIM_BOARD_ID(timing board) or CCD_DSP_UTIL_BOARD_ID(utility board).
 * @param mem_space The memory space on board board_id to read from, of type 
 * <a href="#CCD_DSP_MEM_SPACE">CCD_DSP_MEM_SPACE</a>. One of:
 * 	CCD_DSP_MEM_SPACE_P(program),
 * 	CCD_DSP_MEM_SPACE_X(X data),
 * 	CCD_DSP_MEM_SPACE_Y(Y data)
 * 	or CCD_DSP_MEM_SPACE_R(ROM).
 * @param address The memory location to get the data.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return The data held at the specified address, or false if a failure occurs.
 * @see #CCD_DSP_RDM
 * @see #CCD_DSP_BOARD_ID
 * @see #DSP_Send_Manual_Command
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_Rdm(char *class,char *source,CCD_Interface_Handle_T* handle,
			enum CCD_DSP_BOARD_ID board_id,enum CCD_DSP_MEM_SPACE mem_space,int address,int *reply_value)
{
	int argument_list[1];
	int argument_count = 0;

	argument_list[argument_count++] = (mem_space|address);
	return DSP_Send_Manual_Command(class,source,handle,board_id,CCD_DSP_RDM,argument_list,argument_count,
				       reply_value);
}

/**
 * Internal DSP command to write data to address address in memory space mem_space to board board_id.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param board_id The SDSU CCD Controller board, one of CCD_DSP_INTERFACE_BOARD_ID(interface),
 * 	CCD_DSP_TIM_BOARD_ID(timing board) or CCD_DSP_UTIL_BOARD_ID(utility board).
 * @param mem_space The memory space on board board_id to read from, of type 
 * <a href="#CCD_DSP_MEM_SPACE">CCD_DSP_MEM_SPACE</a>. One of:
 * 	CCD_DSP_MEM_SPACE_P(program),
 * 	CCD_DSP_MEM_SPACE_X(X data),
 * 	CCD_DSP_MEM_SPACE_Y(Y data)
 * 	or CCD_DSP_MEM_SPACE_R(ROM).
 * @param address The memory location to put the data.
 * @param data The data to put into the memory location.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #CCD_DSP_WRM
 * @see #CCD_DSP_BOARD_ID
 * @see #DSP_Send_Manual_Command
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_Wrm(char *class,char *source,CCD_Interface_Handle_T* handle,
			enum CCD_DSP_BOARD_ID board_id,enum CCD_DSP_MEM_SPACE mem_space,int address,int data,
			int *reply_value)
{
	int argument_list[2];
	int argument_count = 0;

	argument_list[argument_count++] = (mem_space | address);
	argument_list[argument_count++] = data;
	return DSP_Send_Manual_Command(class,source,handle,board_id,CCD_DSP_WRM,argument_list,argument_count,
				       reply_value);
}

/**
 * Internal DSP command to test the data link to the SDSU CCD Controller is working correctly.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param board_id The board to send the command to. One of
 * 	CCD_DSP_TIM_BOARD_ID(timing board) or CCD_DSP_UTIL_BOARD_ID(utility board).
 * @param data The data to test the link with. This can any 24 bit number.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Manual_Command
 * @see #CCD_DSP_TDL
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_Tdl(char *class,char *source,CCD_Interface_Handle_T* handle,
			enum CCD_DSP_BOARD_ID board_id,int data,int *reply_value)
{
	int argument_list[1];
	int argument_count = 0;

	argument_list[argument_count++] = data;
	return DSP_Send_Manual_Command(class,source,handle,board_id,CCD_DSP_TDL,argument_list,argument_count,
				       reply_value);
}

/**
 * Internal DSP command to abort readout of the ccd.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_ABORT_READOUT
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_Abr(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value)
{
	return DSP_Send_Command(class,source,handle,CCD_PCI_HCVR_ABORT_READOUT,reply_value);
}

/**
 * Internal DSP command to clear the CCD of any stored charge on it, ready to begin a new exposure.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Manual_Command
 * @see ccd_dsp.html#CCD_DSP_CLR
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_Clr(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value)
{
	return DSP_Send_Manual_Command(class,source,handle,CCD_DSP_TIM_BOARD_ID,CCD_DSP_CLR,NULL,0,reply_value);
}

/**
 * Internal DSP command to tell the timing board to immediately start reading out the array.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Manual_Command
 * @see ccd_dsp.html#CCD_DSP_RDC
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_Rdc(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value)
{
	return DSP_Send_Manual_Command(class,source,handle,CCD_DSP_TIM_BOARD_ID,CCD_DSP_RDC,NULL,0,reply_value);
}

/**
 * Internal DSP command to put the CCD clocks in the readout sequence but not transfering any data. This stops the
 * CCD building up any charge.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Stp
 * @see #DSP_Send_Manual_Command
 * @see ccd_dsp.html#CCD_DSP_IDL
 * @see ccd_dsp.html#CCD_DSP_TIM_BOARD_ID
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_Idl(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value)
{
	return DSP_Send_Manual_Command(class,source,handle,CCD_DSP_TIM_BOARD_ID,CCD_DSP_IDL,NULL,0,reply_value);
}

/**
 * Internal DSP command to set bias voltages.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_SET_BIAS_VOLTAGES
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_Sbv(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value)
{
	return DSP_Send_Command(class,source,handle,CCD_PCI_HCVR_SET_BIAS_VOLTAGES,reply_value);
}

/**
 * Internal DSP command to set the gain values of the video processors. 
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param gain What value to set the gain to. One of :
 *	<dl>
 * 	<dt>CCD_DSP_GAIN_ONE</dt> <dd>Set gain = 1</dd>
 * 	<dt>CCD_DSP_GAIN_TWO</dt> <dd>Set gain = 2</dd>
 * 	<dt>CCD_DSP_GAIN_FOUR</dt> <dd>Set gain = 4.75</dd>
 * 	<dt>CCD_DSP_GAIN_NINE</dt> <dd>Set gain = 9.5</dd>
 * 	</dl>
 * @param speed Sets the speed of the integrators. TRUE is fast integrator speed, FALSE is slow integrator speed.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #CCD_DSP_BOARD_ID
 * @see #CCD_DSP_SGN
 * @see #DSP_Send_Manual_Command
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_Sgn(char *class,char *source,CCD_Interface_Handle_T* handle,
			enum CCD_DSP_GAIN gain,int speed,int *reply_value)
{
	int argument_list[2];
	int argument_count = 0;

	argument_list[argument_count++] = gain;
	argument_list[argument_count++] = speed;
	return DSP_Send_Manual_Command(class,source,handle,CCD_DSP_TIM_BOARD_ID,CCD_DSP_SGN,argument_list,
				       argument_count,reply_value);
}

/**
 * Internal DSP command to set the which amplifier to read out from during readout. 
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param amplifier What amplifier to use during readout. One of the CCD_DSP_AMPLIFIER enum values.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #CCD_DSP_BOARD_ID
 * @see #CCD_DSP_SOS
 * @see #DSP_Send_Manual_Command
 * @see #CCD_DSP_AMPLIFIER
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_Sos(char *class,char *source,CCD_Interface_Handle_T* handle,
			enum CCD_DSP_AMPLIFIER amplifier,int *reply_value)
{
	int argument_list[1];
	int argument_count = 0;

	argument_list[argument_count++] = amplifier;
	return DSP_Send_Manual_Command(class,source,handle,CCD_DSP_TIM_BOARD_ID,CCD_DSP_SOS,argument_list,
				       argument_count,reply_value);
}

/**
 * Internal DSP command to set the subbarray box positions. 
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param y_offset The number of rows (parallel) to clear AFTER THE LAST BOX (in pixels).
 * @param x_offset The number of columns (serial) to clear from the left hand edge of the chip (in pixels).
 * @param bias_x_offset The number of columns (serial) gap to leave between the right hand side of
 *        the subarray box and the start of the bias strip (in pixels).
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #CCD_DSP_BOARD_ID
 * @see #CCD_DSP_SSS
 * @see #DSP_Send_Manual_Command
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_Ssp(char *class,char *source,CCD_Interface_Handle_T* handle,
			int y_offset,int x_offset,int bias_x_offset,int *reply_value)
{
	int argument_list[3];
	int argument_count = 0;

	argument_list[argument_count++] = y_offset;
	argument_list[argument_count++] = x_offset;
	argument_list[argument_count++] = bias_x_offset;
	return DSP_Send_Manual_Command(class,source,handle,CCD_DSP_TIM_BOARD_ID,CCD_DSP_SSP,argument_list,
				       argument_count,reply_value);
}

/**
 * Internal DSP command to set the subbarray size. 
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param bias_width The width of the bias strip (in pixels).
 * @param box_width The width of the subarray box (in pixels).
 * @param box_height The height of the subarray box (in pixels).
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #CCD_DSP_BOARD_ID
 * @see #CCD_DSP_SSS
 * @see #DSP_Send_Manual_Command
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_Sss(char *class,char *source,CCD_Interface_Handle_T* handle,
			int bias_width,int box_width,int box_height,int *reply_value)
{
	int argument_list[3];
	int argument_count = 0;

	argument_list[argument_count++] = bias_width;
	argument_list[argument_count++] = box_width;
	argument_list[argument_count++] = box_height;
	return DSP_Send_Manual_Command(class,source,handle,CCD_DSP_TIM_BOARD_ID,CCD_DSP_SSS,argument_list,
				       argument_count,reply_value);
}

/**
 * Internal DSP command to come out of idle mode.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Idl
 * @see #DSP_Send_Command
 * @see ccd_dsp.html#CCD_DSP_TIM_BOARD_ID
 * @see ccd_dsp.html#CCD_DSP_STP
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_Stp(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value)
{
	return DSP_Send_Manual_Command(class,source,handle,CCD_DSP_TIM_BOARD_ID,CCD_DSP_STP,NULL,0,reply_value);
}

/**
 * Internal DSP command to abort the exposure that is currently underway.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Manual_Command
 * @see ccd_dsp.html#CCD_DSP_AEX
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_Aex(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value)
{
	return DSP_Send_Manual_Command(class,source,handle,CCD_DSP_TIM_BOARD_ID,CCD_DSP_AEX,NULL,0,reply_value);
}

/**
 * Internal DSP command to close the shutter.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Osh
 * @see #DSP_Send_Manual_Command
 * @see #CCD_DSP_CSH
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_Csh(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value)
{
	return DSP_Send_Manual_Command(class,source,handle,CCD_DSP_TIM_BOARD_ID,CCD_DSP_CSH,NULL,0,reply_value);
}

/**
 * Internal DSP command to open the shutter.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Csh
 * @see #DSP_Send_Manual_Command
 * @see #CCD_DSP_OSH
 */
static int DSP_Send_Osh(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value)
{
	return DSP_Send_Manual_Command(class,source,handle,CCD_DSP_TIM_BOARD_ID,CCD_DSP_OSH,NULL,0,reply_value);
}

/**
 * Internal DSP command to pause the exposure that is currently underway.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Rex
 * @see #DSP_Send_Manual_Command
 * @see #CCD_DSP_PEX
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_Pex(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value)
{
	return DSP_Send_Manual_Command(class,source,handle,CCD_DSP_TIM_BOARD_ID,CCD_DSP_PEX,NULL,0,reply_value);
}

/**
 * Internal DSP command to turn the analog power supplies on using the power control board.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Manual_Command
 * @see ccd_dsp.html#CCD_DSP_PON
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_Pon(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value)
{
	return DSP_Send_Manual_Command(class,source,handle,CCD_DSP_TIM_BOARD_ID,CCD_DSP_PON,NULL,0,reply_value);
}

/**
 * Internal DSP command to turn the analog power supplies off using the power control board.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Manual_Command
 * @see ccd_dsp.html#CCD_DSP_POF
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_Pof(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value)
{
	return DSP_Send_Manual_Command(class,source,handle,CCD_DSP_TIM_BOARD_ID,CCD_DSP_POF,NULL,0,reply_value);
}

/**
 * Internal DSP command to resume the exposure that is currently underway.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Pex
 * @see #DSP_Send_Command
 * @see #CCD_DSP_REX
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_Rex(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value)
{
	return DSP_Send_Manual_Command(class,source,handle,CCD_DSP_TIM_BOARD_ID,CCD_DSP_REX,NULL,0,reply_value);
}

/**
 * Internal DSP command to make the SDSU CCD Controller start an exposure.
 * If the tv_sec field of start_time is non-zero, we want to open the shutter as near as possible to the 
 * passed in time, allowing for some transmission delay (CCD_Exposure_Start_Exposure_Offset_Time).
 * Sets the Exposure_Start_Time to start of the exposure.
 * Sets the exposure status to either EXPOSING or READOUT (if the exposure length is small).
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param start_time The time to start the exposure. If the tv_sec field of the structure is zero,
 * 	we can start the exposure at any convenient time.
 * @param exposure_length The length of exposure we are about to start. This used in conjunction with
 * 	CCD_Exposure_Get_Readout_Remaining_Time, to see whether to change status to EXPOSING or READOUT.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Send_Manual_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_START_EXPOSURE
 * @see ccd_exposure.html#CCD_Exposure_Set_Exposure_Start_Time
 * @see ccd_exposure.html#CCD_Exposure_Set_Exposure_Status
 * @see ccd_exposure.html#CCD_Exposure_Get_Start_Exposure_Offset_Time
 * @see ccd_exposure.html#CCD_Exposure_Get_Readout_Remaining_Time
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_Sex(char *class,char *source,CCD_Interface_Handle_T* handle,
			struct timespec start_time,int exposure_length, int *reply_value)
{
	enum CCD_EXPOSURE_STATUS exposure_status;
	struct timespec current_time,sleep_time;
#ifndef _POSIX_TIMERS
	struct timeval gtod_current_time;
#endif
	int remaining_sec,remaining_ns,done = FALSE;

/* if a start time has been specified wait for it */
	if(start_time.tv_sec > 0)
	{
		exposure_status = CCD_EXPOSURE_STATUS_WAIT_START;
		if(!CCD_Exposure_Set_Exposure_Status(handle,exposure_status))
		{
			DSP_Error_Number = 35;
			sprintf(DSP_Error_String,"DSP_Send_Sex:Setting exposure status %d failed.",exposure_status);
			return FALSE;
		}
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
			remaining_sec = start_time.tv_sec - current_time.tv_sec;
		/* if we have over a second before start_time, sleep for a second. */
			if(remaining_sec > 1)
			{
				sleep_time.tv_sec = 1;
				sleep_time.tv_nsec = 0;
				nanosleep(&sleep_time,NULL);
			}
			else if(remaining_sec > -1)
			{
				remaining_ns = (start_time.tv_nsec - current_time.tv_nsec);
			/* we need to allow time for propogation of the SEX command
			** allow offset milliseconds to do this. */
				remaining_ns -= CCD_Exposure_Get_Start_Exposure_Offset_Time(handle)*
					CCD_GLOBAL_ONE_MILLISECOND_NS;
				if(remaining_ns < 0)
				{
					remaining_sec--;
					remaining_ns += CCD_GLOBBAL_ONE_SECOND_NS;
				}
				done = TRUE;
				if(remaining_sec > -1)
				{
					sleep_time.tv_sec = remaining_sec;
					sleep_time.tv_nsec = remaining_ns;
					nanosleep(&sleep_time,NULL);
				}
			}
			else
				done = TRUE;
		/* if an abort has occured, stop sleeping. */
			if(handle->DSP_Data.Abort)
			{
				DSP_Error_Number = 31;
				sprintf(DSP_Error_String,"DSP_Send_Sex:Abort detected whilst waiting for start time.");
				return FALSE;
			}
		}/* end while */
	}/* end if */
/* switch status to exposing and store the actual time the exposure is going to start */
/* If the exposure length is small, we go directly into READOUT status. */
	if(exposure_length < CCD_Exposure_Get_Readout_Remaining_Time(handle))
		exposure_status = CCD_EXPOSURE_STATUS_READOUT;
	else
		exposure_status = CCD_EXPOSURE_STATUS_EXPOSE;
	if(!CCD_Exposure_Set_Exposure_Status(handle,exposure_status))
	{
		DSP_Error_Number = 16;
		sprintf(DSP_Error_String,"DSP_Send_Sex:Setting exposure status %d failed.",exposure_status);
		return FALSE;
	}
	CCD_Exposure_Set_Exposure_Start_Time(handle);
/* start exposure and return result */
	return DSP_Send_Manual_Command(class,source,handle,CCD_DSP_TIM_BOARD_ID,CCD_DSP_SEX,NULL,0,reply_value);
}

/**
 * Internal DSP command to reset the SDSU controller board.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns true if sending the command succeeded, false if it failed. It does not return
 * 	SYR, read the reply value to get this value.
 * @see #DSP_Send_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_RESET_CONTROLLER
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_Reset(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value)
{
	return DSP_Send_Command(class,source,handle,CCD_PCI_HCVR_RESET_CONTROLLER,reply_value);
}

/**
 * Internal DSP command to read the controller config.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns TRUE if the command was sent without error, FALSE otherwise.
 * @see #DSP_Send_Manual_Command
 * @see #CCD_DSP_RCC
 */
static int DSP_Send_Rcc(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value)
{
	return DSP_Send_Manual_Command(class,source,handle,CCD_DSP_TIM_BOARD_ID,CCD_DSP_RCC,NULL,0,reply_value);
}

/**
 * Internal DSP command to ready for SDSU PCI board for DSP code download.
 * This uses CCD_Interface_Command to call the PCI ioctl CCD_PCI_IOCTL_PCI_DOWNLOAD with no arguments.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @return Returns TRUE if the command was sent without error, FALSE otherwise.
 * @see ccd_interface.html#CCD_Interface_Command
 * @see ccd_pci.html#CCD_PCI_IOCTL_PCI_DOWNLOAD
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_PCI_Download(char *class,char *source,CCD_Interface_Handle_T* handle)
{
	int value;

	value = 0;
#if LOGGING > 11
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"PCI_DOWNLOAD.");
#endif
	if(!CCD_Interface_Command(handle,CCD_PCI_IOCTL_PCI_DOWNLOAD,&value))
	{
		DSP_Error_Number = 109;
		sprintf(DSP_Error_String,"DSP_Send_PCI_Download:Sending PCI download failed.");
		return FALSE;
	}
	return TRUE;
}

/**
 * Internal DSP command to wait for the SDSU PCI board to complete itialisation after a DSP code download.
 * This uses CCD_Interface_Command to call the PCI ioctl CCD_PCI_IOCTL_PCI_DOWNLOAD_WAIT with no arguments.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns CCD_DSP_DON if the command was sent without error.
 * @see ccd_interface.html#CCD_Interface_Command
 * @see ccd_pci.html#CCD_PCI_IOCTL_PCI_DOWNLOAD_WAIT
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_PCI_Download_Wait(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value)
{
#if LOGGING > 11
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"PCI_DOWNLOAD_WAIT.");
#endif
	if(reply_value == NULL)
	{
		DSP_Error_Number = 22;
		sprintf(DSP_Error_String,"DSP_Send_PCI_Download_Wait:Reply value is NULL.");
		return FALSE;
	}
	return CCD_Interface_Command(handle,CCD_PCI_IOCTL_PCI_DOWNLOAD_WAIT,reply_value);
}

/**
 * Internal DSP command reset the PCI board's program counter. This uses DSP_Send_Command to set the HCVR to
 * CCD_PCI_HCVR_PCI_PC_RESET with no arguments.
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns TRUE if the command was sent without error, FALSE otherwise.
 * @see #DSP_Send_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_PCI_PC_RESET
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_PCI_PC_Reset(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value)
{
	return DSP_Send_Command(class,source,handle,CCD_PCI_HCVR_PCI_PC_RESET,reply_value);
}

/**
 * Internal DSP command to set exposure time. 
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param msecs The number of milliseconds to expose the CCD for.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns TRUE if the command was sent without error, FALSE otherwise.
 * @see ccd_dsp.html#CCD_DSP_SET
 * @see ccd_dsp.html#DSP_Send_Manual_Command
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_Set(char *class,char *source,CCD_Interface_Handle_T* handle,int msecs,int *reply_value)
{
	int argument_list[1];
	int argument_count = 0;

#if LOGGING > 11
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"SET_EXPTIME:value:%d.",msecs);
#endif
/* send command to interface */
	argument_list[argument_count++] = msecs;
	return DSP_Send_Manual_Command(class,source,handle,CCD_DSP_TIM_BOARD_ID,CCD_DSP_SET,argument_list,
				       argument_count,reply_value);
}

/**
 * Internal DSP command to get the time an exposure has been underway. 
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns TRUE if the command was sent without error, FALSE otherwise.
 * @see #DSP_Send_Manual_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_READ_EXPOSURE_TIME
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_Ret(char *class,char *source,CCD_Interface_Handle_T* handle,int *reply_value)
{
#if LOGGING > 11
	CCD_Global_Log(class,source,LOG_VERBOSITY_VERBOSE,"READ_EXPOSURE_TIME.");
#endif
	return DSP_Send_Manual_Command(class,source,handle,CCD_DSP_TIM_BOARD_ID,CCD_DSP_RET,NULL,0,reply_value);
}

/**
 * Internal DSP command that sends a manual command the SDSU CCD Controller. A manual command is a 24 bit
 * 3 letter command (e.g. SGN) that a sent via the PCI interface to one of the controller boards. 
 * <ul>
 * <li>The argument list is copied into a local copy.
 * <li>The command is sent using CCD_Interface_Command_List (CCD_PCI_IOCTL_COMMAND).
 * <li>The reply values are extracted from the local argument list and copied back to the passed in list.
 * </ul>
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param board_id Which SDSU board to send the manual command to. One of the ID's in the CCD_DSP_BOARD_ID
 * 	enumeration.
 * @param command The command number to put in the CMDR(Manual Command Register) register.
 * @param argument_list The list of arguments to be sent to the controller.
 * @param argument_count The number of arguments in the argument_list.
 * @param reply_value The address of an integer to store the reply value returned from the SDSU board.
 * @return Returns true if no error occurs. If the command fails returns false.
 * @see ccd_interface.html#CCD_Interface_Command_List
 * @see ccd_pci.html#CCD_PCI_IOCTL_COMMAND
 * @see #DSP_Send_Command
 * @see #DSP_Manual_Command_To_String
 * @see #CCD_DSP_BOARD_ID
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_Manual_Command(char *class,char *source,CCD_Interface_Handle_T* handle,
				   enum CCD_DSP_BOARD_ID board_id,int command,int *argument_list,int argument_count,
				   int *reply_value)
{
	int ioctl_argument_list[CCD_PCI_IOCTL_ARGUMENT_LIST_COUNT];
	int header,i;

	if(reply_value == NULL)
	{
		DSP_Error_Number = 23;
		sprintf(DSP_Error_String,"DSP_Send_Manual_Command: Reply Value was NULL.");
		return FALSE;
	}
/* Setup header word. The second byte contains the board_id. The least significant byte contains
** the number of arguments in the command, which is the number of arguments sent to the routine 
** plus two (the header word itself, and the command word). */
	header = ((board_id << 8) | (argument_count+2));
	ioctl_argument_list[0] = header;
	ioctl_argument_list[1] = command;
/* put the argument words into data memory */
	for(i = 0;i < argument_count;i++)
	{
#if LOGGING > 11
		CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"SET_ARG:index:%d:value:%#x.",i,
				      argument_list[i]);
#endif
		ioctl_argument_list[i+2] = argument_list[i];
	}
/* Set all unused elements in the ioctl_argument_list to -1.
** We start from argument_count+2, which was the last element in the array to be filled in.
** The first two elements are the header and the command itself. */
	for(i = argument_count+2;i < CCD_PCI_IOCTL_ARGUMENT_LIST_COUNT;i++)
	{
		ioctl_argument_list[i] = -1;
	}
/* send the command to device driver */
#if LOGGING > 11
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"COMMAND:value:%s (%#x).",
			      DSP_Manual_Command_To_String(command),command);
#endif
	if(!CCD_Interface_Command_List(handle,CCD_PCI_IOCTL_COMMAND,ioctl_argument_list,argument_count+2))
	{
		DSP_Error_Number = 33;
		sprintf(DSP_Error_String,"DSP_Send_Manual_Command:Sending command %s (%#x) failed.",
			DSP_Manual_Command_To_String(command),command);
		return FALSE;
	}
/* copy reply_value from ioctl_argument_list[0] to reply_value */
	(*reply_value) = ioctl_argument_list[0];
	return TRUE;
}

/**
 * Internal DSP command that sends a command to the SDSU CCD Controller. The command is a PCI interface type
 * command.
 * <ul>
 * <li>The command is sent using CCD_Interface_Command (CCD_PCI_IOCTL_SET_HCVR).
 * </ul>
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @param hcvr_command The command number to put in the HCVR(Host Command Vector Register) register.
 * @param reply_value The address of an integer to store the value returned from the SDSU board.
 * @return Returns true if no error occurs. If the command fails returns false.
 * @see ccd_interface.html#CCD_Interface_Command
 * @see ccd_interface.html#CCD_Interface_Handle_T
 */
static int DSP_Send_Command(char *class,char *source,CCD_Interface_Handle_T* handle,int hcvr_command,int *reply_value)
{
	if(reply_value == NULL)
	{
		DSP_Error_Number = 24;
		sprintf(DSP_Error_String,"DSP_Send_Command: Reply Value was NULL.");
		return FALSE;
	}
/* send the command to device driver */
#if LOGGING > 11
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"SET_HCVR:value:%#x.",hcvr_command);
#endif
/* set reply_value to hcvr_command, this is the data value passed into CCD_Interface_Command */
	(*reply_value) = hcvr_command;
	if(!CCD_Interface_Command(handle,CCD_PCI_IOCTL_SET_HCVR,reply_value))
	{
		DSP_Error_Number = 36;
		sprintf(DSP_Error_String,"DSP_Send_Command:Sending command %#x failed.",(*reply_value));
		return FALSE;
	}
/* reply_value was set in CCD_Interface_Command */
	return TRUE;
}

/**
 * This routine checks a reply word from the SDSU CCD Controller. It checks that the reply is the expected_reply 
 * (unless expected_reply is DSP_ACTUAL_VALUE, in which case the reply is a value.
 * If a timeout (CCD_DSP_TOUT) or error (CCD_DSP_ERR) occurs an error is returned. 
 * @param class The class parameter to use for any log messages associated with this operation.
 * @param source The class parameter to use for any log messages associated with this operation.
 * @param expected_reply What the reply should be. Normally it should be CCD_DSP_DON, if an error
 *	occurs the CCD Controller will probably return CCD_DSP_ERR and an error is returned. If the
 * 	special value DSP_ACTUAL_VALUE is passed in no reply chacking is
 * 	performed (for instance, when the reply is a memory value from a RDM command).
 * @return Returns the expected reply value when that value is actually returned. If an error occurs FALSE is
 * 	returned. If an actual value was requested that is returned.
 * @see #CCD_DSP_ERR
 * @see #CCD_DSP_DON
 * @see #CCD_DSP_TOUT
 * @see #DSP_ACTUAL_VALUE
 */
static int DSP_Check_Reply(char *class,char *source,int reply,int expected_reply)
{

#if LOGGING > 11
	CCD_Global_Log_Format(class,source,LOG_VERBOSITY_VERBOSE,"CHECK_REPLY:actual:%#x,expected:%#x.",
			      reply,expected_reply);
#endif
/* If the reply was ERR something went wrong with the last command */
	if(reply == CCD_DSP_ERR)
	{
		DSP_Error_Number = 110;
		sprintf(DSP_Error_String,"DSP_Check_Reply:Reply was ERR.");
		return FALSE;
 	}
/* If the reply was TOUT the device driver did not receive a reply for the last command */
	if(reply == CCD_DSP_TOUT)
	{
		DSP_Error_Number = 111;
		sprintf(DSP_Error_String,"DSP_Check_Reply:Reply was TOUT.");
		return FALSE;
 	}
/* If the expected reply was an actual value we can't test whether this is correct or not
** so just return it. We do this first as a RDM may return the value for ERR correctly etc... */
	if(expected_reply == DSP_ACTUAL_VALUE)
	{
		return reply;
	}
/* check reply vs. expected */
	if(reply != expected_reply)
	{
		DSP_Error_Number = 112;
		sprintf(DSP_Error_String,"DSP_Check_Reply:Unexpected Reply(%#x,%#x).",
			reply,expected_reply);
		return FALSE;
	}
	return reply;
}


#ifdef CCD_DSP_MUTEXED
/**
 * Routine to lock the controller access mutex. This will block until the mutex has been acquired,
 * unless an error occurs.
 * Mutex locking is currently acheived against one statically initialised mutex in ccd_dsp. 
 * I have tried this with one mutex per PCI card in the CCD_DSP_Struct (as part of the handle data), 
 * but this causes the control computer to lock.
 * Although it should theoretically work (as the data paths are parallel), I suspect the locking in the v1.7
 * linux PCI card device driver cannot cope - certainly the software (device driver) locking has changed compared to 
 * v2.0, and installing that may fix the problem and allow simultaneous DSP commands to both SDSU PCI cards.
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 *       This isn't used anymore, but is left in in case I fix the device driver.
 * @return Returns TRUE if the mutex has been  locked for access by this thread,
 * 	FALSE if an error occured.
 */
static int DSP_Mutex_Lock(CCD_Interface_Handle_T* handle)
{
	int error_number;

	error_number = pthread_mutex_lock(&(DSP_Data.Mutex));
	if(error_number != 0)
	{
		DSP_Error_Number = 18;
		sprintf(DSP_Error_String,"DSP_Mutex_Lock:Mutex lock failed '%d'.",error_number);
		return FALSE;
	}
	return TRUE;
}

/**
 * Routine to unlock the controller access mutex. 
 * @param handle The address of a CCD_Interface_Handle_T that holds the device connection specific information.
 * @return Returns TRUE if the mutex has been unlocked, FALSE if an error occured.
 * @see #DSP_Data
 */
static int DSP_Mutex_Unlock(CCD_Interface_Handle_T* handle)
{
	int error_number;

	error_number = pthread_mutex_unlock(&(DSP_Data.Mutex));
	if(error_number != 0)
	{
		DSP_Error_Number = 20;
		sprintf(DSP_Error_String,"DSP_Mutex_Unlock:Mutex unlock failed '%d'.",error_number);
		return FALSE;
	}
	return TRUE;
}
#endif

/**
 * Internal routine to translate a manual command number to a string three letter command name.
 * @param manual_command The command to translate.
 * @return A pointer to a string containing the command string. The pointer points to some static memory
 * 	in the function, this value will be overwritten by the next call to this routine. However, the
 * 	returned memory does <b>NOT</b> have to be freed. The string is always 3 characters long, plus a NULL
 * 	terminator.
 */
static char *DSP_Manual_Command_To_String(int manual_command)
{
	static char command_string[4];

	command_string[0] = (char)((manual_command >> 16)&0xFF);
	command_string[1] = (char)((manual_command >>  8)&0xFF);
	command_string[2] = (char)( manual_command &      0xFF);
	command_string[3] = '\0';
	return command_string;
}

/*
** $Log: not supported by cvs2svn $
** Revision 0.55  2009/05/05 10:42:04  cjm
** Removed mutex from per-handle data and made global to ccd_dsp.
** The v1.7 device driver cannot cope with 2 simultaneous DSP commands, although the
** PCI cards downwards presumably can. Probably actually a device driver error.
**
** Revision 0.54  2009/04/30 14:16:35  cjm
** Removed DSP_Data. This data is now per-handle.
** Rewrote mutex and abort routines to take handles.
** Added CCD_DSP_Data_Initialise to initialise handle DSP data.
**
** Revision 0.53  2009/02/05 11:40:27  cjm
** Swapped Bitwise for Absolute logging levels.
**
** Revision 0.52  2008/12/11 18:14:38  cjm
** Fixes to logging changes.
**
** Revision 0.51  2008/12/11 18:13:24  cjm
** Logging changes.
**
** Revision 0.50  2008/11/20 11:34:46  cjm
** *** empty log message ***
**
** Revision 0.49  2006/05/16 14:12:17  cjm
** gnuify: Added GNU General Public License.
**
** Revision 0.48  2004/05/16 14:28:18  cjm
** Re-wrote abort code.
**
** Revision 0.47  2003/12/08 15:04:10  cjm
** CCD_EXPOSURE_STATUS_WAIT_START added.
**
** Revision 0.46  2003/06/09 11:30:39  cjm
** Added exposure status checking to VON/VOF, to stop lock-ups in readout.
**
** Revision 0.45  2003/06/06 12:36:01  cjm
** Added VON/VOF.
**
** Revision 0.44  2003/03/26 15:44:48  cjm
** Added SSS and SSP implementations.
**
** Revision 0.43  2002/12/16 16:49:36  cjm
** Fixed status problems.
** Removed Error routines resetting error number to zero.
**
** Revision 0.42  2002/12/03 17:13:19  cjm
** Added DSP_Manual_Command_To_String routine, for better error messages.
** Made a note of error codes that are used in the Java layer, to stop
** the error log filling with Exposure status in READOUT errors
** (which are not serious to the Ccs).
**
** Revision 0.41  2002/11/08 12:13:19  cjm
** CCD_DSP_Command_SEX now changes the exposure status to READOUT immediately,
** if the exposure length is small enough.
**
** Revision 0.40  2002/11/07 19:13:39  cjm
** Changes to make library work with SDSU version 1.7 DSP code.
**
** Revision 0.39  2001/07/11 10:08:21  cjm
** Added MJD FITS keyword to exposure save routine.
**
** Revision 0.38  2001/06/04 14:36:17  cjm
** Changed DEBUG to LOGGING.
** Added Increase Priority and memory locking conditional compilation code.
**
** Revision 0.37  2001/04/17 10:01:00  cjm
** Added logging calls.
**
** Revision 0.36  2001/03/12 15:15:41  cjm
** Added CCD_DSP_UTIL_EXPOSURE_CHECK conditional compilation flag.
** When set, this only allows communication with the timing board when we are not exposing.
** This is because doing this operation used to lock the SDSU controller (now fixed in DSP code).
**
** Revision 0.35  2001/02/16 09:55:19  cjm
** Added more detail to error messages.
**
** Revision 0.34  2001/02/09 18:30:40  cjm
** Changed Filter wheel code - to just simply send command, there is now no
** second DON message returned from the controller.
**
** Revision 0.33  2001/02/05 17:04:11  cjm
** Fixed return value in CCD_DSP_Command_RDC.
**
** Revision 0.32  2001/01/31 16:35:19  cjm
** Added tests for filename is NULL in DSP download code.
**
** Revision 0.31  2001/01/23 18:20:59  cjm
** Added check for maximum exposure length CCD_DSP_EXPOSURE_MAX_LENGTH.
**
** Revision 0.30  2000/12/21 11:48:02  cjm
** Changed mutexing support on FWM/FWR/FWA so that getting status does not mix up
** filter wheel replies.
**
** Revision 0.29  2000/12/19 17:52:47  cjm
** New filter wheel code.
**
** Revision 0.28  2000/09/25 09:51:28  cjm
** Changes to use with v1.4 SDSU DSP code.
**
** Revision 0.27  2000/08/11 13:55:50  cjm
** Removed extraneous error code setting in CCD_DSP_Command_SEX if
** CCD_DSP_Command_RDI fails: it already sets up the error code.
**
** Revision 0.26  2000/07/14 16:25:44  cjm
** Backup.
**
** Revision 0.25  2000/07/11 10:41:11  cjm
** Fixed Exposure Status errors.
** Added read PCI status routines.
**
** Revision 0.24  2000/06/20 12:53:07  cjm
** CCD_DSP_Command_Sex now automatically calls CCD_DSP_Command_RDI.
**
** Revision 0.23  2000/06/19 08:48:34  cjm
** Backup.
**
** Revision 0.22  2000/06/14 13:29:27  cjm
** Further DSP_DeInterlace enhancements - don't allocate new_iptr unless it's
** nessassary.
**
** Revision 0.21  2000/06/14 12:58:35  cjm
** Fixed DSP_DeInterlace memory error.
**
** Revision 0.20  2000/06/13 17:14:13  cjm
** Changes to make Ccs agree with voodoo.
**
** Revision 0.19  2000/05/23 10:33:44  cjm
** Added CCD_DSP_Set_Exposure_Start_Time and changed DSP_Send_Sex to use it.
**
** Revision 0.18  2000/05/22 16:28:12  cjm
** DSP_Save now updates DATE,DATE-OBS and UTSTART keywords.
**
** Revision 0.17  2000/05/10 16:51:02  cjm
** Added DSP_Byte_Swap.
** Tidied RDC command (numbytes).
**
** Revision 0.16  2000/05/10 11:06:49  cjm
** Removed perror.
**
** Revision 0.15  2000/04/13 13:19:52  cjm
** Added current time to error string.
**
** Revision 0.14  2000/03/21 16:37:14  cjm
** Added mutex compilation print.
**
** Revision 0.13  2000/03/20 11:45:17  cjm
** Added _POSIX_TIMERS feature test macros around calls to clock_gettime, to allow the
** library to compile under Linux. Note nanosleep should also be tested, but this exists under
** Linux so it is not a problem.
**
** Revision 0.12  2000/03/20 10:41:30  cjm
** Replaced cftime with gmtime/strftime for Linux compatibility.
** Also tidied up some unused variables.
**
** Revision 0.11  2000/03/09 16:09:28  cjm
** Added CCD_DSP_Command_Read_Exposure_Time.
**
** Revision 0.10  2000/03/09 15:01:25  cjm
** Initial mutex implementation.
**
** Revision 0.9  2000/03/02 09:48:11  cjm
** Moved Exposure_Status EXPOSE to after wait for start_time of exposure.
**
** Revision 0.8  2000/03/01 15:31:36  cjm
** Added exposure timing code.
**
** Revision 0.7  2000/02/28 19:13:01  cjm
** Backup.
**
** Revision 0.6  2000/02/23 11:56:16  cjm
** Removed CCD_DSP_Command_WRM_No_Reply.
**
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

/* ccd_dsp.c -*- mode: Fundamental;-*-
** ccd library
** $Header: /home/cjm/cvs/frodospec/ccd/c/ccd_dsp.c,v 0.19 2000-05-23 10:33:44 cjm Exp $
*/
/**
 * ccd_dsp.c contains all the SDSU CCD Controller commands. Commands are passed to the 
 * controller using the <a href="ccd_interface.html">CCD_Interface_</a> calls.
 * @author SDSU, Chris Mottram
 * @version $Revision: 0.19 $
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
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#ifndef _POSIX_TIMERS
#include <sys/time.h>
#endif
#ifdef CCD_DSP_MUTEXED
#include <pthread.h>
#endif
#include "ccd_interface.h"
#include "ccd_pci.h"
#include "ccd_dsp.h"
#ifdef CFITSIO
#include "fitsio.h"
#endif

/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id: ccd_dsp.c,v 0.19 2000-05-23 10:33:44 cjm Exp $";

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
 * The number of nanoseconds in one second. A struct timespec has fields in nanoseconds.
 */
#define DSP_ONE_SECOND_NS		(1000000000)
/**
 * The number of nanoseconds in one millisecond. A struct timespec has fields in nanoseconds.
 */
#define DSP_ONE_MILLISECOND_NS		(1000000)
/**
 * The number of milliseconds in one second.
 */
#define DSP_ONE_SECOND_MS		(1000)
/**
 * The number of nanoseconds in one microsecond.
 */
#define DSP_ONE_MICROSECOND_NS		(1000)

/**
 * The default amount of time before we are due to start an exposure, that a CLEAR_ARRAY command should be sent to
 * the controller. This time is in seconds, and must be greater than the time the CLEAR_ARRAY command takes to
 * clock all accumulated charge off the CCD (approx 5 seconds for a 2kx2k EEV42-40).
 */
#define DSP_DEFAULT_START_EXPOSURE_CLEAR_TIME	(10)
/**
 * The default amount of time, in milliseconds, before the desired start of exposure that we should send the
 * START_EXPOSURE command, to allow for transmission delay.
 */
#define DSP_DEFAULT_START_EXPOSURE_OFFSET_TIME	(2)
/**
 * The default amount of time, in milleseconds, remaining for an exposure when we stop sleeping and tell the
 * interface to enter readout mode. Note, because the exposure time is read every second, it is best
 * not have have this constant an exact multiple of 1000.
 */
#define DSP_DEFAULT_READOUT_REMAINING_TIME	(1100)

/* structure */
/**
 * Structure used to hold local data to ccd_dsp.
 * <dl>
 * <dt>Abort</dt> <dd>Whether it has been requested to abort the current operation.</dd>
 * <dt>Exposure_Status</dt> <dd>Whether an operation is being performed to CLEAR, EXPOSE or READOUT the CCD.</dd>
 * <dt>Mutex</dt> <dd>Optionally compiled mutex locking for sending commands and getting replies from the 
 * 	controller.</dd>
 * <dt>Start_Exposure_Clear_Time</dt> <dd>The amount of time before we are due to start an exposure, 
 * 	that a CLEAR_ARRAY command should be sent to the controller. This time is in seconds, 
 * 	and must be greater than the time the CLEAR_ARRAY command takes to clock all accumulated charge off the CCD 
 * 	(approx 5 seconds for a 2kx2k EEV42-40).</dd>
 * <dt>Start_Exposure_Offset_Time</dt> <dd>The amount of time, in milliseconds, before the desired start of 
 * 	exposure that we should send the START_EXPOSURE command, to allow for transmission delay.</dd>
 * <dt>Readout_Remaining_Time</dt> <dd>Amount of time, in milleseconds,
 * 	remaining for an exposure when we stop sleeping and tell the interface to enter readout mode.</dd>
 * <dt>Exposure_Length</dt> <dd>The last exposure length to be set.</dd>
 * <dt>Exposure_Start_Time</dt> <dd>The time stamp when the START_EXPOSURE command was sent to the controller.</dd>
 * </dl>
 */
struct DSP_Attr_Struct
{
	volatile int Abort; /* This is volatile as a different thread may change this variable. */
	enum CCD_DSP_EXPOSURE_STATUS Exposure_Status;
#ifdef CCD_DSP_MUTEXED
	pthread_mutex_t Mutex;
#endif
	int Start_Exposure_Clear_Time;
	int Start_Exposure_Offset_Time;
	int Readout_Remaining_Time;
	int Exposure_Length;
	struct timespec Exposure_Start_Time;
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
 * <dt>Abort</dt> <dd>FALSE</dd>
 * <dt>Exposure_Status</dt> <dd>CCD_DSP_EXPOSURE_STATUS_NONE</dd>
 * <dt>Mutex</dt> <dd>PTHREAD_MUTEX_INITIALIZER</dd>
 * <dt>Start_Exposure_Clear_Time</dt> <dd>DSP_DEFAULT_START_EXPOSURE_CLEAR_TIME</dd>
 * <dt>Start_Exposure_Offset_Time</dt> <dd>DSP_DEFAULT_START_EXPOSURE_OFFSET_TIME</dd>
 * <dt>Readout_Remaining_Time</dt> <dd>DSP_DEFAULT_READOUT_REMAINING_TIME</dd>
 * <dt>Exposure_Length</dt> <dd>0</dd>
 * <dt>Exposure_Start_Time</dt> <dd>{0L,0L}</dd>
 * </dl>
 * @see #DSP_Attr_Struct
 * @see #CCD_DSP_EXPOSURE_STATUS
 * @see #DSP_DEFAULT_START_EXPOSURE_CLEAR_TIME
 * @see #DSP_DEFAULT_START_EXPOSURE_OFFSET_TIME
 * @see #DSP_DEFAULT_READOUT_REMAINING_TIME
 */
static struct DSP_Attr_Struct DSP_Data = 
{
	FALSE,
	CCD_DSP_EXPOSURE_STATUS_NONE,
	PTHREAD_MUTEX_INITIALIZER,
	DSP_DEFAULT_START_EXPOSURE_CLEAR_TIME,
	DSP_DEFAULT_START_EXPOSURE_OFFSET_TIME,
	DSP_DEFAULT_READOUT_REMAINING_TIME,
	0,
	{0L,0L}
};

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
static int DSP_Send_Sex(struct timespec start_time);
static int DSP_Send_Reset(void);
static int DSP_Send_Set_Util_Options(int bit_value);
static int DSP_Send_Set_Exposure_Time(int msecs);
static int DSP_Send_Read_Exposure_Time(void);

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
static int DSP_Image_Transfer(int ncols,int nrows,enum CCD_DSP_DEINTERLACE_TYPE deinterlace_type,char *filename);
/* we should provide an alternative for these two routines if the library is not using short ints. */
#if CCD_GLOBAL_BYTES_PER_PIXEL == 2
static void DSP_Byte_Swap(unsigned short *svalues,long nvals);
static int DSP_DeInterlace(int ncols,int nrows,unsigned short *old_iptr,
	enum CCD_DSP_DEINTERLACE_TYPE deinterlace_type);
#else
#error CCD_GLOBAL_BYTES_PER_PIXEL uses illegal value.
#endif
static int DSP_Save(char *filename,unsigned short *exposure_data,int ncols,int nrows);
static void DSP_TimeSpec_To_Date_String(struct timespec time,char *time_string);
static void DSP_TimeSpec_To_Date_Obs_String(struct timespec time,char *time_string);
static void DSP_TimeSpec_To_UtStart_String(struct timespec time,char *time_string);
#ifdef CCD_DSP_MUTEXED
static int DSP_Mutex_Lock(void);
static int DSP_Mutex_Unlock(void);
#endif

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
	DSP_Data.Abort = FALSE;
	DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_NONE;
/* don't intialise the mutex here, it is statically initialised, once only */
	DSP_Data.Start_Exposure_Clear_Time = DSP_DEFAULT_START_EXPOSURE_CLEAR_TIME;
	DSP_Data.Start_Exposure_Offset_Time = DSP_DEFAULT_START_EXPOSURE_OFFSET_TIME;
	DSP_Data.Readout_Remaining_Time = DSP_DEFAULT_READOUT_REMAINING_TIME;
	DSP_Data.Exposure_Length = 0;
	DSP_Data.Exposure_Start_Time.tv_sec = 0;
	DSP_Data.Exposure_Start_Time.tv_nsec = 0;
/* print some compile time option information to stdout */
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
	return TRUE;
}

/* Boot commands */
/**
 * This routine executes the LoaD Application (LDA) command on a 
 * SDSU Controller board. This
 * causes some DSP application code to be loaded from (EEP)ROM into DSP memory for the controller to execute.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
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
	int retval;

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
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock())
		return FALSE;
#endif
	if(!DSP_Send_Lda(board_id,application_number))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
	/* get reply - should be DON */
	retval = DSP_Get_Reply(CCD_DSP_DON);
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock())
		return FALSE;
#endif
	return retval;
}

/**
 * This routine executes the ReaD Memory (RDM) command on a SDSU Controller board. 
 * This
 * gets the value of a word of memory, location specified by board,memory space and address, and returns
 * it's value.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
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
	int retval;

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
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock())
		return FALSE;
#endif
	if(!DSP_Send_Rdm(board_id,mem_space,address))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
	/* get reply - actual value of memory location returned */
	retval = DSP_Get_Reply(DSP_ACTUAL_VALUE);
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock())
		return FALSE;
#endif
	return retval;
}

/**
 * This routine Tests the Data Link (TDL) on a SDSU Controller board. 
 * This ensures the host computer can communicate with the board.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
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
	if(!DSP_Mutex_Lock())
		return FALSE;
#endif
	if(!DSP_Send_Tdl(board_id,data))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
	/* get reply - data value sent should be returned */
	retval = DSP_Get_Reply(data);
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock())
		return FALSE;
#endif
	return retval;
}

/**
 * This routine executes the WRite Memory (WRM) command on a SDSU Controller board.
 * This sets the value of a word of memory, it's location specified by board,memory space and address.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
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
	int retval;

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
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock())
		return FALSE;
#endif
	if(!DSP_Send_Wrm(board_id,mem_space,address,data))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
	/* get reply - DON should be returned */
	retval = DSP_Get_Reply(CCD_DSP_DON);
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock())
		return FALSE;
#endif
	return retval;
}

/* timing board commands */
/**
 * This routine executes the ABort Readout (ABR) command on a SDSU Controller board.
 * If the SDSU CCD Controller is currently reading out the CCD, it is stopped immediately.
 * Unlike most commands, this one does not wait for a DON message to be returned from the controller,
 * as the interface will be returning data when this request is made.
 * This routine is not mutexed, the Abort Readout command should be sent whilst a read is underway from the 
 * controller.
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
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Clr
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_CLR(void)
{
	int retval;

	DSP_Error_Number = 0;
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock())
		return FALSE;
#endif
	DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_CLEAR;
	if(!DSP_Send_Clr())
	{
		DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_NONE;
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
	/* get reply - DON should be returned */
	retval = DSP_Get_Reply(CCD_DSP_DON);
	DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_NONE;
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock())
		return FALSE;
#endif
	return retval;
}

/**
 * This routine executes the IDLe (IDL) command on a SDSU Controller board. This
 * puts the clocks in the readout sequence, but does not transfer the clocked data to prevent charge from 
 * building up on the CCD.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #CCD_DSP_Command_STP
 * @see #DSP_Send_Idl
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_IDL(void)
{
	int retval;

	DSP_Error_Number = 0;
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock())
		return FALSE;
#endif
	if(!DSP_Send_Idl())
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
	/* get reply - DON should be returned */
	retval = DSP_Get_Reply(CCD_DSP_DON);
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock())
		return FALSE;
#endif
	return retval;
}

/**
 * This routine executes the ReaDout Ccd (RDC) command on a SDSU Controller board. 
 * This reads out the data on the CCD.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param ncols The number of columns in the image. This must be a positive non-zero integer.
 * @param nrows The number of rows in the image to be readout from the CCD. This must be a positive non-zero integer.
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
int CCD_DSP_Command_RDC(int ncols,int nrows,enum CCD_DSP_DEINTERLACE_TYPE deinterlace_type,char *filename)
{
	int retval;

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
	if(!CCD_DSP_IS_DEINTERLACE_TYPE(deinterlace_type))
	{
		DSP_Error_Number = 13;
		sprintf(DSP_Error_String,"CCD_DSP_Command_RDC:Illegal deinterlace type '%d'.",deinterlace_type);
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock())
		return FALSE;
#endif
	if(!DSP_Send_Rdi())
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
	/* DSP_Image_Transfer saves the exposed image into a filename */
	if(!DSP_Image_Transfer(ncols,nrows,deinterlace_type,filename))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
	/* get reply - DON should be returned */
	retval = DSP_Get_Reply(CCD_DSP_DON);
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock())
		return FALSE;
#endif
	return retval;
}

/**
 * This routine executes the Set Bias Voltage (SBV) command on a 
 * SDSU Controller board. This
 * sets the voltage of the video processor DC bias and clock driver DACs from information in DSP memory.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Sbv
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_SBV(void)
{
	int retval;

	DSP_Error_Number = 0;
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock())
		return FALSE;
#endif
	if(!DSP_Send_Sbv())
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
	/* get reply - DON should be returned */
	retval = DSP_Get_Reply(CCD_DSP_DON);
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock())
		return FALSE;
#endif
	return retval;
}

/**
 * SGN command routine. Timing board command that means Set GaiN. 
 * This sets the gains of all the video processors.
 * The integrator speed is also set using this command to slow or fast.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
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
	int retval;

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
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock())
		return FALSE;
#endif
	if(!DSP_Send_Sgn(gain,speed))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
	/* get reply - DON should be returned */
	retval = DSP_Get_Reply(CCD_DSP_DON);
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock())
		return FALSE;
#endif
	return retval;
}

/**
 * This routine executes the SToP (STP) command on the timing board. This
 * stops the clocks clocking the readout sequence.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #CCD_DSP_Command_IDL
 * @see #DSP_Send_Stp
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_STP(void)
{
	int retval;

	DSP_Error_Number = 0;
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock())
		return FALSE;
#endif
	if(!DSP_Send_Stp())
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
	/* get reply - DON should be returned */
	retval = DSP_Get_Reply(CCD_DSP_DON);
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock())
		return FALSE;
#endif
	return retval;
}

/**
 * This routine sets the number of Columns the SDSU Controller will read out. This is sent to the PCI
 * interface which stores it in the timing table.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param ncols The number of columns to read out, after binning has been taken into account.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Set_NCols
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_Set_NCols(int ncols)
{
	int retval;

	DSP_Error_Number = 0;
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock())
		return FALSE;
#endif
	if(!DSP_Send_Set_NCols(ncols))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
	/* get reply - DON should be returned */
	retval = DSP_Get_Reply(CCD_DSP_DON);
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock())
		return FALSE;
#endif
	return retval;
}

/**
 * This routine sets the number of Rows the SDSU Controller will read out. This is sent to the PCI
 * interface which stores it in the timing table.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param nrows The number of rows to read out, after binning has been taken into account.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Set_NRows
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_Set_NRows(int nrows)
{
	int retval;

	DSP_Error_Number = 0;
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock())
		return FALSE;
#endif
	if(!DSP_Send_Set_NRows(nrows))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
	/* get reply - DON should be returned */
	retval = DSP_Get_Reply(CCD_DSP_DON);
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock())
		return FALSE;
#endif
	return retval;
}

/* utility board commands */
/**
 * This routine executes the Abort EXposure (AEX) command on a 
 * SDSU utility board. If an
 * exposure is currently underway this is stopped by closing the shutter and putting the CCD in idle mode.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Aex
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_AEX(void)
{
	int retval;

	DSP_Error_Number = 0;
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock())
		return FALSE;
#endif
	if(!DSP_Send_Aex())
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
	/* get reply - DON should be returned */
	retval = DSP_Get_Reply(CCD_DSP_DON);
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock())
		return FALSE;
#endif
	return retval;
}

/**
 * This routine executes the Close SHutter (CSH) command on the SDSU utility board.
 * This closes the shutter.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Csh
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_CSH(void)
{
	int retval;

	DSP_Error_Number = 0;
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock())
		return FALSE;
#endif
	if(!DSP_Send_Csh())
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
	/* get reply - DON should be returned */
	retval = DSP_Get_Reply(CCD_DSP_DON);
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock())
		return FALSE;
#endif
	return retval;
}

/**
 * This routine executes the Open SHutter (OSH) command on the SDSU utility board.
 * This opens the shutter.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Osh
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_OSH(void)
{
	int retval;

	DSP_Error_Number = 0;
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock())
		return FALSE;
#endif
	if(!DSP_Send_Osh())
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
	/* get reply - DON should be returned */
	retval = DSP_Get_Reply(CCD_DSP_DON);
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock())
		return FALSE;
#endif
	return retval;
}

/**
 * This routine executes the Pause EXposure (PEX) command on the
 * SDSU utility board. This closes the shutter and stops the timer.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #CCD_DSP_Command_REX
 * @see #DSP_Send_Pex
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_PEX(void)
{
	int retval;

	DSP_Error_Number = 0;
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock())
		return FALSE;
#endif
	if(!DSP_Send_Pex())
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
	/* get reply - DON should be returned */
	retval = DSP_Get_Reply(CCD_DSP_DON);
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock())
		return FALSE;
#endif
	return retval;
}

/**
 * This routine executes the Power ON (PON) command on a SDSU Controller board.
 * This turns the analog power on safely, using the power control board.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Pon
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_PON(void)
{
	int retval;

	DSP_Error_Number = 0;
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock())
		return FALSE;
#endif
	if(!DSP_Send_Pon())
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
	/* get reply - DON should be returned */
	retval = DSP_Get_Reply(CCD_DSP_DON);
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock())
		return FALSE;
#endif
	return retval;
}

/**
 * This routine executes the Power OFF (POF) command on a SDSU Controller board.
 * This turns the analog power off safely, using the power control board.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Pof
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_POF(void)
{
	int retval;

	DSP_Error_Number = 0;
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock())
		return FALSE;
#endif
	if(!DSP_Send_Pof())
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
	/* get reply - DON should be returned */
	retval = DSP_Get_Reply(CCD_DSP_DON);
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock())
		return FALSE;
#endif
	return retval;
}

/**
 * Routine to read the array temperature from the utility board.
 * This calls the PCI interfaces read temperature command. This in turn calls the utility board with
 * read memory command for address 0x0c in Y memory space. The reply memory is then read.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @return The adu counts held on the utility board. If an error occurs, zero is returned. Use 
 * 	CCD_DSP_Get_Error_Number to determine whether zero is returned from the board or is an error.
 * @see #DSP_Send_Read_Temperature
 * @see #DSP_Get_Reply
 * @see #DSP_ACTUAL_VALUE
 */
int CCD_DSP_Command_Read_Temperature(void)
{
	int retval;

	DSP_Error_Number = 0;
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock())
		return FALSE;
#endif
	if(!DSP_Send_Read_Temperature())
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
	/* get reply - the array temperature adu should be returned */
	retval = DSP_Get_Reply(DSP_ACTUAL_VALUE);
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock())
		return FALSE;
#endif
	return retval;
}

/**
 * Routine to set the target array temperature on the utility board.
 * This calls the PCI interfaces set temperature command. This in turn calls the utility board with
 * write memory command for address 0x1c in Y memory space. The reply memory is then read to ensure DON is returned.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param The target adu counts for the desired temperature.
 * @return The routine returnd DON if the command suceeded, FALSE if it failed.
 * @see #DSP_Send_Set_Temperature
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_Set_Temperature(int adu)
{
	int retval;

	DSP_Error_Number = 0;
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock())
		return FALSE;
#endif
	if(!DSP_Send_Set_Temperature(adu))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
	/* get reply - DON returned */
	retval = DSP_Get_Reply(CCD_DSP_DON);
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock())
		return FALSE;
#endif
	return retval;
}

/**
 * This routine executes the Resume EXposure (REX) command on a 
 * SDSU Controller board. This opens the shutter and restarts the timer.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #CCD_DSP_Command_PEX
 * @see #DSP_Send_Rex
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_REX(void)
{
	int retval;

	DSP_Error_Number = 0;
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock())
		return FALSE;
#endif
	if(!DSP_Send_Rex())
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
	/* get reply - DON should be returned */
	retval = DSP_Get_Reply(CCD_DSP_DON);
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock())
		return FALSE;
#endif
	return retval;
}

/**
 * This routine executes the Start EXposure (SEX) command on a SDSU Controller board.
 * <ul>
 * <li>A sleep is executed until it is nearly (DSP_Data.Start_Exposure_Clear_Time) time to start the exposure.
 * <li>The array is cleared using CCD_DSP_Command_CLR.
 * <li>A command is sent to the controller to open the shutter and start the exposure (DSP_Send_Sex). 
 * <li>The routine then enters a loop reading the exposure time (using CCD_DSP_Command_Read_Exposure_Time),
 * 	until the time nears for the shutter to close (DSP_Data.Readout_Remaining_Time) and readout to take place.
 * </ul>
 * The DSP_Data.Exposure_Status is changed to reflect the operation being performed on the CCD.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param start_time The time to start the exposure. If the tv_sec field of the structure is zero,
 * 	we can start the exposure at any convenient time.
 * @param exposure_time The amount of time in milliseconds to open the shutter for. This must be greater than zero.
 * @return The routine returns DON if the command succeeded and FALSE if the command failed.
 * @see #DSP_Data
 * @see #DSP_Send_Sex
 * @see #CCD_DSP_Command_CLR
 * @see #CCD_DSP_Command_Read_Exposure_Time
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_SEX(struct timespec start_time,int exposure_time)
{
	struct timespec sleep_time,current_time;
#ifndef _POSIX_TIMERS
	struct timeval gtod_current_time;
#endif
	int dsp_exposure_time,remaining_exposure_time,done;

	DSP_Error_Number = 0;
/* exposure time must be greater than zero */
	if(exposure_time <= 0)
	{
		DSP_Error_Number = 19;
		sprintf(DSP_Error_String,"CCD_DSP_Command_SEX:Illegal exposure_time '%d'.",exposure_time);
		return FALSE;
	}
/* initialise variables */
/* we haven't started exposing yet, the remaining exposure time is the exposure time. */
	remaining_exposure_time = exposure_time;
/* We will use the start_time parameter to determine when to start the exposure IF 
** it's seconds are greater then zero */ 
/* do the clear array a few seconds before the exposure is due to start */
	if(start_time.tv_sec > 0)
	{
		done = FALSE;
		while(done == FALSE)
		{
#ifdef _POSIX_TIMERS
			clock_gettime(CLOCK_REALTIME,&current_time);
#else
			gettimeofday(&gtod_current_time,NULL);
			current_time.tv_sec = gtod_current_time.tv_sec;
			current_time.tv_nsec = gtod_current_time.tv_usec*DSP_ONE_MICROSECOND_NS;
#endif
		/* if we've time, sleep for a second */
			if((start_time.tv_sec - current_time.tv_sec) > DSP_Data.Start_Exposure_Clear_Time)
			{
				sleep_time.tv_sec = 1;
				sleep_time.tv_nsec = 0;
				nanosleep(&sleep_time,NULL);
			}
			else
				done = TRUE;
		/* check - have we been aborted? */
			if(DSP_Data.Abort)
				done = TRUE;
		}/* end while */
	}
	if(DSP_Data.Abort)
	{
		DSP_Error_Number = 16;
		sprintf(DSP_Error_String,"CCD_DSP_Command_SEX:Aborted.");
		return FALSE;
	}
/* clear the array */
	if(!CCD_DSP_Command_CLR())
	{
		DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_NONE;
		return FALSE;
	}
/* start the exposure */
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock())
		return FALSE;
#endif
	if(!DSP_Send_Sex(start_time))
	{
		DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_NONE;
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
/* ensure SEX command sent correctly */
	if(!DSP_Get_Reply(CCD_DSP_DON))
	{
		DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_NONE;
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock())
		return FALSE;
#endif
/* wait until we are about ready to read out */
	while((remaining_exposure_time > DSP_Data.Readout_Remaining_Time)&&(DSP_Data.Abort == FALSE))
	{
		sleep_time.tv_sec = 1;
		sleep_time.tv_nsec = 0;
		nanosleep(&sleep_time,NULL);
	/* get elapsed time from utility board */
		dsp_exposure_time = CCD_DSP_Command_Read_Exposure_Time();
		if(dsp_exposure_time != 0)
			remaining_exposure_time = exposure_time-dsp_exposure_time;
		else
		{
			remaining_exposure_time -= DSP_ONE_SECOND_MS;
			if(DSP_Error_Number != 0)
				CCD_DSP_Error();
		}
	}/* end while exposing */
	if(DSP_Data.Abort)
	{
		DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_NONE;
		DSP_Error_Number = 21;
		sprintf(DSP_Error_String,"CCD_DSP_Command_SEX:Aborted.");
		return FALSE;
	}
/* reset status to none, until readout command is sent */
	DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_NONE;
	return CCD_DSP_DON;
}

/**
 * This routine resets the SDSU Controller boards. It sends the PCI reset controller command.
 * It then gets the reply from the interface, which should be SYR.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @return The routine returns SYR if the command succeeded and FALSE if the command failed.
 * @see #DSP_Send_Reset
 * @see #DSP_Get_Reply
 * @see #CCD_DSP_SYR
 */
int CCD_DSP_Command_Reset(void)
{
	int retval;

	DSP_Error_Number = 0;
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock())
		return FALSE;
#endif
	if(!DSP_Send_Reset())
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
	/* get reply - DON should be returned */
	retval = DSP_Get_Reply(CCD_DSP_SYR);
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock())
		return FALSE;
#endif
	return retval;
}

/**
 * Routine to set the utility board options. This is written to the PCI interface 
 * utility options register using a special PCI command. The PCI interface itself writes this to the
 * utility board, memory space X, location 1.
 * Currently only the first bit is used. If it is set the utility board will open the shutter during a 
 * SEX command, otherwise the shutter will remain closed.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param bit_value The bit value of the utility options. 
 * @return The routine returns DON if the operation succeeds, FALSE otherwise.
 * @see #DSP_Send_Set_Util_Options
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_Set_Util_Options(int bit_value)
{
	int retval;

	DSP_Error_Number = 0;
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock())
		return FALSE;
#endif
	if(!DSP_Send_Set_Util_Options(bit_value))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
/* get reply - DON should be returned */
	retval = DSP_Get_Reply(CCD_DSP_DON);
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock())
		return FALSE;
#endif
	return retval;
}

/**
 * Routine to set the utility board exposure time. This is written to the PCI interface 
 * exposure time register using a special PCI command. The PCI interface itself writes this to the
 * utility board, memory space Y, location 24.
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @param msecs The exposure time in milliseconds. 
 * @return The routine returns DON if the operation succeeds, FALSE otherwise.
 * @see #DSP_Send_Set_Exposure_Time
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_Set_Exposure_Time(int msecs)
{
	int retval;

	DSP_Error_Number = 0;
/* exposure time  must be a positive/zero number */
	if(msecs < 0)
	{
		DSP_Error_Number = 29;
		sprintf(DSP_Error_String,"CCD_DSP_Command_Set_Exposure_Time:Illegal msecs '%d'.",msecs);
		return FALSE;
	}
	DSP_Data.Exposure_Length = msecs;
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock())
		return FALSE;
#endif
	if(!DSP_Send_Set_Exposure_Time(msecs))
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
/* get reply - DON should be returned */
	retval = DSP_Get_Reply(CCD_DSP_DON);
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock())
		return FALSE;
#endif
	return retval;
}

/**
 * Routine to get the amount of time the utility board has had the shutter open for, i.e. the
 * amount of time an exposure has been underway. 
 * If mutex locking has been compiled in, the routine is mutexed over sending the command to the controller
 * and receiving a reply from it.
 * @return If an error has occured or an exposure is not taking place, FALSE is returned. Otherwise
 * 	the amount of time an exposure has been underway is returned, in milliseconds.
 * @see #DSP_Send_Read_Exposure_Time
 * @see #DSP_Get_Reply
 */
int CCD_DSP_Command_Read_Exposure_Time(void)
{
	int retval;

	DSP_Error_Number = 0;
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Lock())
		return FALSE;
#endif
	if(!DSP_Send_Read_Exposure_Time())
	{
#ifdef CCD_DSP_MUTEXED
		DSP_Mutex_Unlock();
#endif
		return FALSE;
	}
/* get reply - the exposure time in milliseconds should be returned */
	retval = DSP_Get_Reply(DSP_ACTUAL_VALUE);
#ifdef CCD_DSP_MUTEXED
	if(!DSP_Mutex_Unlock())
		return FALSE;
#endif
	return retval;
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
	int finished,download_board_id,addr;
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
 * This routine gets the current value of Exposure Length.
 * @return The last exposure length.
 * @see #DSP_Data
 */
int CCD_DSP_Get_Exposure_Length(void)
{
	return DSP_Data.Exposure_Length;
}

/**
 * This routine gets the time stamp for the start of the exposure.
 * @return The time stamp for the start of the exposure.
 * @see #DSP_Data
 */
struct timespec CCD_DSP_Get_Exposure_Start_Time(void)
{
	return DSP_Data.Exposure_Start_Time;
}

/**
 * Routine to set how many seconds before an exposure is due to start we wish to send the CLEAR_ARRAY
 * command to the controller.
 * @param time The time in seconds. This should be greater than the time the CLEAR_ARRAY command takes to
 * 	clock all accumulated charge off the CCD (approx 5 seconds for a 2kx2k EEV42-40).
 * @see #DSP_Data
 */
void CCD_DSP_Set_Start_Exposure_Clear_Time(int time)
{
	DSP_Data.Start_Exposure_Clear_Time = time;
}

/**
 * Routine to get the current setting for how many seconds before an exposure is due to start we wish 
 * to send the CLEAR_ARRAY command to the controller.
 * @return The time, in seconds.
 * @see #DSP_Data
 */
int CCD_DSP_Get_Start_Exposure_Clear_Time(void)
{
	return DSP_Data.Start_Exposure_Clear_Time;
}

/**
 * Routine to set the amount of time, in milliseconds, before the desired start of exposure that we should send the
 * START_EXPOSURE command, to allow for transmission delay.
 * @param time The time, in milliseconds.
 * @see #DSP_Data
 */
void CCD_DSP_Set_Start_Exposure_Offset_Time(int time)
{
	DSP_Data.Start_Exposure_Offset_Time = time;
}

/**
 * Routine to get the amount of time, in milliseconds, before the desired start of exposure that we should send the
 * START_EXPOSURE command, to allow for transmission delay.
 * @return The time, in milliseconds.
 * @see #DSP_Data
 */
int CCD_DSP_Get_Start_Exposure_Offset_Time(void)
{
	return DSP_Data.Start_Exposure_Offset_Time;
}

/**
 * Routine to set the amount of time, in milleseconds, remaining for an exposure when we stop sleeping and tell the
 * interface to enter readout mode. 
 * @param time The time, in milliseconds. Note, because the exposure time is read every second, it is best
 * 	not have have this constant an exact multiple of 1000.
 * @see #DSP_Data
 */
void CCD_DSP_Set_Readout_Remaining_Time(int time)
{
	DSP_Data.Readout_Remaining_Time = time;
}

/**
 * Routine to get the amount of time, in milleseconds, remaining for an exposure when we stop sleeping and tell the
 * interface to enter readout mode. 
 * @return The time, in milliseconds.
 * @see #DSP_Data
 */
int CCD_DSP_Get_Readout_Remaining_Time(void)
{
	return DSP_Data.Readout_Remaining_Time;
}

/**
 * Routine to set the Exposure_Start_Time of DSP_Data, to the current time of the real time clock.
 * clock_gettime or gettimeofday is used, depending on whether _POSIX_TIMERS is defined.
 * @see #DSP_Data
 */
void CCD_DSP_Set_Exposure_Start_Time(void)
{
#ifndef _POSIX_TIMERS
	struct timeval gtod_current_time;
#endif

#ifdef _POSIX_TIMERS
	clock_gettime(CLOCK_REALTIME,&(DSP_Data.Exposure_Start_Time));
#else
	gettimeofday(&gtod_current_time,NULL);
	DSP_Data.Exposure_Start_Time.tv_sec = gtod_current_time.tv_sec;
	DSP_Data.Exposure_Start_Time.tv_nsec = gtod_current_time.tv_usec*DSP_ONE_MICROSECOND_NS;
#endif
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
	DSP_Error_Number = 0;
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
	DSP_Error_Number = 0;
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
 * If the tv_sec field of start_time is non-zero, we want to open the shutter as near as possible to the 
 * passed in time, allowing for some transmission delay (DSP_Data.Start_Exposure_Offset_Time).
 * Sets the DSP_Data.Exposure_Start_Time to start of the exposure.
 * @param start_time The time to start the exposure. If the tv_sec field of the structure is zero,
 * 	we can start the exposure at any convenient time.
 * @return Returns true if sending the command succeeded, false if it failed.
 * @see #DSP_Data
 * @see #DSP_Send_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_START_EXPOSURE
 */
static int DSP_Send_Sex(struct timespec start_time)
{
	struct timespec current_time,sleep_time;
#ifndef _POSIX_TIMERS
	struct timeval gtod_current_time;
#endif
	int remaining_sec,remaining_ns,done = FALSE;

/* We need to review whether this set destination call is needed.
** Acording to the Voodoo code it is, according to the DSP code it is not.
*/
	if(!DSP_Set_Destination(CCD_DSP_INTERFACE_BOARD_ID,0))
		return FALSE;
/* if a start time has been specified wait for it */
	if(start_time.tv_sec > 0)
	{
		done = FALSE;
		while(done == FALSE)
		{
#ifdef _POSIX_TIMERS
			clock_gettime(CLOCK_REALTIME,&current_time);
#else
			gettimeofday(&gtod_current_time,NULL);
			current_time.tv_sec = gtod_current_time.tv_sec;
			current_time.tv_nsec = gtod_current_time.tv_usec*DSP_ONE_MICROSECOND_NS;
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
				remaining_ns -= DSP_Data.Start_Exposure_Offset_Time*DSP_ONE_MILLISECOND_NS;
				if(remaining_ns < 0)
				{
					remaining_sec--;
					remaining_ns += DSP_ONE_SECOND_NS;
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
		/* if an abort has occured, stop sleeping. The following command send will catch the abort. */
			if(DSP_Data.Abort)
				done = TRUE;
		}/* end while */
	}/* end if */
/* switch status to exposing and store the actual time the exposure is going to start */
	DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_EXPOSE;
	CCD_DSP_Set_Exposure_Start_Time();
/* start exposure and return result */
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
 * Internal DSP command to get the time an exposure has been underway. 
 * @return Returns TRUE if the command was sent without error, FALSE otherwise.
 * @see #DSP_Send_Command
 * @see ccd_pci.html#CCD_PCI_HCVR_READ_EXPOSURE_TIME
 */
static int DSP_Send_Read_Exposure_Time(void)
{
	return DSP_Send_Command(CCD_PCI_HCVR_READ_EXPOSURE_TIME,NULL,0);
}

/**
 * Internal command to clear the reply memory. This sets all the reply memory to -1.
 * @return Returns true if clearing the reply memory succeeded, false if it failed.
 * @see ccd_interface.html#CCD_Interface_Command
 * @see ccd_pci.html#CCD_PCI_IOCTL_CLEAR_REPLY
 */
static int DSP_Clear_Reply_Memory(void)
{
	int retval;

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
 * @return Returns TRUE if the conversion succeeded, FALSE if it failed.
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
	int finished, value;
	char c;

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

#if CCD_GLOBAL_BYTES_PER_PIXEL == 2
/**
 * This routine is responsible for transferring image data on the CCD into memory, de-interlacing it
 * and saving it to disk.
 * <ul>
 * <li>It allocates sufficient memory for the resultant image (ncols*nrows*CCD_GLOBAL_BYTES_PER_PIXEL).
 * <li>It reads the image into memory using  
 * 	<a href="ccd_interface.html#CCD_Interface_Get_Reply_Data">CCD_Interface_Get_Reply_Data</a>. 
 * <li>It byte swaps the image using <a href="#DSP_Byte_Swap">DSP_Byte_Swap</a>.
 * <li>It deinterlaces the image using <a href="#DSP_DeInterlace">DSP_DeInterlace</a>.
 * <li>The image is then saved using <a href="#DSP_Save">DSP_Save</a> to a file and the image data freed.
 * </ul>
 * The Exposure_Status variable in <a href="#DSP_Data">DSP_Data</a> is maintained to show the current
 * status of the exposure.
 * If at any point the exposure is aborted using <a href="#CCD_DSP_Set_Abort">CCD_DSP_Set_Abort</a> the routine
 * aborts when it next checks the Abort flag.
 * @param ncols The number of columns the CCD has.
 * @param nrows The number of rows the CCD has.
 * @param deinterlace_type What deinterlacing algorithm to perform on the raw data.
 * @param filename The filename to save the deinterlaced image to.
 * @return Returns TRUE if the exposure succeeded, FALSE if it failed or was aborted.
 * @see #CCD_DSP_Set_Abort
 * @see #DSP_Byte_Swap
 * @see #DSP_DeInterlace
 * @see #DSP_Save
 * @see ccd_interface.html#CCD_Interface_Get_Reply_Data
 * @see ccd_global.html#CCD_GLOBAL_BYTES_PER_PIXEL
 */
static int DSP_Image_Transfer(int ncols,int nrows,enum CCD_DSP_DEINTERLACE_TYPE deinterlace_type,char *filename)
{
	int retval,numbytes;
	unsigned short *exposure_data = NULL;

/* if we have aborted stop here */
	if(DSP_Data.Abort)
	{
		DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_NONE;
		DSP_Error_Number = 42;
		sprintf(DSP_Error_String,"DSP_Image_Transfer:Aborted.");
		return FALSE;
	}
/* calculate number of bytes in the image */
	numbytes = ncols*nrows*CCD_GLOBAL_BYTES_PER_PIXEL;
/* start reading out image */
	DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_READOUT;
	exposure_data = (unsigned short *)malloc(numbytes);
	if(exposure_data == NULL)
	{
		DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_NONE;
		DSP_Error_Number = 43;
		sprintf(DSP_Error_String,"DSP_Image_Transfer:Memory Allocation Error(%d).",numbytes);
		return FALSE;
	}
	retval = CCD_Interface_Get_Reply_Data((char *)exposure_data,numbytes);
	if(retval == -1)
	{
		if(exposure_data != NULL)
			free(exposure_data);
		DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_NONE;
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
		if(exposure_data != NULL)
			free(exposure_data);
		DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_NONE;
		DSP_Error_Number = 46;
		sprintf(DSP_Error_String,"DSP_Image_Transfer:Aborted.");
		return FALSE;
	}
	DSP_Data.Exposure_Status = CCD_DSP_EXPOSURE_STATUS_NONE;
/* byte swap to get into right order */
	DSP_Byte_Swap(exposure_data,nrows*ncols);
/* Do deinterlacing. The image returned from the boards may not be in the correct order
** if the CCD was readout from multiple places etc. The deinterlace routine reorders the image
** so that it is back in the right order. */
	if(!DSP_DeInterlace(ncols,nrows,exposure_data,deinterlace_type))
	{
		if(exposure_data == NULL)
			free(exposure_data);
		return FALSE;
	}
/* if we have aborted stop and return */
	if(DSP_Data.Abort)
	{
		if(exposure_data != NULL)
			free(exposure_data);
		DSP_Error_Number = 48;
		sprintf(DSP_Error_String,"DSP_Image_Transfer:Aborted.");
		return FALSE;
	}
	/* save the resultant image to disk */
	if(!DSP_Save(filename,exposure_data,ncols,nrows))
	{
		if(exposure_data != NULL)
			free(exposure_data);
		return FALSE;
	}
	/* free image in memory */
	if(exposure_data != NULL)
		free(exposure_data);
	return TRUE;
}
#else
#error DSP_Image_Transfer not defined for this value of CCD_GLOBAL_BYTES_PER_PIXEL.
#endif

#if CCD_GLOBAL_BYTES_PER_PIXEL == 2
/**
 * Swap the bytes in the input unsigned short integers: ( 0 1 -> 1 0 ).
 * Based on CFITSIO's ffswap2 routine. This routine only works for CCD_GLOBAL_BYTES_PER_PIXEL == 2.
 * @param svalues A list of unsigned short values to byte swap.
 * @param nvals The number of values in svalues.
 */
static void DSP_Byte_Swap(unsigned short *svalues,long nvals)
{
	register char *cvalues;
	register long i;

/* equivalence an array of 2 bytes with a short */
	union u_tag
	{
		char cvals[2];
		unsigned short sval;
	} u;
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
}
#else
#error DSP_Byte_Swap not defined for this value of CCD_GLOBAL_BYTES_PER_PIXEL.
#endif

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
 * <em>Note: This routine assumes 
 * <a href="ccd_global.html#CCD_GLOBAL_BYTES_PER_PIXEL">CCD_GLOBAL_BYTES_PER_PIXEL</a>== 2 e.g. 
 * 16 bits per pixel.</em>
 * @param ncols The number of columns on the CCD.
 * @param nrows The number of rows on the CCD.
 * @param old_iptr The interlaced image data received from the CCD. Once deinterlaced the image data is copied back
 * 	in this memory area.
 * @param deinterlace_type The type of deinterlacing to perform. One of 
 * 	<a href="ccd_setup.html#CCD_DSP_DEINTERLACE_TYPE">CCD_DSP_DEINTERLACE_TYPE</a>:
 * 	CCD_DSP_DEINTERLACE_SINGLE,
 * 	CCD_DSP_DEINTERLACE_SPLIT_PARALLEL,
 * 	CCD_DSP_DEINTERLACE_SPLIT_SERIAL or
 * 	CCD_DSP_DEINTERLACE_SPLIT_QUAD.
 * @return If everything was successful TRUE is returned, otherwise FALSE is returned.
 */
static int DSP_DeInterlace(int ncols,int nrows,unsigned short *old_iptr,
	enum CCD_DSP_DEINTERLACE_TYPE deinterlace_type)
{
	unsigned short *new_iptr;

	/* allocate enough memory to store the deinterlaced image */
	if((new_iptr = (unsigned short *)malloc(ncols*nrows*CCD_GLOBAL_BYTES_PER_PIXEL)) == NULL)
	{
 		DSP_Error_Number = 49;
		sprintf(DSP_Error_String,"DSP_DeInterlace:Memory Allocation Error(%d,%d),image not deinterlaced.",
			ncols,nrows);
		return FALSE;
	}
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
 				DSP_Error_Number = 50;
				sprintf(DSP_Error_String,"DSP_DeInterlace:Split Parallel Readout,"
					"nrows not even(%d), image not deinterlaced.",nrows);
				free(new_iptr);
				return FALSE;
			}
			for(i=0;i<(ncols*nrows)/2;i++)
			{
				*(new_iptr+i) = *(old_iptr+(2*i));
				*(new_iptr+(ncols*nrows)-i-1) = *(old_iptr+(2*i)+1);
			}
			memcpy(old_iptr,new_iptr,ncols*nrows*CCD_GLOBAL_BYTES_PER_PIXEL);
			return TRUE;
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
 				DSP_Error_Number = 52;
				sprintf(DSP_Error_String,"DSP_DeInterlace:Split Quad Readout,"
					"ncols or nrows not even(%d,%d), image not deinterlaced.",ncols,nrows);
				free(new_iptr);
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
	DSP_Error_Number = 53;
	sprintf(DSP_Error_String,"DSP_DeInterlace:Wrong DeInterlace option(%d),Image not deinterlaced.",
		deinterlace_type);
	free(new_iptr);
	return FALSE;
} /* end deinterlacing */
#else
#error DSP_DeInterlace not defined for this value of CCD_GLOBAL_BYTES_PER_PIXEL.
#endif

/* 
** DSP_Save uses a different implementation depending on whether CFITSIO define was defined at compile time.
** If it was we use CFITSIO routines, otherwise we don't.
*/

#ifdef CFITSIO
/**
 * This routine takes some image data and saves it in a file on disc. It also updates the 
 * DATE-OBS FITS keyword to the value saved just before the START_EXPOSURE command was sent to the controller.
 * @param filename The filename to save the data into.
 * @param exposure_data The data to save.
 * @param ncols The number of columns in the image data.
 * @param nrows The number of rows in the image data.
 * @return Returns TRUE if the image is saved successfully, FALSE if it fails.
 */
static int DSP_Save(char *filename,unsigned short *exposure_data,int ncols,int nrows)
{
	fitsfile *fp = NULL;
	int retval=0,status=0;
	char buff[32]; /* fits_get_errstatus returns 30 chars max */
	char exposure_start_time_string[64];

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
	/* write the data */
	retval = fits_write_img(fp,TUSHORT,1,ncols*nrows,exposure_data,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fp,&status);
		DSP_Error_Number = 55;
		sprintf(DSP_Error_String,"DSP_Save: File write failed(%s,%d,%s).",filename,status,buff);
		return FALSE;
	}
/* update DATE keyword */
	DSP_TimeSpec_To_Date_String(DSP_Data.Exposure_Start_Time,exposure_start_time_string);
	retval = fits_update_key(fp,TSTRING,"DATE",exposure_start_time_string,NULL,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fp,&status);
		DSP_Error_Number = 68;
		sprintf(DSP_Error_String,"DSP_Save: Updating DATE failed(%s,%d,%s).",filename,status,buff);
		return FALSE;
	}
/* update DATE-OBS keyword */
	DSP_TimeSpec_To_Date_Obs_String(DSP_Data.Exposure_Start_Time,exposure_start_time_string);
	retval = fits_update_key(fp,TSTRING,"DATE-OBS",exposure_start_time_string,NULL,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fp,&status);
		DSP_Error_Number = 12;
		sprintf(DSP_Error_String,"DSP_Save: Updating DATE-OBS failed(%s,%d,%s).",filename,status,buff);
		return FALSE;
	}
/* update UTSTART keyword */
	DSP_TimeSpec_To_UtStart_String(DSP_Data.Exposure_Start_Time,exposure_start_time_string);
	retval = fits_update_key(fp,TSTRING,"UTSTART",exposure_start_time_string,NULL,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fp,&status);
		DSP_Error_Number = 17;
		sprintf(DSP_Error_String,"DSP_Save: Updating UTSTART failed(%s,%d,%s).",filename,status,buff);
		return FALSE;
	}
/* close file */
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
 * This routine does not update the DATE-OBS keyword, unlike the CFITSIO routine.
 * @param filename The filename to save the data into.
 * @param exposure_data The data to save.
 * @param ncols The number of columns in the image data.
 * @param nrows The number of rows in the image data.
 * @return Returns TRUE if the image is saved successfully, FALSE if it fails.
 */
static int DSP_Save(char *filename,unsigned short *exposure_data,int ncols,int nrows)
{
	FILE *fp = NULL;
	int retval,error_number,nitems;

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
	nitems = nrows*ncols;
	retval = fwrite(exposure_data,CCD_GLOBAL_BYTES_PER_PIXEL,nitems,fp);
	if(retval != nitems)
	{
		fclose(fp);
		DSP_Error_Number = 59;
		sprintf(DSP_Error_String,"DSP_Save: File write failed(%s,%d,%d).",filename,retval,nitems);
		return FALSE;
	}
	fclose(fp);
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
static void DSP_TimeSpec_To_Date_String(struct timespec time,char *time_string)
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
 */
static void DSP_TimeSpec_To_Date_Obs_String(struct timespec time,char *time_string)
{
	struct tm *tm_time = NULL;
	char buff[32];
	int milliseconds;

	tm_time = gmtime(&(time.tv_sec));
	strftime(buff,32,"%Y-%m-%dT%H:%M:%S.",tm_time);
	milliseconds = (((double)time.tv_nsec)/((double)DSP_ONE_MILLISECOND_NS));
	sprintf(time_string,"%s%03d",buff,milliseconds);
}

/**
 * Routine to convert a timespec structure to a UTSTART sytle string to put into a FITS header.
 * This uses gmtime and strftime to format most of the string, and tags the milliseconds on the end.
 * @param time The time to convert.
 * @param time_string The string to put the time representation in. The string must be at least
 * 	14 characters long.
 */
static void DSP_TimeSpec_To_UtStart_String(struct timespec time,char *time_string)
{
	struct tm *tm_time = NULL;
	char buff[16];
	int milliseconds;

	tm_time = gmtime(&(time.tv_sec));
	strftime(buff,16,"%H:%M:%S.",tm_time);
	milliseconds = (((double)time.tv_nsec)/((double)DSP_ONE_MILLISECOND_NS));
	sprintf(time_string,"%s%03d",buff,milliseconds);
}

#ifdef CCD_DSP_MUTEXED
/**
 * Routine to lock the controller access mutex. This will block until the mutex has been acquired,
 * unless an error occurs.
 * @return Returns TRUE if the mutex has been  locked for access by this thread,
 * 	FALSE if an error occured.
 * @see #DSP_Data
 */
static int DSP_Mutex_Lock(void)
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
 * @return Returns TRUE if the mutex has been unlocked, FALSE if an error occured.
 * @see #DSP_Data
 */
static int DSP_Mutex_Unlock(void)
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

/*
** $Log: not supported by cvs2svn $
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

/* newmark_command.c
** Newmark Motion Controller library.
** $Header: /home/cjm/cvs/frodospec/newmark_motion_controller/c/newmark_command.c,v 1.4 2011-01-05 14:14:17 cjm Exp $
*/
/**
 * Command routines for the Newmark Motion Controller.
 * @author Chris Mottram
 * @version $Revision: 1.4 $
 */
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
#define _POSIX_SOURCE 1
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
#define _POSIX_C_SOURCE 199309L

#include <errno.h>   /* Error number definitions */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#ifdef NEWMARK_MUTEXED
#include <pthread.h>
#endif
#include "log_udp.h"
#include "newmark_general.h"
#include "newmark_command.h"

/* hash defines */
/**
 * The character used as a command prompt by the newmark controller ('>').
 */
#define COMMAND_PROMPT      ('>')
/**
 * Length of general buffer.
 */
#define COMMAND_BUFF_LENGTH (256)
/**
 * How long some buffers are when generating logging messages.
 */
#define LOG_BUFF_LENGTH     (1024)
/**
 * The number of nanoseconds in a millsecond (1000000).
 */
#define ONE_MILLISECOND_NS  (1000000)
/**
 * Number of milliseconds to sleep between reads in Command_Read_Until_Prompt.
 * Actual timeout time is determined by this and COMMAND_READ_LOOP_TIMEOUT.
 */
#define COMMAND_READ_LOOP_SLEEP_MS (10)
/**
 * Number of times around the read loop reading nothing before we time out.
 * Actual length of time is affected by COMMAND_READ_LOOP_SLEEP_MS.
 * @see #COMMAND_READ_LOOP_SLEEP_MS
 * @see #Command_Read_Until_Promp
 */
#define COMMAND_READ_LOOP_TIMEOUT (100)
/**
 * The default value for the position tolerance.
 * @see #Position_Tolerance
 */
#define DEFAULT_POSITION_TOLERANCE (0.002)

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id: newmark_command.c,v 1.4 2011-01-05 14:14:17 cjm Exp $";
/**
 * How close the reported position has to be to the requested position before the stage
 * is deemed to be at the requested position. In millimetres.
 * @see #DEFAULT_POSITION_TOLERANCE
 */
static double Position_Tolerance = DEFAULT_POSITION_TOLERANCE;

/* internal functions */
static int Command_Read_Flush(char *class,char *source,Arcom_ESS_Interface_Handle_T *handle);
static int Command_Read_Until_Prompt(char *class,char *source,Arcom_ESS_Interface_Handle_T *handle,
				     int loop_timeout_count,int timeout_is_error,char **read_string);
static char *Newmark_Command_Fix_String(char *string);

/* =======================================
**  external functions 
** ======================================= */
/**
 * Meta command. Sends a MOVA and then enters a loop reading the position and error status until the 
 * position is reached, or a timeout or error is detected.
 * @param class The class parameter for logging.
 * @param source The source parameter for logging.
 * @param handle The interface handle to the newmark controller.
 * @param position The address of a double to store the retrieved position of the slide.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Newmark_Command_Move_Absolute
 * @see #Newmark_Command_Position_Get
 * @see #Newmark_Command_Error_Get
 * @see #Newmark_Command_Err_Get
 * @see #Newmark_Command_Error_Reset
 * @see #Position_Tolerance
 */
int Newmark_Command_Move(char *class,char *source,Arcom_ESS_Interface_Handle_T *handle,double position)
{
	struct timespec sleep_time;
	int done,timeout_index,retval,sleep_errno,error_exists,error_code;
	double current_position,last_position;

#if LOGGING > 1
	Newmark_Log_Format(class,source,LOG_VERBOSITY_INTERMEDIATE,"Newmark_Command_Move(%.6f):Start.",position);
#endif
	/* reset error code */
	if(!Newmark_Command_Error_Reset(class,source,handle))
		return FALSE;
	/* move to an absolute position */
	if(!Newmark_Command_Move_Absolute(class,source,handle,position))
		return FALSE;
	done = FALSE;
	current_position = 0.0;
	timeout_index = 0;
	while(done == FALSE)
	{
		/* sleep a bit (10ms) */
		sleep_time.tv_sec = 0;
		sleep_time.tv_nsec = COMMAND_READ_LOOP_SLEEP_MS*ONE_MILLISECOND_NS;
		retval = nanosleep(&sleep_time,NULL);
		if(retval != 0)
		{
			sleep_errno = errno;
			Newmark_Error_Number = 130;
			sprintf(Newmark_Error_String,"Newmark_Command_Move:nanosleep failed.");
			/* non terminal error - just log and continue. */
			Newmark_Error();
		}
		/* keep a copy of last position */
		last_position = current_position;
		/* get current position */
		if(!Newmark_Command_Position_Get(class,source,handle,&current_position))
			return FALSE;
		/* have we moved - if not increment timeout */
		if(last_position == current_position)
			timeout_index++;
		if(timeout_index == 10)
		{
			Newmark_Error_Number = 131;
			sprintf(Newmark_Error_String,"Newmark_Command_Move:Move timed out.");
			return FALSE;
		}
		/* Is there an error? 
		** Don't use 'PRINT ERROR' for this as set errors persist until reset with 'ERROR = 0'.
		** 'PRINT ERR' returns TRUE after an error UNTIL the next 'PRINT ERROR' 
		** after which it is reset to FALSE. */
		if(!Newmark_Command_Err_Get(class,source,handle,&error_exists))
			return FALSE;
		if(error_exists)
		{
			/* Get error code.
			** Must only do this after a 'PRINT ERR' returns true, as 'PRINT ERROR' resets the
			** 'PRINT ERR' return value to FALSE (internally in the controller) 
			** put the error code from 'PRINT ERROR' persists
			** until explicitly being reset with 'ERROR = 0'.
			*/
			if(!Newmark_Command_Error_Get(class,source,handle,&error_code))
				return FALSE;
			Newmark_Error_Number = 132;
			sprintf(Newmark_Error_String,"Newmark_Command_Move:Error code was non-zero:%d.",error_code);
			return FALSE;
		}
		/* are we there? Is the reported position close enough to the requested position */
		done = (fabs(current_position - position) < Position_Tolerance);
	}
#if LOGGING > 1
	Newmark_Log(class,source,LOG_VERBOSITY_INTERMEDIATE,"Newmark_Command_Move:Finished.");
#endif
	return TRUE;
}
/**
 * Command used to home the Newmark encoder.
 * @param class The class parameter for logging.
 * @param source The source parameter for logging.
 * @param handle  The interface handle to the newmark controller.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Command_Read_Flush
 * @see #Command_Read_Until_Prompt
 * @see #Command_Buff
 * @see #Newmark_Command_Fix_String
 * @see #COMMAND_BUFF_LENGTH
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Handle_T
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Mutex_Lock
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Mutex_Unlock
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Write
 */
int Newmark_Command_Home(char *class,char *source,Arcom_ESS_Interface_Handle_T *handle)
{
	char *reply_string = NULL;
	char command_buff[COMMAND_BUFF_LENGTH];

#if LOGGING > 1
	Newmark_Log(class,source,LOG_VERBOSITY_INTERMEDIATE,"Newmark_Command_Home:Start.");
#endif
	/* mutex */
	if(!Arcom_ESS_Interface_Mutex_Lock(handle))
	{
		Newmark_Error_Number = 100;
		sprintf(Newmark_Error_String,"Newmark_Command_Home:Arcom_ESS_Interface_Mutex_Lock failed.");
		return FALSE;	       
	}
	/* clear any unread data */
	if(!Command_Read_Flush(class,source,handle))
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		return FALSE;
	}
	/* send HOME */
	sprintf(command_buff,"HOME\r\n");
#if LOGGING > 5
	Newmark_Log_Format(class,source,LOG_VERBOSITY_INTERMEDIATE,"Newmark_Command_Home:Writing '%s' to handle.",
			   Newmark_Command_Fix_String(command_buff));
#endif
	if(!Arcom_ESS_Interface_Write(class,source,handle,command_buff,strlen(command_buff)))
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		Newmark_Error_Number = 101;
		sprintf(Newmark_Error_String,"Newmark_Command_Home:Arcom_ESS_Interface_Write failed.");
		return FALSE;	       
	}
	/* read reply  - takes a long time to home - allow 6000 (x 10ms) = 60s */
	if(!Command_Read_Until_Prompt(class,source,handle,6000,TRUE,&reply_string))
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		return FALSE;
	}
#if LOGGING > 5
	Newmark_Log_Format(class,source,LOG_VERBOSITY_INTERMEDIATE,"Newmark_Command_Home:Read '%s' from handle.",
			   Newmark_Command_Fix_String(reply_string));
#endif
	/* parse reply string */
	if(strstr(reply_string,"Homing Complete") == NULL)
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		Newmark_Error_Number = 102;
		sprintf(Newmark_Error_String,"Newmark_Command_Home:Illegal Reply '%s'.",reply_string);
		if(reply_string != NULL)
			free(reply_string);
		return FALSE;	       
	}
	if(reply_string != NULL)
		free(reply_string);
	/* unlock mutex */
	if(!Arcom_ESS_Interface_Mutex_Unlock(handle))
	{
		Newmark_Error_Number = 103;
		sprintf(Newmark_Error_String,"Newmark_Command_Home:Arcom_ESS_Interface_Mutex_Unlock failed.");
		return FALSE;	       
	}
#if LOGGING > 1
	Newmark_Log(class,source,LOG_VERBOSITY_INTERMEDIATE,"Newmark_Command_Home:Finished.");
#endif
	return TRUE;
}

/**
 * Command used to get the current encoder position of the device, using the "PRINT POS" command.
 * @param class The class parameter for logging.
 * @param source The source parameter for logging.
 * @param handle The interface handle to the newmark controller.
 * @param position The address of a double to store the retrieved position of the slide.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #COMMAND_BUFF_LENGTH
 * @see #Command_Read_Flush
 * @see #Command_Read_Until_Prompt
 * @see #Newmark_Command_Fix_String
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Handle_T
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Mutex_Lock
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Mutex_Unlock
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Write
 */
int Newmark_Command_Position_Get(char *class,char *source,Arcom_ESS_Interface_Handle_T *handle,double *position)
{
	char command_buff[COMMAND_BUFF_LENGTH];
	char *reply_string = NULL;
	int retval;

	if(position == NULL)
	{
		Newmark_Error_Number = 113;
		sprintf(Newmark_Error_String,"Newmark_Command_Position_Get:position was NULL.");
		return FALSE;	       
	}
#if LOGGING > 1
	Newmark_Log(class,source,LOG_VERBOSITY_INTERMEDIATE,"Newmark_Command_Position_Get:Start.");
#endif
	/* mutex */
	if(!Arcom_ESS_Interface_Mutex_Lock(handle))
	{
		Newmark_Error_Number = 114;
		sprintf(Newmark_Error_String,"Newmark_Command_Position_Get:Arcom_ESS_Interface_Mutex_Lock failed.");
		return FALSE;	       
	}
	/* clear any unread data */
	if(!Command_Read_Flush(class,source,handle))
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		return FALSE;
	}
	/* send PRINT POS */
	sprintf(command_buff,"PRINT POS\r\n");
#if LOGGING > 5
	Newmark_Log_Format(class,source,LOG_VERBOSITY_INTERMEDIATE,
			   "Newmark_Command_Position_Get:Writing '%s' to handle.",
			   Newmark_Command_Fix_String(command_buff));
#endif
	if(!Arcom_ESS_Interface_Write(class,source,handle,command_buff,strlen(command_buff)))
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		Newmark_Error_Number = 115;
		sprintf(Newmark_Error_String,"Newmark_Command_Position_Get:Arcom_ESS_Interface_Write failed.");
		return FALSE;	       
	}
	/* read reply */
	if(!Command_Read_Until_Prompt(class,source,handle,100,TRUE,&reply_string))
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		return FALSE;
	}
#if LOGGING > 5
	Newmark_Log_Format(class,source,LOG_VERBOSITY_INTERMEDIATE,
			   "Newmark_Command_Position_Get:Read '%s' from handle.",
			   Newmark_Command_Fix_String(reply_string));
#endif
	/* parse reply string */
	retval = sscanf(reply_string,"PRINT POS %lf >",position);
	if(retval != 1)
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		Newmark_Error_Number = 116;
		sprintf(Newmark_Error_String,"Newmark_Command_Position_Get:Illegal Reply '%s'.",reply_string);
		if(reply_string != NULL)
			free(reply_string);
		return FALSE;	       
	}
	if(reply_string != NULL)
		free(reply_string);
	/* unlock mutex */
	if(!Arcom_ESS_Interface_Mutex_Unlock(handle))
	{
		Newmark_Error_Number = 117;
		sprintf(Newmark_Error_String,"Newmark_Command_Position_Get:Arcom_ESS_Interface_Mutex_Unlock failed.");
		return FALSE;	       
	}
#if LOGGING > 1
	Newmark_Log_Format(class,source,LOG_VERBOSITY_INTERMEDIATE,
			   "Newmark_Command_Position_Get:Finished with position %.6f.",(*position));
#endif
	return TRUE;
}

/**
 * Command used to move the slide the position controller is controlling to an absolute position, 
 * using the MOVA command.
 * Note the routine completes after the command has been received by the controller, the move may be
 * ongoing after this routine returns. Use Newmark_Command_Position_Get to monitor whether the
 * move arrives at the new position.
 * @param class The class parameter for logging.
 * @param source The source parameter for logging.
 * @param handle The interface handle to the newmark controller.
 * @param position A double indicating the target position to move to.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #COMMAND_BUFF_LENGTH
 * @see #Command_Read_Flush
 * @see #Command_Read_Until_Prompt
 * @see #Newmark_Command_Position_Get
 * @see #Newmark_Command_Fix_String
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Handle_T
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Mutex_Lock
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Mutex_Unlock
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Write
 */
int Newmark_Command_Move_Absolute(char *class,char *source,Arcom_ESS_Interface_Handle_T *handle,double position)
{
	char command_buff[COMMAND_BUFF_LENGTH];
	char *reply_string = NULL;

#if LOGGING > 1
	Newmark_Log_Format(class,source,LOG_VERBOSITY_INTERMEDIATE,"Newmark_Command_Move_Absolute(%.6f):Start.",
			   position);
#endif
	/* mutex */
	if(!Arcom_ESS_Interface_Mutex_Lock(handle))
	{
		Newmark_Error_Number = 107;
		sprintf(Newmark_Error_String,"Newmark_Command_Move_Absolute:Arcom_ESS_Interface_Mutex_Lock failed.");
		return FALSE;	       
	}
	/* clear any unread data */
	if(!Command_Read_Flush(class,source,handle))
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		return FALSE;
	}
	/* send MOVA <position> */
	sprintf(command_buff,"MOVA %.6f\r\n",position);
#if LOGGING > 5
	Newmark_Log_Format(class,source,LOG_VERBOSITY_INTERMEDIATE,
			   "Newmark_Command_Move_Absolute:Writing '%s' to handle.",
			   Newmark_Command_Fix_String(command_buff));
#endif
	if(!Arcom_ESS_Interface_Write(class,source,handle,command_buff,strlen(command_buff)))
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		Newmark_Error_Number = 108;
		sprintf(Newmark_Error_String,"Newmark_Command_Move_Absolute:Arcom_ESS_Interface_Write failed.");
		return FALSE;	       
	}
	/* read reply */
	if(!Command_Read_Until_Prompt(class,source,handle,100,TRUE,&reply_string))
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		return FALSE;
	}
#if LOGGING > 5
	Newmark_Log_Format(class,source,LOG_VERBOSITY_INTERMEDIATE,
			   "Newmark_Command_Move_Absolute:Read '%s' from handle.",
			   Newmark_Command_Fix_String(reply_string));
#endif
	/* parse reply string */
	if(strstr(reply_string,">") == NULL)
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		Newmark_Error_Number = 109;
		sprintf(Newmark_Error_String,"Newmark_Command_Move_Absolute:Illegal Reply '%s'.",reply_string);
		if(reply_string != NULL)
			free(reply_string);
		return FALSE;	       
	}
	if(reply_string != NULL)
		free(reply_string);
	/* unlock mutex */
	if(!Arcom_ESS_Interface_Mutex_Unlock(handle))
	{
		Newmark_Error_Number = 110;
		sprintf(Newmark_Error_String,"Newmark_Command_Move_Absolute:Arcom_ESS_Interface_Mutex_Unlock failed.");
		return FALSE;	       
	}
#if LOGGING > 1
	Newmark_Log(class,source,LOG_VERBOSITY_INTERMEDIATE,"Newmark_Command_Move_Absolute:Finished.");
#endif
	return TRUE;
}

/**
 * Command used to move the slide the position controller is controlling relative to it's current position, 
 * using the MOVR command.
 * Note the routine completes after the command has been received by the controller, the move may be
 * ongoing after this routine returns. Use Newmark_Command_Position_Get to monitor whether the
 * move arrives at the new position.
 * @param class The class parameter for logging.
 * @param source The source parameter for logging.
 * @param handle The interface handle to the newmark controller.
 * @param position A double indicating the relative position (offset) to move.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #COMMAND_BUFF_LENGTH
 * @see #Command_Read_Flush
 * @see #Command_Read_Until_Prompt
 * @see #Newmark_Command_Position_Get
 * @see #Newmark_Command_Fix_String
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Handle_T
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Mutex_Lock
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Mutex_Unlock
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Write
 */
int Newmark_Command_Move_Relative(char *class,char *source,Arcom_ESS_Interface_Handle_T *handle,double position_offset)
{
	char command_buff[COMMAND_BUFF_LENGTH];
	char *reply_string = NULL;

#if LOGGING > 1
	Newmark_Log_Format(class,source,LOG_VERBOSITY_INTERMEDIATE,"Newmark_Command_Move_Relative(%.6f):Start.",
			   position_offset);
#endif
	/* mutex */
	if(!Arcom_ESS_Interface_Mutex_Lock(handle))
	{
		Newmark_Error_Number = 118;
		sprintf(Newmark_Error_String,"Newmark_Command_Move_Relative:Arcom_ESS_Interface_Mutex_Lock failed.");
		return FALSE;	       
	}
	/* clear any unread data */
	if(!Command_Read_Flush(class,source,handle))
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		return FALSE;
	}
	/* send MOVR <position> */
	sprintf(command_buff,"MOVR %.6f\r\n",position_offset);
#if LOGGING > 5
	Newmark_Log_Format(class,source,LOG_VERBOSITY_INTERMEDIATE,
			   "Newmark_Command_Move_Relative:Writing '%s' to handle.",
			   Newmark_Command_Fix_String(command_buff));
#endif
	if(!Arcom_ESS_Interface_Write(class,source,handle,command_buff,strlen(command_buff)))
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		Newmark_Error_Number = 119;
		sprintf(Newmark_Error_String,"Newmark_Command_Move_Relative:Arcom_ESS_Interface_Write failed.");
		return FALSE;	       
	}
	/* read reply */
	if(!Command_Read_Until_Prompt(class,source,handle,100,TRUE,&reply_string))
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		return FALSE;
	}
	/* parse reply string */
#if LOGGING > 5
	Newmark_Log_Format(class,source,LOG_VERBOSITY_INTERMEDIATE,
			   "Newmark_Command_Move_Relative:Read '%s' from handle.",
			   Newmark_Command_Fix_String(reply_string));
#endif
	if(strstr(reply_string,">") == NULL)
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		Newmark_Error_Number = 120;
		sprintf(Newmark_Error_String,"Newmark_Command_Move_Relative:Illegal Reply '%s'.",reply_string);
		if(reply_string != NULL)
			free(reply_string);
		return FALSE;	       
	}
	if(reply_string != NULL)
		free(reply_string);
	/* unlock mutex */
	if(!Arcom_ESS_Interface_Mutex_Unlock(handle))
	{
		Newmark_Error_Number = 121;
		sprintf(Newmark_Error_String,"Newmark_Command_Move_Relative:Arcom_ESS_Interface_Mutex_Unlock failed.");
		return FALSE;	       
	}
#if LOGGING > 1
	Newmark_Log(class,source,LOG_VERBOSITY_INTERMEDIATE,"Newmark_Command_Relative:Finished.");
#endif
	return TRUE;
}

/**
 * Stop motion with the controller using the <esc> command.
 * @param class The class parameter for logging.
 * @param source The source parameter for logging.
 * @param handle The interface handle to the newmark controller.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #COMMAND_BUFF_LENGTH
 * @see #Command_Read_Flush
 * @see #Command_Read_Until_Prompt
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Handle_T
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Mutex_Lock
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Mutex_Unlock
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Write
 */
int Newmark_Command_Abort_Move(char *class,char *source,Arcom_ESS_Interface_Handle_T *handle)
{
	char command_buff[COMMAND_BUFF_LENGTH];

#if LOGGING > 1
	Newmark_Log(class,source,LOG_VERBOSITY_INTERMEDIATE,"Newmark_Command_Abort_Move:Start.");
#endif
	/* mutex */
	if(!Arcom_ESS_Interface_Mutex_Lock(handle))
	{
		Newmark_Error_Number = 122;
		sprintf(Newmark_Error_String,"Newmark_Command_Abort_Move:Arcom_ESS_Interface_Mutex_Lock failed.");
		return FALSE;	       
	}
	/* clear any unread data */
	if(!Command_Read_Flush(class,source,handle))
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		return FALSE;
	}
	/* send <esc>  - escape is ASCII character 0x1B*/
	strcpy(command_buff,"\x1B");
#if LOGGING > 5
	Newmark_Log(class,source,LOG_VERBOSITY_INTERMEDIATE,
		    "Newmark_Command_Move_Relative:Writing '<esc>' to handle.");
#endif
	if(!Arcom_ESS_Interface_Write(class,source,handle,command_buff,strlen(command_buff)))
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		Newmark_Error_Number = 123;
		sprintf(Newmark_Error_String,"Newmark_Command_Abort_Move:Arcom_ESS_Interface_Write failed.");
		return FALSE;	       
	}
	/* read reply */
	if(!Command_Read_Until_Prompt(class,source,handle,100,FALSE,NULL))
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		return FALSE;
	}
	/* unlock mutex */
	if(!Arcom_ESS_Interface_Mutex_Unlock(handle))
	{
		Newmark_Error_Number = 124;
		sprintf(Newmark_Error_String,"Newmark_Command_Abort_Move:Arcom_ESS_Interface_Mutex_Unlock failed.");
		return FALSE;	       
	}
#if LOGGING > 1
	Newmark_Log(class,source,LOG_VERBOSITY_INTERMEDIATE,"Newmark_Command_Abort_Move:Finished.");
#endif
	return TRUE;
}

/**
 * Get whether there is an error from the controller using the 'PRINT ERR' command.
 * @param class The class parameter for logging.
 * @param source The source parameter for logging.
 * @param handle The interface handle to the newmark controller.
 * @param error_exists The address of an integer to store a boolean : FALSE (0) No error exists, TRUE (1) Error exists.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #COMMAND_BUFF_LENGTH
 * @see #Command_Read_Flush
 * @see #Command_Read_Until_Prompt
 * @see #Newmark_Command_Fix_String
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Handle_T
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Mutex_Lock
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Mutex_Unlock
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Write
 */
int Newmark_Command_Err_Get(char *class,char *source,Arcom_ESS_Interface_Handle_T *handle,int *error_exists)
{
	char command_buff[COMMAND_BUFF_LENGTH];
	char true_false_string[32];
	char *reply_string = NULL;
	int retval;

	if(error_exists == NULL)
	{
		Newmark_Error_Number = 133;
		sprintf(Newmark_Error_String,"Newmark_Command_Err_Get:error_code was NULL.");
		return FALSE;	       
	}
#if LOGGING > 1
	Newmark_Log(class,source,LOG_VERBOSITY_INTERMEDIATE,"Newmark_Command_Err_Get:Start.");
#endif
	/* mutex */
	if(!Arcom_ESS_Interface_Mutex_Lock(handle))
	{
		Newmark_Error_Number = 134;
		sprintf(Newmark_Error_String,"Newmark_Command_Err_Get:Arcom_ESS_Interface_Mutex_Lock failed.");
		return FALSE;	       
	}
	/* clear any unread data */
	if(!Command_Read_Flush(class,source,handle))
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		return FALSE;
	}
	/* send PRINT POS */
	sprintf(command_buff,"PRINT ERR\r\n");
#if LOGGING > 5
	Newmark_Log_Format(class,source,LOG_VERBOSITY_INTERMEDIATE,"Newmark_Command_Err_Get:Writing '%s' to handle.",
			   Newmark_Command_Fix_String(command_buff));
#endif
	if(!Arcom_ESS_Interface_Write(class,source,handle,command_buff,strlen(command_buff)))
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		Newmark_Error_Number = 135;
		sprintf(Newmark_Error_String,"Newmark_Command_Err_Get:Arcom_ESS_Interface_Write failed.");
		return FALSE;	       
	}
	/* read reply 
	** If the loop timeout count is 100 (100*10 = 1000ms), this sometimes times out.
	** Increased timeout to 1000*10 = 10000ms. */
	if(!Command_Read_Until_Prompt(class,source,handle,1000,TRUE,&reply_string))
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		return FALSE;
	}
#if LOGGING > 5
	Newmark_Log_Format(class,source,LOG_VERBOSITY_INTERMEDIATE,"Newmark_Command_Err_Get:Read '%s' from handle.",
			   Newmark_Command_Fix_String(reply_string));
#endif
	/* parse reply string */
	retval = sscanf(reply_string,"PRINT ERR %31s >",true_false_string);
	if(retval != 1)
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		Newmark_Error_Number = 136;
		sprintf(Newmark_Error_String,"Newmark_Command_Err_Get:Illegal Reply '%s'.",reply_string);
		if(reply_string != NULL)
			free(reply_string);
		return FALSE;	       
	}
	if(strcmp(true_false_string,"TRUE") == 0)
		(*error_exists) = TRUE;
	else if(strcmp(true_false_string,"FALSE") == 0)
		(*error_exists) = FALSE;
	else
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		Newmark_Error_Number = 138;
		sprintf(Newmark_Error_String,"Newmark_Command_Err_Get:Illegal true/false string '%s' : Reply = '%s'.",
			true_false_string,reply_string);
		if(reply_string != NULL)
			free(reply_string);
		return FALSE;	       
	}
	if(reply_string != NULL)
		free(reply_string);
	/* unlock mutex */
	if(!Arcom_ESS_Interface_Mutex_Unlock(handle))
	{
		Newmark_Error_Number = 137;
		sprintf(Newmark_Error_String,"Newmark_Command_Err_Get:Arcom_ESS_Interface_Mutex_Unlock failed.");
		return FALSE;	       
	}
#if LOGGING > 1
	Newmark_Log_Format(class,source,LOG_VERBOSITY_INTERMEDIATE,
			   "Newmark_Command_Err_Get:Finished : error exists = %d.",(*error_exists));
#endif
	return TRUE;
}

/**
 * Get the last error code from the controller using the 'PRINT ERROR' command.
 * @param class The class parameter for logging.
 * @param source The source parameter for logging.
 * @param handle The interface handle to the newmark controller.
 * @param error_code The address of an integer to store the retrieved error code.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #COMMAND_BUFF_LENGTH
 * @see #Command_Read_Flush
 * @see #Command_Read_Until_Prompt
 * @see #Newmark_Command_Fix_String
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Handle_T
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Mutex_Lock
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Mutex_Unlock
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Write
 */
int Newmark_Command_Error_Get(char *class,char *source,Arcom_ESS_Interface_Handle_T *handle,int *error_code)
{
	char command_buff[COMMAND_BUFF_LENGTH];
	char *reply_string = NULL;
	int retval;

	if(error_code == NULL)
	{
		Newmark_Error_Number = 125;
		sprintf(Newmark_Error_String,"Newmark_Command_Error_Get:error_code was NULL.");
		return FALSE;	       
	}
#if LOGGING > 1
	Newmark_Log(class,source,LOG_VERBOSITY_INTERMEDIATE,"Newmark_Command_Error_Get:Start.");
#endif
	/* mutex */
	if(!Arcom_ESS_Interface_Mutex_Lock(handle))
	{
		Newmark_Error_Number = 126;
		sprintf(Newmark_Error_String,"Newmark_Command_Error_Get:Arcom_ESS_Interface_Mutex_Lock failed.");
		return FALSE;	       
	}
	/* clear any unread data */
	if(!Command_Read_Flush(class,source,handle))
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		return FALSE;
	}
	/* send PRINT POS */
	sprintf(command_buff,"PRINT ERROR\r\n");
#if LOGGING > 5
	Newmark_Log_Format(class,source,LOG_VERBOSITY_INTERMEDIATE,"Newmark_Command_Error_Get:Writing '%s' to handle.",
			   Newmark_Command_Fix_String(command_buff));
#endif
	if(!Arcom_ESS_Interface_Write(class,source,handle,command_buff,strlen(command_buff)))
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		Newmark_Error_Number = 127;
		sprintf(Newmark_Error_String,"Newmark_Command_Error_Get:Arcom_ESS_Interface_Write failed.");
		return FALSE;	       
	}
	/* read reply */
	if(!Command_Read_Until_Prompt(class,source,handle,100,TRUE,&reply_string))
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		return FALSE;
	}
#if LOGGING > 5
	Newmark_Log_Format(class,source,LOG_VERBOSITY_INTERMEDIATE,"Newmark_Command_Error_Get:Read '%s' from handle.",
			   Newmark_Command_Fix_String(reply_string));
#endif
	/* parse reply string */
	retval = sscanf(reply_string,"PRINT ERROR %d >",error_code);
	if(retval != 1)
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		Newmark_Error_Number = 128;
		sprintf(Newmark_Error_String,"Newmark_Command_Error_Get:Illegal Reply '%s'.",reply_string);
		if(reply_string != NULL)
			free(reply_string);
		return FALSE;	       
	}
	if(reply_string != NULL)
		free(reply_string);
	/* unlock mutex */
	if(!Arcom_ESS_Interface_Mutex_Unlock(handle))
	{
		Newmark_Error_Number = 129;
		sprintf(Newmark_Error_String,"Newmark_Command_Error_Get:Arcom_ESS_Interface_Mutex_Unlock failed.");
		return FALSE;	       
	}
#if LOGGING > 1
	Newmark_Log_Format(class,source,LOG_VERBOSITY_INTERMEDIATE,
			   "Newmark_Command_Error_Get:Finished with error code %d.",(*error_code));
#endif
	return TRUE;
}

/**
 * Reset the controller's error code using 'ERROR = 0' command.
 * @param class The class parameter for logging.
 * @param source The source parameter for logging.
 * @param handle The interface handle to the newmark controller.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #COMMAND_BUFF_LENGTH
 * @see #Command_Read_Flush
 * @see #Command_Read_Until_Prompt
 * @see #Newmark_Command_Fix_String
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Handle_T
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Mutex_Lock
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Mutex_Unlock
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Write
 */
int Newmark_Command_Error_Reset(char *class,char *source,Arcom_ESS_Interface_Handle_T *handle)
{
	char command_buff[COMMAND_BUFF_LENGTH];
	char *reply_string = NULL;
	int retval,error_code;

#if LOGGING > 1
	Newmark_Log(class,source,LOG_VERBOSITY_INTERMEDIATE,"Newmark_Command_Error_Reset:Start.");
#endif
	/* mutex */
	if(!Arcom_ESS_Interface_Mutex_Lock(handle))
	{
		Newmark_Error_Number = 139;
		sprintf(Newmark_Error_String,"Newmark_Command_Error_Reset:Arcom_ESS_Interface_Mutex_Lock failed.");
		return FALSE;	       
	}
	/* clear any unread data */
	if(!Command_Read_Flush(class,source,handle))
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		return FALSE;
	}
	/* send PRINT POS */
	sprintf(command_buff,"ERROR = 0\r\n");
#if LOGGING > 5
	Newmark_Log_Format(class,source,LOG_VERBOSITY_INTERMEDIATE,
			   "Newmark_Command_Error_Reset:Writing '%s' to handle.",
			   Newmark_Command_Fix_String(command_buff));
#endif
	if(!Arcom_ESS_Interface_Write(class,source,handle,command_buff,strlen(command_buff)))
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		Newmark_Error_Number = 140;
		sprintf(Newmark_Error_String,"Newmark_Command_Error_Reset:Arcom_ESS_Interface_Write failed.");
		return FALSE;	       
	}
	/* read reply */
	if(!Command_Read_Until_Prompt(class,source,handle,100,TRUE,&reply_string))
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		return FALSE;
	}
#if LOGGING > 5
	Newmark_Log_Format(class,source,LOG_VERBOSITY_INTERMEDIATE,
			   "Newmark_Command_Error_Reset:Read '%s' from handle.",
			   Newmark_Command_Fix_String(reply_string));
#endif
	/* parse reply string */
	retval = sscanf(reply_string,"ERROR = %d >",&error_code);
	if(retval != 1)
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		Newmark_Error_Number = 141;
		sprintf(Newmark_Error_String,"Newmark_Command_Error_Reset:Illegal Reply '%s'.",reply_string);
		if(reply_string != NULL)
			free(reply_string);
		return FALSE;	       
	}
	if(reply_string != NULL)
		free(reply_string);
	if(error_code != 0)
	{
		Arcom_ESS_Interface_Mutex_Unlock(handle);
		Newmark_Error_Number = 142;
		sprintf(Newmark_Error_String,"Newmark_Command_Error_Reset:Error code not reset to 0 : Reply '%s'.",
			reply_string);
		return FALSE;	       
	}
	/* unlock mutex */
	if(!Arcom_ESS_Interface_Mutex_Unlock(handle))
	{
		Newmark_Error_Number = 143;
		sprintf(Newmark_Error_String,"Newmark_Command_Error_Reset:Arcom_ESS_Interface_Mutex_Unlock failed.");
		return FALSE;	       
	}
#if LOGGING > 1
	Newmark_Log(class,source,LOG_VERBOSITY_INTERMEDIATE,"Newmark_Command_Error_Reset:Finished.");
#endif
	return TRUE;
}

/**
 * Set the position tolerance of the move command.
 * @param class The class parameter for logging.
 * @param source The source parameter for logging.
 * @param mm The position tolerance in mm. Values from 0.0 to 1.0 are allowed.
 * @return The routine returns true if the tolerance was in range, false otherwise.
 * @see #Position_Tolerance
 */
int Newmark_Command_Position_Tolerance_Set(char *class,char *source,double mm)
{
#if LOGGING > 1
	Newmark_Log(class,source,LOG_VERBOSITY_INTERMEDIATE,"Newmark_Command_Position_Tolerance_Set:Started.");
#endif
	if((mm < 0.0) || (mm > 1.0))
	{
		Newmark_Error_Number = 147;
		sprintf(Newmark_Error_String,"Newmark_Command_Position_Tolerance_Set:"
			"tolerance %.6f out of range 0.0..1.0.",mm);
		return FALSE;	       
	}
	Position_Tolerance = mm;
#if LOGGING > 1
	Newmark_Log_Format(class,source,LOG_VERBOSITY_INTERMEDIATE,
			   "Newmark_Command_Position_Tolerance_Set:Set to %.6f.",Position_Tolerance);
#endif
#if LOGGING > 1
	Newmark_Log(class,source,LOG_VERBOSITY_INTERMEDIATE,"Newmark_Command_Position_Tolerance_Set:Finished.");
#endif
	return TRUE;
}

/* =======================================
**  internal functions 
** ======================================= */
/**
 * Routine to read from the handle until nothing more is read (any read buffers are flushed).
 * @param class The class parameter for logging.
 * @param source The source parameter for logging.
 * @param handle The interface handle to read the data from.
 * @see #Newmark_Command_Fix_String
 * @see #COMMAND_BUFF_LENGTH
 * @see #ONE_MILLISECOND_NS
 * @see #COMMAND_READ_LOOP_SLEEP_MS
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Read
 */
static int Command_Read_Flush(char *class,char *source,Arcom_ESS_Interface_Handle_T *handle)
{
	char command_buff[COMMAND_BUFF_LENGTH];
	struct timespec sleep_time;
	int done,retval,bytes_read,sleep_errno;

#if LOGGING > 5
	Newmark_Log(class,source,LOG_VERBOSITY_VERY_VERBOSE,"Command_Read_Flush:Start.");
#endif
	done = FALSE;
	while(done == FALSE)
	{
		/* sleep a bit (10ms) */
		sleep_time.tv_sec = 0;
		sleep_time.tv_nsec = COMMAND_READ_LOOP_SLEEP_MS*ONE_MILLISECOND_NS;
		retval = nanosleep(&sleep_time,NULL);
		if(retval != 0)
		{
			sleep_errno = errno;
			Newmark_Error_Number = 111;
			sprintf(Newmark_Error_String,"Command_Read_Flush:nanosleep failed.");
			/* non terminal error - just log and continue. */
			Newmark_Error();
		}
#if LOGGING > 10
		Newmark_Log(class,source,LOG_VERBOSITY_VERY_VERBOSE,"Command_Read_Flush:Arcom_ESS_Interface_Read.");
#endif
		/* read from interface */
		if(!Arcom_ESS_Interface_Read(class,source,handle,command_buff,COMMAND_BUFF_LENGTH,&bytes_read))
		{
			Newmark_Error_Number = 112;
			sprintf(Newmark_Error_String,"Command_Read_Flush:Arcom_ESS_Interface_Read failed.");
			return FALSE;
		}
#if LOGGING > 10
		Newmark_Log_Format(class,source,LOG_VERBOSITY_VERY_VERBOSE,"Command_Read_Flush:"
				   "Arcom_ESS_Interface_Read read %d bytes.",bytes_read);
#endif
		/* have we read anything? */
		if(bytes_read > 0)
		{
			/* terminate buffer */
			command_buff[bytes_read] = '\0';
#if LOGGING > 9
			Newmark_Log_Format(class,source,LOG_VERBOSITY_VERY_VERBOSE,
					   "Command_Read_Flush:Last Read '%s'.",
					   Newmark_Command_Fix_String(command_buff));
#endif
		}/* if we read something */
		else /* nothing to read - terminate */
			done = TRUE;
	}/* end while */
#if LOGGING > 5
	Newmark_Log(class,source,LOG_VERBOSITY_VERY_VERBOSE,"Command_Read_Flush:Finished.");
#endif
	return TRUE;
}
/**
 * Routine to read from the handle until the COMMAND_PROMPT is read.
 * @param class The class parameter for logging.
 * @param source The source parameter for logging.
 * @param handle The interface handle to read the data from.
 * @param loop_timeout_count How many times round the read loop before we timeout - each loop has a 
 *        COMMAND_READ_LOOP_SLEEP_MS in it.
 * @param timeout_is_error Boolean, whether to return an error on timeout or not.
 * @param read_string The address of a string pointer. This can be NULL, if the read details do not need to be kept.
 *        If it is not NULL, and read characters (including the prompt if read) are added to this string by memory
 *        allocation. If non-null, this pointer will need to be freed on return from this routine.
 * @return The routine returns TRUE on success and FALSE if an error occured.
 * @see #COMMAND_PROMPT
 * @see #Newmark_Command_Fix_String
 * @see #COMMAND_BUFF_LENGTH
 * @see #ONE_MILLISECOND_NS
 * @see #COMMAND_READ_LOOP_SLEEP_MS
 * @see newmark_general.html#Newmark_Add_To_String
 * @see http://ltdevsrv.livjm.ac.uk/~dev/arcom_ess/cdocs/arcom_ess_interface.html#Arcom_ESS_Interface_Read
 */
static int Command_Read_Until_Prompt(char *class,char *source,Arcom_ESS_Interface_Handle_T *handle,
				     int loop_timeout_count,int timeout_is_error,char **read_string)
{
	char command_buff[COMMAND_BUFF_LENGTH];
	struct timespec sleep_time;
	int done,retval,bytes_read,sleep_errno,timeout_index;

#if LOGGING > 5
	Newmark_Log(class,source,LOG_VERBOSITY_VERY_VERBOSE,"Command_Read_Until_Prompt:Start.");
#endif
	timeout_index = 0;
	done = FALSE;
	while(done == FALSE)
	{
#if LOGGING > 10
		Newmark_Log(class,source,LOG_VERBOSITY_VERY_VERBOSE,
			    "Command_Read_Until_Prompt:Arcom_ESS_Interface_Read.");
#endif
		/* read from interface */
		if(!Arcom_ESS_Interface_Read(class,source,handle,command_buff,COMMAND_BUFF_LENGTH,&bytes_read))
		{
			Newmark_Error_Number = 104;
			sprintf(Newmark_Error_String,"Command_Read_Until_Prompt:Arcom_ESS_Interface_Read failed.");
			return FALSE;
		}
#if LOGGING > 10
		Newmark_Log_Format(class,source,LOG_VERBOSITY_VERY_VERBOSE,"Command_Read_Until_Prompt:"
				   "Arcom_ESS_Interface_Read read %d bytes.",bytes_read);
#endif
		/* have we read anything? */
		if(bytes_read > 0)
		{
			/* terminate buffer */
			command_buff[bytes_read] = '\0';
#if LOGGING > 9
			Newmark_Log_Format(class,source,LOG_VERBOSITY_VERY_VERBOSE,
					   "Command_Read_Until_Prompt:Last Read '%s'.",
					   Newmark_Command_Fix_String(command_buff));
#endif
			/* have we read a command prompt. If so, we are ready to stop */
			if(strchr(command_buff,COMMAND_PROMPT) != NULL)
			{
				done = TRUE;
#if LOGGING > 9
				Newmark_Log(class,source,LOG_VERBOSITY_VERY_VERBOSE,
					    "Command_Read_Until_Prompt:Found Prompt.");
#endif
			}
			/* If the read_string is not null, add the command_buff to it */
			if(read_string != NULL)
			{
				if(!Newmark_Add_To_String(read_string,command_buff))
					return FALSE;
#if LOGGING > 9
				Newmark_Log_Format(class,source,LOG_VERBOSITY_VERY_VERBOSE,"Command_Read_Until_Prompt:"
						   "Total Read '%s'.",Newmark_Command_Fix_String((*read_string)));
#endif
			}
		}/* if we read something */
		else /* increment timeout index */
			timeout_index++;
		/* have we timed out ? */
		if(timeout_index >= loop_timeout_count)
		{
			if(timeout_is_error)
			{
				Newmark_Error_Number = 105;
				sprintf(Newmark_Error_String,"Command_Read_Until_Prompt:Readout timed out(%d).",
					loop_timeout_count);
				return FALSE;
			}
			else
				done = TRUE;
		}
		/* sleep a bit (10ms) */
		sleep_time.tv_sec = 0;
		sleep_time.tv_nsec = COMMAND_READ_LOOP_SLEEP_MS*ONE_MILLISECOND_NS;
		retval = nanosleep(&sleep_time,NULL);
		if(retval != 0)
		{
			sleep_errno = errno;
			Newmark_Error_Number = 106;
			sprintf(Newmark_Error_String,"Command_Read_Until_Prompt:nanosleep failed.");
			/* non terminal error - just log and continue. */
			Newmark_Error();
		}
	}/* end while */
#if LOGGING > 5
	Newmark_Log(class,source,LOG_VERBOSITY_VERY_VERBOSE,"Command_Read_Until_Prompt:Finished.");
	if((read_string != NULL)&&((*read_string) != NULL))
		Newmark_Log_Format(class,source,LOG_VERBOSITY_VERY_VERBOSE,"Command_Read_Until_Prompt:Read '%s'.",
				   Newmark_Command_Fix_String((*read_string)));
#endif
	return TRUE;
}

/**
 * Routine to fix some common control characters in the specified string, and replace them with a printable
 * representation.
 * @param string The initial string. This is <b>not</b> modified.
 * @return The routine returns the string, with certain control codes replaced.
 *        Note the string is internally static, and is over-written on each call to this routine. If not
 *        immediately used, it should be copied. The routine can return NULL if an error occured.
 * @see #LOG_BUFF_LENGTH
 */
static char *Newmark_Command_Fix_String(char *string)
{
	static char return_string[LOG_BUFF_LENGTH];
	char *ptr = NULL;
	char *find_string = NULL,*replace_string = NULL;
	int done,start_index,end_index,i,move_count,find_string_index;
	char *find_string_list[] = {"\r","\n"};
	char *replace_string_list[] = {"<cr>","<lf>"};
	int find_string_count = 2;

	if(string == NULL)
	{
		Newmark_Error_Number = 144;
		sprintf(Newmark_Error_String,"Newmark_Command_Fix_String:string is null.");
		return NULL;
	}
	if(strlen(string) >= LOG_BUFF_LENGTH)
	{
		Newmark_Error_Number = 145;
		sprintf(Newmark_Error_String,"Newmark_Command_Fix_String:string was too long (%d,%d).",
			strlen(string),LOG_BUFF_LENGTH);
		return NULL;
	}
	strcpy(return_string,string);
	for(find_string_index = 0; find_string_index < find_string_count;find_string_index++)
	{
		done = FALSE;
		find_string = find_string_list[find_string_index];
		replace_string = replace_string_list[find_string_index];
		move_count = (int)(strlen(replace_string)-strlen(find_string));
		while(done == FALSE)
		{
			ptr = strstr(return_string,find_string);
			if(ptr != NULL)
			{
				start_index = (int)(ptr-return_string);
				end_index = strlen(return_string);
				if(move_count > 0)
				{
					if((strlen(return_string) + move_count) > LOG_BUFF_LENGTH)
					{
						Newmark_Error_Number = 146;
						sprintf(Newmark_Error_String,
						   "Newmark_Command_Fix_String:string is too short ((%d + %d) > %d).",
							strlen(return_string),move_count,LOG_BUFF_LENGTH);
						return NULL;
					}
					for(i=end_index;i>start_index;i--)
						return_string[i+move_count] = return_string[i];
				}
				else
				{
					for(i=start_index;i<end_index;i++)
						return_string[i] = return_string[i+move_count];
				}
				strncpy(return_string+start_index,replace_string,strlen(replace_string));
			}
			else
				done = TRUE;
		}/* end while */
	}/* end for */
	return return_string;
}

/*
** $Log: not supported by cvs2svn $
** Revision 1.3  2010/12/09 14:48:48  cjm
** Changed timeout count in call to Command_Read_Until_Prompt in Newmark_Command_Err_Get.
** This means the software waits for ~10s for a reply to PRINT ERR rather than ~1s, as it was found
** the software sometimes times out awaiting that reply.
**
** Revision 1.2  2009/02/05 11:41:03  cjm
** Swapped Bitwise for Absolute logging levels.
**
** Revision 1.1  2008/11/20 11:35:45  cjm
** Initial revision
**
*/


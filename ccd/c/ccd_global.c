/* ccd_global.c -*- mode: Fundamental;-*-
** low level ccd library
** $Header: /home/cjm/cvs/frodospec/ccd/c/ccd_global.c,v 0.7 2001-04-17 09:37:55 cjm Exp $
*/
/**
 * ccd_global.c contains routines that tie together all the modules that make up libccd.
 * @author SDSU, Chris Mottram
 * @version $Revision: 0.7 $
 */
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
#define _POSIX_SOURCE 1
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include "ccd_global.h"
#include "ccd_pci.h"
#include "ccd_text.h"
#include "ccd_interface.h"
#include "ccd_dsp.h"
#include "ccd_exposure.h"
#include "ccd_temperature.h"
#include "ccd_setup.h"
#include "ccd_filter_wheel.h"

/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id: ccd_global.c,v 0.7 2001-04-17 09:37:55 cjm Exp $";

/* data types */
/**
 * Data type holding local data to ccd_global. This consists of the following:
 * <dl>
 * <dt>Global_Log_Handler</dt> <dd>Function pointer to the routine that will log messages passed to it.</dd>
 * <dt>Global_Log_Filter</dt> <dd>Function pointer to the routine that will filter log messages passed to it.
 * 		The funtion will return TRUE if the message should be logged, and FALSE if it shouldn't.</dd>
 * <dt>Global_Log_Filter_Level</dt> <dd>A globally maintained log filter level. 
 * 		This is set using CCD_Global_Set_Log_Filter_Level.
 * 		CCD_Global_Log_Filter_Level_Absolute and CCD_Global_Log_Filter_Level_Bitwise test it against
 * 		message levels to determine whether to log messages.</dd>
 * </dl>
 * @see #CCD_Global_Log
 * @see #CCD_Global_Set_Log_Filter_Level
 * @see #CCD_Global_Log_Filter_Level_Absolute
 * @see #CCD_Global_Log_Filter_Level_Bitwise
 */
struct Global_Struct
{
	void (*Global_Log_Handler)(int level,char *string);
	int (*Global_Log_Filter)(int level,char *string);
	int Global_Log_Filter_Level;
};

/* internal data */
/**
 * The instance of Global_Struct that contains local data for this module.
 * @see #Global_Struct
 */
static struct Global_Struct Global_Data = {NULL,NULL,0};
/**
 * General buffer used for string formatting during logging.
 * @see #CCD_GLOBAL_ERROR_STRING_LENGTH
 */
static char Global_Buff[CCD_GLOBAL_ERROR_STRING_LENGTH];

/* ----------------------------------------------------------------------------
** 		external functions 
** ---------------------------------------------------------------------------- */
/**
 * This routine calls all initialisation routines for modules in libccd. These routines are generally used to
 * initialise parts of the library. This routine should be called at the start of a program.
 * <i>
 * Note some of the initialisation can produce errors. Currently these are just printed out, this should
 * perhaps return an error code when this occurs.
 * </i>
 * @param interface_device The device the library will write DSP commands to. The routine calls
 * 	<a href="ccd_interface.html#CCD_Interface_Set_Device">CCD_Interface_Set_Device</a> to set the
 * 	device.
 * @see #Global_Data
 * @see ccd_interface.html#CCD_Interface_Set_Device
 * @see ccd_interface.html#CCD_Interface_Initialise
 * @see ccd_dsp.html#CCD_DSP_Initialise
 * @see ccd_exposure.html#CCD_Exposure_Initialise
 * @see ccd_setup.html#CCD_Setup_Initialise
 * @see ccd_filter_wheel.html#CCD_Filter_Wheel_Initialise
 */
void CCD_Global_Initialise(enum CCD_INTERFACE_DEVICE_ID interface_device)
{
	/* try to set which device the library is going to talk to */
	if(!CCD_Interface_Set_Device(interface_device))
		CCD_Interface_Error();
	CCD_Interface_Initialise();
	if(!CCD_DSP_Initialise())
		CCD_DSP_Error();
	CCD_Exposure_Initialise();
	CCD_Setup_Initialise();
	if(!CCD_Filter_Wheel_Initialise())
		CCD_Filter_Wheel_Error();
/* print some compile time information to stdout */
	fprintf(stdout,"CCD_Global_Initialise:%s.\n",rcsid);
}

/**
 * A general error routine. This checks the error numbers for all the modules that make up the library, and
 * for any non-zero numbers prints out the error message to stderr.
 * <b>Note</b> you cannot call both CCD_Global_Error and CCD_Global_Error_String to print the error string and 
 * get a string copy of it, only one of the error routines can be called after libccd has generated an error.
 * A second call to one of these routines will generate a 'Error not found' error!.
 * @see ccd_filter_wheel.html#CCD_Filter_Wheel_Get_Error_Number
 * @see ccd_filter_wheel.html#CCD_Filter_Wheel_Error
 * @see ccd_setup.html#CCD_Setup_Get_Error_Number
 * @see ccd_setup.html#CCD_Setup_Error
 * @see ccd_exposure.html#CCD_Exposure_Get_Error_Number
 * @see ccd_exposure.html#CCD_Exposure_Error
 * @see ccd_temperature.html#CCD_Temperature_Get_Error_Number
 * @see ccd_temperature.html#CCD_Temperature_Error
 * @see ccd_dsp.html#CCD_DSP_Get_Error_Number
 * @see ccd_dsp.html#CCD_DSP_Error
 * @see ccd_interface.html#CCD_Interface_Get_Error_Number
 * @see ccd_interface.html#CCD_Interface_Error
 * @see ccd_pci.html#CCD_PCI_Get_Error_Number
 * @see ccd_pci.html#CCD_PCI_Error
 */
void CCD_Global_Error(void)
{
	int found = FALSE;

	if(CCD_Filter_Wheel_Get_Error_Number() != 0)
	{
		found = TRUE;
		CCD_Filter_Wheel_Error();
	}
	if(CCD_Setup_Get_Error_Number() != 0)
	{
		found = TRUE;
		CCD_Setup_Error();
	}
	if(CCD_Exposure_Get_Error_Number() != 0)
	{
		found = TRUE;
		CCD_Exposure_Error();
	}
	if(CCD_Temperature_Get_Error_Number() != 0)
	{
		found = TRUE;
		fprintf(stderr,"\t");
		CCD_Temperature_Error();
	}
	if(CCD_DSP_Get_Error_Number() != 0)
	{
		found = TRUE;
		fprintf(stderr,"\t");
		CCD_DSP_Error();
	}
	if(CCD_Interface_Get_Error_Number() != 0)
	{
		found = TRUE;
		fprintf(stderr,"\t\t");
		CCD_Interface_Error();
	}
	if(CCD_PCI_Get_Error_Number() != 0)
	{
		found = TRUE;
		fprintf(stderr,"\t\t\t");
		CCD_PCI_Error();
	}
	if(CCD_Text_Get_Error_Number() != 0)
	{
		found = TRUE;
		fprintf(stderr,"\t\t\t");
		CCD_Text_Error();
	}
	if(!found)
	{
		fprintf(stderr,"Error:CCD_Global_Error:Error not found\n");
	}
}

/**
 * A general error routine. This checks the error numbers for all the modules that make up the library, and
 * for any non-zero numbers adds the error message to a passed in string. The string paramater is set to the
 * blank string initially.
 * <b>Note</b> you cannot call both CCD_Global_Error and CCD_Global_Error_String to print the error string and 
 * get a string copy of it, only one of the error routines can be called after libccd has generated an error.
 * A second call to one of these routines will generate a 'Error not found' error!.
 * @param error_string A character buffer big enough to store the longest possible error message. It is
 * recomended that it is at least 1024 bytes in size.
 * @see ccd_filter_wheel.html#CCD_Filter_Wheel_Get_Error_Number
 * @see ccd_filter_wheel.html#CCD_Filter_Wheel_Error_String
 * @see ccd_setup.html#CCD_Setup_Get_Error_Number
 * @see ccd_setup.html#CCD_Setup_Error_String
 * @see ccd_exposure.html#CCD_Exposure_Get_Error_Number
 * @see ccd_exposure.html#CCD_Exposure_Error_String
 * @see ccd_temperature.html#CCD_Temperature_Get_Error_Number
 * @see ccd_temperature.html#CCD_Temperature_Error_String
 * @see ccd_dsp.html#CCD_DSP_Get_Error_Number
 * @see ccd_dsp.html#CCD_DSP_Error_String
 * @see ccd_interface.html#CCD_Interface_Get_Error_Number
 * @see ccd_interface.html#CCD_Interface_Error_String
 * @see ccd_pci.html#CCD_PCI_Get_Error_Number
 * @see ccd_pci.html#CCD_PCI_Error_String
 */
void CCD_Global_Error_String(char *error_string)
{
	strcpy(error_string,"");
	if(CCD_Filter_Wheel_Get_Error_Number() != 0)
	{
		CCD_Filter_Wheel_Error_String(error_string);
	}
	if(CCD_Setup_Get_Error_Number() != 0)
	{
		CCD_Setup_Error_String(error_string);
	}
	if(CCD_Exposure_Get_Error_Number() != 0)
	{
		CCD_Exposure_Error_String(error_string);
	}
	if(CCD_Temperature_Get_Error_Number() != 0)
	{
		strcat(error_string,"\t");
		CCD_Temperature_Error_String(error_string);
	}
	if(CCD_DSP_Get_Error_Number() != 0)
	{
		strcat(error_string,"\t");
		CCD_DSP_Error_String(error_string);
	}
	if(CCD_Interface_Get_Error_Number() != 0)
	{
		strcat(error_string,"\t\t");
		CCD_Interface_Error_String(error_string);
	}
	if(CCD_PCI_Get_Error_Number() != 0)
	{
		strcat(error_string,"\t\t\t");
		CCD_PCI_Error_String(error_string);
	}
	if(CCD_Text_Get_Error_Number() != 0)
	{
		strcat(error_string,"\t\t\t");
		CCD_Text_Error_String(error_string);
	}
	if(strlen(error_string) == 0)
	{
		strcat(error_string,"Error:CCD_Global_Error:Error not found\n");
	}
}

/**
 * Routine to get the current time in a string. The string is returned in the format
 * '01/01/2000 13:59:59', or the string "Unknown time" if the routine failed.
 * The time is in UTC.
 * @param time_string The string to fill with the current time.
 * @param string_length The length of the buffer passed in. It is recommended the length is at least 20 characters.
 */
void CCD_Global_Get_Current_Time_String(char *time_string,int string_length)
{
	time_t current_time;
	struct tm *utc_time = NULL;

	if(time(&current_time) > -1)
	{
		utc_time = gmtime(&current_time);
		strftime(time_string,string_length,"%d/%m/%Y %H:%M:%S",utc_time);
	}
	else
		strncpy(time_string,"Unknown time",string_length);
}

/**
 * Routine to log a message to a defined logging mechanism. This routine has an arbitary number of arguments,
 * and uses vsprintf to format them i.e. like fprintf. The Global_Buff is used to hold the created string,
 * therefore the total length of the generated string should not be longer than CCD_GLOBAL_ERROR_STRING_LENGTH.
 * CCD_Global_Log is then called to handle the log message.
 * @param level An integer, used to decide whether this particular message has been selected for
 * 	logging or not.
 * @param format A string, with formatting statements the same as fprintf would use to determine the type
 * 	of the following arguments.
 * @see #CCD_Global_Log
 * @see #Global_Buff
 * @see #CCD_GLOBAL_ERROR_STRING_LENGTH
 */
void CCD_Global_Log_Format(int level,char *format,...)
{
	va_list ap;

/* format the arguments */
	va_start(ap,format);
	vsprintf(Global_Buff,format,ap);
	va_end(ap);
/* call the log routine to log the results */
	CCD_Global_Log(level,Global_Buff);
}

/**
 * Routine to log a message to a defined logging mechanism. If the string or Global_Data.Global_Log_Handler are NULL
 * the routine does not log the message. If the Global_Data.Global_Log_Filter function pointer is non-NULL, the
 * message is passed to it to determoine whether to log the message.
 * @param level An integer, used to decide whether this particular message has been selected for
 * 	logging or not.
 * @param string The message to log.
 * @see #Global_Data
 */
void CCD_Global_Log(int level,char *string)
{
/* If the string is NULL, don't log. */
	if(string == NULL)
		return;
/* If there is no log handler, return */
	if(Global_Data.Global_Log_Handler == NULL)
		return;
/* If there's a log filter, check it returns TRUE for this message */
	if(Global_Data.Global_Log_Filter != NULL)
	{
		if(Global_Data.Global_Log_Filter(level,string) == FALSE)
			return;
	}
/* We can log the message */
	(*Global_Data.Global_Log_Handler)(level,string);
}

/**
 * Routine to set the Global_Data.Global_Log_Handler used by CCD_Global_Log.
 * @param log_fn A function pointer to a suitable handler.
 * @see #Global_Data
 * @see #CCD_Global_Log
 */
void CCD_Global_Set_Log_Handler_Function(void (*log_fn)(int level,char *string))
{
	Global_Data.Global_Log_Handler = log_fn;
}

/**
 * Routine to set the Global_Data.Global_Log_Filter used by CCD_Global_Log.
 * @param log_fn A function pointer to a suitable filter function.
 * @see #Global_Data
 * @see #CCD_Global_Log
 */
void CCD_Global_Set_Log_Filter_Function(int (*filter_fn)(int level,char *string))
{
	Global_Data.Global_Log_Filter = filter_fn;
}

/**
 * A log handler to be used for the Global_Data.Global_Log_Handler function.
 * Just prints the message to stdout, terminated by a newline.
 * @param level The log level for this message.
 * @param string The log message to be logged. 
 */
void CCD_Global_Log_Handler_Stdout(int level,char *string)
{
	if(string == NULL)
		return;
	fprintf(stdout,"%s\n",string);
}

/**
 * Routine to set the Global_Data.Global_Log_Filter_Level.
 * @see #Global_Data
 */
void CCD_Global_Set_Log_Filter_Level(int level)
{
	Global_Data.Global_Log_Filter_Level = level;
}

/**
 * A log message filter routine, to be used for the Global_Data.Global_Log_Filter function pointer.
 * @param level The log level of the message to be tested.
 * @param string The log message to be logged, not used in this filter. 
 * @return The routine returns TRUE if the level is less than or equal to the Global_Data.Global_Log_Filter_Level,
 * 	otherwise it returns FALSE.
 * @see #Global_Data
 */
int CCD_Global_Log_Filter_Level_Absolute(int level,char *string)
{
	return (level <= Global_Data.Global_Log_Filter_Level);
}

/**
 * A log message filter routine, to be used for the Global_Data.Global_Log_Filter function pointer.
 * @param level The log level of the message to be tested.
 * @param string The log message to be logged, not used in this filter. 
 * @return The routine returns TRUE if the level has bits set that are also set in the 
 * 	Global_Data.Global_Log_Filter_Level, otherwise it returns FALSE.
 * @see #Global_Data
 */
int CCD_Global_Log_Filter_Level_Bitwise(int level,char *string)
{
	return ((level & Global_Data.Global_Log_Filter_Level) > 0);
}

/*
** $Log: not supported by cvs2svn $
** Revision 0.6  2000/12/19 16:23:56  cjm
** Added filter wheel module calls.
**
** Revision 0.5  2000/06/13 17:14:13  cjm
** Changes to make Ccs agree with voodoo.
**
** Revision 0.4  2000/04/13 12:57:55  cjm
** Added CCD_Global_Get_Current_Time_String.
**
** Revision 0.3  2000/03/08 18:27:11  cjm
** CCD_DSP_Initialise now returns success.
**
** Revision 0.2  2000/03/01 15:44:41  cjm
** Backup.
**
** Revision 0.1  2000/01/25 14:57:27  cjm
** initial revision (PCI version).
**
*/

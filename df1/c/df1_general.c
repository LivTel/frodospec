/* df1_general.c
** FrodoSpec Micrologix 1100 df1 library
** $Header: /home/cjm/cvs/frodospec/df1/c/df1_general.c,v 1.1 2023-03-21 14:34:10 cjm Exp $
*/
/**
 * Error and Log handlers.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include "df1_general.h"

/* defines */
/**
 * How long some buffers are when generating logging messages.
 */
#define LOG_BUFF_LENGTH           (1024)

/* data types */
/**
 * Data type holding local data to filter_wheel_general. This consists of the following:
 * <dl>
 * <dt>Log_Handler</dt> <dd>Function pointer to the routine that will log messages passed to it.</dd>
 * <dt>Log_Filter</dt> <dd>Function pointer to the routine that will filter log messages passed to it.
 * 		The funtion will return TRUE if the message should be logged, and FALSE if it shouldn't.</dd>
 * <dt>Log_Filter_Level</dt> <dd>A globally maintained log filter level. 
 * 		This is set using Df1_Set_Log_Filter_Level.
 * 		Df1_Log_Filter_Level_Absolute and Df1_Log_Filter_Level_Bitwise 
 *              test it against message levels to determine whether to log messages.</dd>
 * </dl>
 * @see #Df1_Log
 * @see #Df1_Set_Log_Filter_Level
 * @see #Df1_Log_Filter_Level_Absolute
 * @see #Df1_Log_Filter_Level_Bitwise
 */

struct General_Struct
{
	void (*Log_Handler)(int level,char *string);
	int (*Log_Filter)(int level,char *string);
	int Log_Filter_Level;
};


/* external variables */
/**
 * The error number.
 */
int Df1_Error_Number = 0;
/**
 * The error string.
 * @see #DF1_ERROR_LENGTH
 */
char Df1_Error_String[DF1_ERROR_LENGTH];

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id: df1_general.c,v 1.1 2023-03-21 14:34:10 cjm Exp $";
/**
 * The instance of General_Struct that contains local data for this module.
 * This is statically initialised to the following:
 * <dl>
 * <dt>Log_Handler</dt> <dd>NULL</dd>
 * <dt>Log_Filter</dt> <dd>NULL</dd>
 * <dt>Log_Filter_Level</dt> <dd>0</dd>
 * </dl>
 * @see #General_Struct
 */
static struct General_Struct General_Data = 
{
	NULL,NULL,0,
};

/* external functions */
/**
 * Basic error reporting routine, to stderr.
 * @see #Df1_Error_Number
 * @see #Df1_Error_String
 * @see #Df1_Get_Current_Time_String
 */
void Df1_Error(void)
{
	char time_string[32];

	Df1_Get_Current_Time_String(time_string,32);
	if(Df1_Error_Number == 0)
		sprintf(Df1_Error_String,"%s Df1:An unknown error has occured.",time_string);
	fprintf(stderr,"%s Df1:Error(%d) : %s\n",time_string,Df1_Error_Number,
		Df1_Error_String);
}

/**
 * Basic error reporting routine, to the specified string.
 * @param error_string Pointer to an already allocated area of memory, to store the generated error string. 
 *        This should be at least 256 bytes long.
 * @see #Df1_Error_Number
 * @see #Df1_Error_String
 * @see #Df1_Get_Current_Time_String
 */
void Df1_Error_To_String(char *error_string)
{
	char time_string[32];

	strcpy(error_string,"");
	Df1_Get_Current_Time_String(time_string,32);
	if(Df1_Error_Number != 0)
	{
		sprintf(error_string+strlen(error_string),"%s Df1:Error(%d) : %s\n",time_string,
			Df1_Error_Number,Df1_Error_String);
	}
	if(strlen(error_string) == 0)
	{
		sprintf(error_string,"%s Error:Df1:Error not found\n",time_string);
	}
}

/**
 * Routine to return the current value of the error number.
 * @return The value of Df1_Error_Number.
 * @see #Df1_Error_Number
 */
int Df1_Get_Error_Number(void)
{
	return Df1_Error_Number;
}

/**
 * Routine to get the current time in a string. The string is returned in the format
 * '01/01/2000 13:59:59', or the string "Unknown time" if the routine failed.
 * The time is in UTC.
 * @param time_string The string to fill with the current time.
 * @param string_length The length of the buffer passed in. It is recommended the length is at least 20 characters.
 */
void Df1_Get_Current_Time_String(char *time_string,int string_length)
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
 * and uses vsprintf to format them i.e. like fprintf. 
 * Df1_Log is then called to handle the log message.
 * @param level An integer, used to decide whether this particular message has been selected for
 * 	logging or not.
 * @param format A string, with formatting statements the same as fprintf would use to determine the type
 * 	of the following arguments.
 * @see #Df1_Log
 * @see #LOG_BUFF_LENGTH
 */
void Df1_Log_Format(int level,char *format,...)
{
	char buff[LOG_BUFF_LENGTH];
	va_list ap;

/* format the arguments */
	va_start(ap,format);
	vsprintf(buff,format,ap);
	va_end(ap);
/* call the log routine to log the results */
	Df1_Log(level,buff);
}

/**
 * Routine to log a message to a defined logging mechanism. If the string or General_Data.Log_Handler are NULL
 * the routine does not log the message. If the General_Data.Log_Filter function pointer is non-NULL, the
 * message is passed to it to determoine whether to log the message.
 * @param level An integer, used to decide whether this particular message has been selected for
 * 	logging or not.
 * @param string The message to log.
 * @see #General_Data
 */
void Df1_Log(int level,char *string)
{
/* If the string is NULL, don't log. */
	if(string == NULL)
		return;
/* If there is no log handler, return */
	if(General_Data.Log_Handler == NULL)
		return;
/* If there's a log filter, check it returns TRUE for this message */
	if(General_Data.Log_Filter != NULL)
	{
		if(General_Data.Log_Filter(level,string) == FALSE)
			return;
	}
/* We can log the message */
	(*General_Data.Log_Handler)(level,string);
}

/**
 * Routine to set the General_Data.Log_Handler used by Df1_Log.
 * @param log_fn A function pointer to a suitable handler.
 * @see #General_Data
 * @see #Df1_Log
 */
void Df1_Set_Log_Handler_Function(void (*log_fn)(int level,char *string))
{
	General_Data.Log_Handler = log_fn;
}

/**
 * Routine to set the General_Data.Log_Filter used by Df1_Log.
 * @param log_fn A function pointer to a suitable filter function.
 * @see #General_Data
 * @see #Df1_Log
 */
void Df1_Set_Log_Filter_Function(int (*filter_fn)(int level,char *string))
{
	General_Data.Log_Filter = filter_fn;
}

/**
 * A log handler to be used for the General_Data.Log_Handler function.
 * Just prints the message to stdout, terminated by a newline.
 * @param level The log level for this message.
 * @param string The log message to be logged. 
 */
void Df1_Log_Handler_Stdout(int level,char *string)
{
	if(string == NULL)
		return;
	fprintf(stdout,"%s\n",string);
}

/**
 * Routine to set the General_Data.Log_Filter_Level.
 * @see #General_Data
 */
void Df1_Set_Log_Filter_Level(int level)
{
	General_Data.Log_Filter_Level = level;
}

/**
 * A log message filter routine, to be used for the General_Data.Log_Filter function pointer.
 * @param level The log level of the message to be tested.
 * @param string The log message to be logged, not used in this filter. 
 * @return The routine returns TRUE if the level is less than or equal to the General_Data.Log_Filter_Level,
 * 	otherwise it returns FALSE.
 * @see #General_Data
 */
int Df1_Log_Filter_Level_Absolute(int level,char *string)
{
	return (level <= General_Data.Log_Filter_Level);
}

/**
 * A log message filter routine, to be used for the General_Data.Log_Filter function pointer.
 * @param level The log level of the message to be tested.
 * @param string The log message to be logged, not used in this filter. 
 * @return The routine returns TRUE if the level has bits set that are also set in the 
 * 	General_Data.Log_Filter_Level, otherwise it returns FALSE.
 * @see #General_Data
 */
int Df1_Log_Filter_Level_Bitwise(int level,char *string)
{
	return ((level & General_Data.Log_Filter_Level) > 0);
}

/**
 * Routine to replace instances of find_string with instances of replace_string in string, and return a 
 * string.
 * @param string The initial string. This is <b>not</b> modified.
 * @param find_string The string to look for.
 * @param replace_string The string to replace the found string with. Note the replacement string should
 *        not contain the find_string, otherwise an infinite loop can occur.
 * @return The routine returns the string, with instances of find_string replaced with replace_string.
 *        Note the string is internally static, and is over-written on each call to this routine. If not
 *        immediately used, it should be copied. The routine can return NULL if an error occured.
 * @see #LOG_BUFF_LENGTH
 */
char *Df1_Replace_String(char *string,char *find_string,char *replace_string)
{
	static char return_string[LOG_BUFF_LENGTH];
	char *ptr = NULL;
	int done,start_index,end_index,i,move_count;

	if(string == NULL)
	{
		Df1_Error_Number = 202;
		sprintf(Df1_Error_String,"Df1_Replace_String:string is null.");
		return NULL;
	}
	if(find_string == NULL)
	{
		Df1_Error_Number = 203;
		sprintf(Df1_Error_String,"Df1_Replace_String:find_string is null.");
		return NULL;
	}
	if(replace_string == NULL)
	{
		Df1_Error_Number = 204;
		sprintf(Df1_Error_String,"Df1_Replace_String:replace_string is null.");
		return NULL;
	}
	if(strlen(string) >= LOG_BUFF_LENGTH)
	{
		Df1_Error_Number = 205;
		sprintf(Df1_Error_String,"Df1_Replace_String:string was too long (%d,%d).",
			strlen(string),LOG_BUFF_LENGTH);
		return NULL;
	}
	strcpy(return_string,string);
	done = FALSE;
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
					Df1_Error_Number = 206;
					sprintf(Df1_Error_String,
						"Df1_Replace_String:string is too short ((%d + %d) > %d).",
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
	return return_string;
}

/*
** $Log: not supported by cvs2svn $
*/

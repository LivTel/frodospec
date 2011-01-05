/* newmark_test_error_get.c
** $Header: /home/cjm/cvs/frodospec/newmark_motion_controller/test/newmark_test_error_get.c,v 1.3 2011-01-05 14:17:04 cjm Exp $
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include "log_udp.h"
#include "arcom_ess_general.h"
#include "arcom_ess_interface.h"
#include "newmark_general.h"
#include "newmark_command.h"

/**
 * This program tests the Newmark motion controller "PRINT ERR" and "PRINT ERROR" command.
 * @author $Author: cjm $
 * @version $Revision: 1.3 $
 */
/**
 * Default absolute log level.
 */
#define DEFAULT_LOG_LEVEL       (LOG_VERBOSITY_VERY_VERBOSE)

/* internal variables */
/**
 * Revision control system identifier.
 */
static char rcsid[] = "$Id: newmark_test_error_get.c,v 1.3 2011-01-05 14:17:04 cjm Exp $";
/**
 * Variable holding which type of device we are using to communicate with the PLC.
 * @see ../cdocs/arcom_ess_interface.html#ARCOM_ESS_INTERFACE_DEVICE_ID
 */
static enum ARCOM_ESS_INTERFACE_DEVICE_ID Device_Id = ARCOM_ESS_INTERFACE_DEVICE_NONE;
/**
 * The name of the serial device to open, or the IP Address/hostnmae of the socket device.
 * @see #Device_Id
 */
static char Device_Name[256];
/**
 * The port number of the Arcomm Ethernet Serial Server to open.
 */
static int Port_Number = 0;

/* internal routines */
static int Parse_Arguments(int argc, char *argv[]);
static void Help(void);

/**
 * Main program.
 * @param argc The number of arguments to the program.
 * @param argv An array of argument strings.
 * @return This function returns 0 if the program succeeds, and a positive integer if it fails.
 */
int main(int argc, char *argv[])
{
	Arcom_ESS_Interface_Handle_T *handle = NULL;
	char buff[256];
	int bytes_read,done,error_exists,error_code;

	fprintf(stdout,"Test Newmark Error Get command.\n");
	/* initialise logging */
	Arcom_ESS_Set_Log_Handler_Function(Arcom_ESS_Log_Handler_Stdout);
	Arcom_ESS_Set_Log_Filter_Function(Arcom_ESS_Log_Filter_Level_Absolute);
	Arcom_ESS_Set_Log_Filter_Level(DEFAULT_LOG_LEVEL);
	Newmark_Set_Log_Handler_Function(Newmark_Log_Handler_Stdout);
	Newmark_Set_Log_Filter_Function(Newmark_Log_Filter_Level_Absolute);
	Newmark_Set_Log_Filter_Level(DEFAULT_LOG_LEVEL);
	fprintf(stdout,"Parsing Arguments.\n");
	/* parse arguments */
	if(!Parse_Arguments(argc,argv))
		return 1;
	/* open the interface */
	if(!Arcom_ESS_Interface_Handle_Create(&handle))
	{
		Arcom_ESS_Error();
		return 4;
	}
	if(!Arcom_ESS_Interface_Open("newmark_test_error_get",NULL,Device_Id,Device_Name,Port_Number,handle))
	{
		Arcom_ESS_Error();
		return 3;
	}
	/* Whether there is currently an error in the motion controller */
	if(!Newmark_Command_Err_Get("newmark_test_error_get",NULL,handle,&error_exists))
	{
		Newmark_Error();
		Arcom_ESS_Error();
		return 5;
	}
	/* The current error code in the motion controller */
	if(!Newmark_Command_Error_Get("newmark_test_error_get",NULL,handle,&error_code))
	{
		Newmark_Error();
		Arcom_ESS_Error();
		return 5;
	}
	fprintf(stdout,"Newmark Test Error Get:Existence of an error: %d (1 = exists)\n",error_exists);
	fprintf(stdout,"Newmark Test Error Get:The current error code is : %d\n",error_code);
	/* close interface */
	if(!Arcom_ESS_Interface_Close("newmark_test_error_get",NULL,handle))
	{
		Arcom_ESS_Error();
		return 3;
	}
	if(!Arcom_ESS_Interface_Handle_Destroy(&handle))
	{
		Arcom_ESS_Error();
		return 5;
	}
	fprintf(stdout,"Newmark Test Error Get:Finished Test ...\n");
	return 0;
}

/**
 * Routine to parse command line arguments.
 * @param argc The number of arguments sent to the program.
 * @param argv An array of argument strings.
 * @see #Help
 * @see #Device_Name
 * @see #Port_Number
 * @see #Device_Id
 */
static int Parse_Arguments(int argc, char *argv[])
{
	int i,retval,ivalue;

	for(i=1;i<argc;i++)
	{
		if(strcmp(argv[i],"-baud_rate")==0)
		{
			if((i+1)<argc)
			{
				if(strcmp(argv[i+1],"B9600")==0)
					Arcom_ESS_Serial_Baud_Rate_Set(B9600);
				else if(strcmp(argv[i+1],"B19200")==0)
					Arcom_ESS_Serial_Baud_Rate_Set(B19200);
				else
				{
					fprintf(stderr,"Newmark Test Error Get :Parse_Arguments:"
						"Illegal baud rate : %s.\n",argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Newmark Test Error Get :Parse_Arguments:"
					"Device filename requires a filename.\n");
				return FALSE;
			}
		}
		else if(strcmp(argv[i],"-serial_device")==0)
		{
			if((i+1)<argc)
			{
				strcpy(Device_Name,argv[i+1]);
				Device_Id = ARCOM_ESS_INTERFACE_DEVICE_SERIAL;
				i++;
			}
			else
			{
				fprintf(stderr,"Newmark Test Error Get :Parse_Arguments:"
					"Device filename requires a filename.\n");
				return FALSE;
			}
		}
		else if(strcmp(argv[i],"-socket_device")==0)
		{
			if((i+2)<argc)
			{
				strcpy(Device_Name,argv[i+1]);
				retval = sscanf(argv[i+2],"%d",&Port_Number);
				if(retval != 1)
				{
					fprintf(stderr,"Newmark Test Error Get:Parse_Arguments:"
						"Illegal Socket Port %s.\n",argv[i+2]);
					return FALSE;
				}
				Device_Id = ARCOM_ESS_INTERFACE_DEVICE_SOCKET;
				i+= 2;
			}
			else
			{
				fprintf(stderr,"Newmark Test Error Get:Parse_Arguments:"
					"Socket Device requires an address and port number.\n");
				return FALSE;
			}
		}
		else if(strcmp(argv[i],"-log_level")==0)
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&ivalue);
				if(retval != 1)
				{
					fprintf(stderr,"Newmark Test Error Get:Parse_Arguments:"
						"Illegal log level %s.\n",argv[i+1]);
					return FALSE;
				}
				Arcom_ESS_Set_Log_Filter_Level(ivalue);
				Newmark_Set_Log_Filter_Level(ivalue);
				i++;
			}
			else
			{
				fprintf(stderr,"Newmark Test Error Get:Parse_Arguments:"
					"Log Level requires a number.\n");
				return FALSE;
			}
		}
		else if(strcmp(argv[i],"-help")==0)
		{
			Help();
			exit(0);
		}
		else
		{
			fprintf(stderr,"Newmark Test Error Get:Parse_Arguments:argument '%s' not recognized.\n",argv[i]);
			return FALSE;
		}			
	}
	return TRUE;
}

/**
 * Help routine.
 */
static void Help(void)
{
	fprintf(stdout,"Newmark Test Error Get:Help.\n");
	fprintf(stdout,"Newmark Test Error Get gets the current error code of the Newmark motion controller.\n");
	fprintf(stdout,"newmark_test_error_get [-serial_device <filename>][-socket_device <address> <port>]\n");
	fprintf(stdout,"\t[-log_level <number>][-help][-baud_rate <B9600|B19200>]\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"\t-serial_device specifies the serial device name.\n");
	fprintf(stdout,"\t\tTry /dev/ttyS0 for Linux, try /dev/ttyb for Solaris.\n");
	fprintf(stdout,"\t-socket_device specifies the socket device name.\n");
	fprintf(stdout,"\t-baud_rate changes the serial devices configured baud rate (serial connection only).\n");
	fprintf(stdout,"\t-log_level specifies the logging(0..5).\n");
}

/*
** $Log: not supported by cvs2svn $
** Revision 1.2  2009/02/05 11:42:14  cjm
** Swapped Bitwise for Absolute logging levels.
**
** Revision 1.1  2008/11/20 11:35:54  cjm
** Initial revision
**
*/

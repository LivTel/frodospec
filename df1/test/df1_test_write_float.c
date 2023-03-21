/* df1_test_write_float.c
** $Header: /home/cjm/cvs/frodospec/df1/test/df1_test_write_float.c,v 1.1 2023-03-21 14:36:52 cjm Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "df1_general.h"
#include "df1.h"
#include "df1_read_write.h"
#include "df1_socket.h"
#include "df1_serial.h"

/**
 * This program tests writing a float value to a PLC.
 * @author $Author: cjm $
 * @version $Revision: 1.1 $
 */

/* hash definitions */
/**
 * The default serial device name. /dev/ttyS0 for Linux, try /dev/ttya / /dev/term/a for Solaris.
 */
#define DEFAULT_SERIAL_DEVICE_NAME 	("/dev/ttyS0")
/**
 * Default name of Arcom Ethernet Serial Server.
 */
#define DEFAULT_SOCKET_ADDRESS          ("frodospecserialports")
/**
 * Default port number on the Arcom Ethernet Serial Server to talk to the serial port connected to the PLC.
 */
#define DEFAULT_SOCKET_PORT_NUMBER      (3040)

/**
 * Default bit-wise log level.
 */
#define DEFAULT_LOG_LEVEL       (DF1_LOG_BIT_SERIAL|DF1_LOG_BIT_SOCKET|DF1_LOG_BIT_DF1|DF1_LOG_BIT_DF1_READ_WRITE)

/* internal variables */
/**
 * Revision control system identifier.
 */
static char rcsid[] = "$Id: df1_test_write_float.c,v 1.1 2023-03-21 14:36:52 cjm Exp $";
/**
 * Variable holding which type of device we are using to communicate with the PLC.
 * @see ../cdocs/df1_interface.html#DF1_INTERFACE_DEVICE_ID
 */
static enum DF1_INTERFACE_DEVICE_ID Device_Id = DF1_INTERFACE_DEVICE_NONE;

/**
 * The name of the serial device to open, or the IP Address/hostnmae of the socket device.
 * @see #Device_Id
 */
static char Device_Name[256];
/**
 * The port number of the Arcomm Ethernet Serial Server to open.
 * @see #DEFAULT_SOCKET_PORT_NUMBER
 */
static int Port_Number = DEFAULT_SOCKET_PORT_NUMBER;
/**
 * The address of PLC containing the float value to query.
 */
static char PLC_Address[256];
/**
 * The float value to write to the PLC address. 
 */
static float Value = 0.0f;

/* internal routines */
static int Parse_Arguments(int argc, char *argv[]);
static void Help(void);
/**
 * Main program.
 * @param argc The number of arguments to the program.
 * @param argv An array of argument strings.
 * @return This function returns 0 if the program succeeds, and a positive float if it fails.
 */
int main(int argc, char *argv[])
{
	Df1_Interface_Handle_T *handle = NULL;

	fprintf(stdout,"Test Writing Float values to the PLC.\n");
	/* initialise logging */
	Df1_Set_Log_Handler_Function(Df1_Log_Handler_Stdout);
	Df1_Set_Log_Filter_Function(Df1_Log_Filter_Level_Bitwise);
	Df1_Set_Log_Filter_Level(DEFAULT_LOG_LEVEL);
	fprintf(stdout,"Parsing Arguments.\n");
	strcpy(Device_Name,"");
	strcpy(PLC_Address,"");
	Port_Number = DEFAULT_SOCKET_PORT_NUMBER;
	/* parse arguments */
	if(!Parse_Arguments(argc,argv))
		return 1;
	/* check parameters */
	if(strlen(PLC_Address) == 0)
	{
		fprintf(stderr,"No PLC address specified.\n");
		return 2;
	}
	/* open the interface */
	if(!Df1_Interface_Handle_Create(&handle))
	{
		Df1_Error();
		return 4;
	}
	if(!Df1_Interface_Open(Device_Id,Device_Name,Port_Number,handle))
	{
		Df1_Error();
		return 3;
	}
	/* write float value */
	fprintf(stdout,"Test Write Float:Writing float value %f to PLC Address '%s'.\n",Value,PLC_Address);
	if(!Df1_Write_Float(handle,SLC,PLC_Address,Value))
	{
		Df1_Error();
		return 4;
	}
	fprintf(stdout,"Test Write Float:Written float value %f to PLC Address '%s'.\n",Value,PLC_Address);
	/* close interface */
	if(!Df1_Interface_Close(handle))
	{
		Df1_Error();
		return 3;
	}
	if(!Df1_Interface_Handle_Destroy(&handle))
	{
		Df1_Error();
		return 5;
	}
	fprintf(stdout,"Test Write Float:Finished Test ...\n");
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
 * @see #PLC_Address
 * @see #Value
 */
static int Parse_Arguments(int argc, char *argv[])
{
	int i,retval,ivalue;

	for(i=1;i<argc;i++)
	{
		if(strcmp(argv[i],"-serial_device")==0)
		{
			if((i+1)<argc)
			{
				strcpy(Device_Name,argv[i+1]);
				Device_Id = DF1_INTERFACE_DEVICE_SERIAL;
				i++;
			}
			else
			{
				fprintf(stderr,"Test Write Boolean:Parse_Arguments:"
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
					fprintf(stderr,"Test Write Boolean:Parse_Arguments:"
						"Illegal Socket Port %s.\n",argv[i+2]);
					return FALSE;
				}
				Device_Id = DF1_INTERFACE_DEVICE_SOCKET;
				i+= 2;
			}
			else
			{
				fprintf(stderr,"Test Write Boolean:Parse_Arguments:"
					"Socket Device requires an address and port number.\n");
				return FALSE;
			}
		}
		else if(strcmp(argv[i],"-address")==0)
		{
			if((i+1)<argc)
			{
				strncpy(PLC_Address,argv[i+1],255);
				i++;
			}
			else
			{
				fprintf(stderr,"Test Write Boolean:Parse_Arguments:"
					"Address requires an address.\n");
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
					fprintf(stderr,"Test Write Boolean:Parse_Arguments:"
						"Illegal log level %s.\n",argv[i+1]);
					return FALSE;
				}
				Df1_Set_Log_Filter_Level(ivalue);
				i++;
			}
			else
			{
				fprintf(stderr,"Test Write Boolean:Parse_Arguments:"
					"Log Level requires a number.\n");
				return FALSE;
			}
		}
		else if(strcmp(argv[i],"-value")==0)
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%f",&Value);
				if(retval != 1)
				{
					fprintf(stderr,"Test Write Float:Parse_Arguments:"
						"Illegal value %s.\n",argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Test Write Float:Parse_Arguments:"
					"Value requires a float.\n");
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
			fprintf(stderr,"Test Write Float:Parse_Arguments:argument '%s' not recognized.\n",argv[i]);
			return FALSE;
		}			
	}
	return TRUE;
}

/**
 * Help routine.
 * @see #DEFAULT_SERIAL_DEVICE_NAME
 */
static void Help(void)
{
	fprintf(stdout,"Test Write Float:Help.\n");
	fprintf(stdout,"Test Write Float tries writing an float value to a PLC.\n");
	fprintf(stdout,"df1_test_write_float [-serial_device <filename>][-socket_device <address> <port>]\n");
	fprintf(stdout,"\t[-address <string>][-value <number>]\n");
	fprintf(stdout,"\t[-log_level <number>][-help]\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"\t-serial_device specifies the serial device name.\n");
	fprintf(stdout,"\t\tTry /dev/ttyS0 for Linux, try /dev/ttyb for Solaris.\n");
	fprintf(stdout,"\t-socket_device specifies the socket device name.\n");
	fprintf(stdout,"\t-address specifies the float PLC address to set, of the form N7:1.\n");
	fprintf(stdout,"\t-value specifies the value of the float to write to the PLC address.\n");
	fprintf(stdout,"\t-log_level specifies the logging. See df1_general.h for details.\n");
}

/*
** $Log: not supported by cvs2svn $
*/

/* ccd_interface.c
** low level ccd library
** $Header: /home/cjm/cvs/frodospec/ccd/c/ccd_interface.c,v 0.4 2002-11-07 19:13:39 cjm Exp $
*/
/**
 * ccd_interface.c is a generic interface for communicating with the underlying hardware interface to the
 * SDSU CCD Controller hardware. A device is selected, then the generic routines in this module call the
 * interface specific routines to perform the task.
 * @author SDSU, Chris Mottram
 * @version $Revision: 0.4 $
 */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include "ccd_global.h"
#include "ccd_interface.h"
#include "ccd_text.h"
#include "ccd_pci.h"

/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id: ccd_interface.c,v 0.4 2002-11-07 19:13:39 cjm Exp $";

/* external variables */

/* local variables */
/**
 * Variable holding error code of last operation performed by ccd_interface.
 */
static int Interface_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 */
static char Interface_Error_String[CCD_GLOBAL_ERROR_STRING_LENGTH] = "";
/**
 * The current device the interface is talking to. One of 
 * <a href="#CCD_INTERFACE_DEVICE_ID">CCD_INTERFACE_DEVICE_ID</a>:
 * CCD_INTERFACE_DEVICE_NONE,
 * CCD_INTERFACE_DEVICE_TEXT or
 * CCD_INTERFACE_DEVICE_PCI.
 * @see #CCD_INTERFACE_DEVICE_ID
 */
static enum CCD_INTERFACE_DEVICE_ID Interface_Current_Device = CCD_INTERFACE_DEVICE_NONE;

/* external functions */
/**
 * This routine sets which device the library is going to talk to.
 * @param device_number The device the library will talk to. One of
 * <a href="#CCD_INTERFACE_DEVICE_ID">CCD_INTERFACE_DEVICE_ID</a>:
 * CCD_INTERFACE_DEVICE_NONE,
 * CCD_INTERFACE_DEVICE_TEXT or
 * CCD_INTERFACE_DEVICE_PCI.
 * @return Returns whether the device number could be set as the current device i.e. The device number
 * 	is a legal device number.
 * @see #CCD_INTERFACE_DEVICE_ID
 */
int CCD_Interface_Set_Device(enum CCD_INTERFACE_DEVICE_ID device_number)
{
	Interface_Error_Number = 0;
	/* ensure the device number is legal */
	if(!CCD_INTERFACE_IS_INTERFACE_DEVICE(device_number))
	{
		Interface_Error_Number = 1;
		sprintf(Interface_Error_String,"CCD_Interface_Set_Device:Illegal device '%d'.",
			device_number);
		return FALSE;
	}
	Interface_Current_Device = device_number;
	return TRUE;
}

/**
 * This routine calls the setup routine for the device the library is currently using.
 * @see ccd_text.html#CCD_Text_Initialise
 * @see ccd_pci.html#CCD_PCI_Initialise
 */
void CCD_Interface_Initialise(void)
{
	Interface_Error_Number = 0;
	/* call the device specific setup routine */
	switch(Interface_Current_Device)
	{
		case CCD_INTERFACE_DEVICE_TEXT:
			CCD_Text_Initialise();
			break;
		case CCD_INTERFACE_DEVICE_PCI:
			CCD_PCI_Initialise();
			break;
		default:
			Interface_Error_Number = 2;
			sprintf(Interface_Error_String,"CCD_Interface_Initialise failed:No device selected.");
			CCD_Interface_Error();
			break;
	}
/* print some compile time information to stdout */
	fprintf(stdout,"CCD_Interface_Initialise:%s.\n",rcsid);
}

/**
 * This routine opens the interface for the device the library is currently using.
 * @return The routine returns the return value from the open routine it called. This will normally be TRUE
 * 	if the device was successfully opened, or FALSE if it failed in some way.
 * @see ccd_text.html#CCD_Text_Open
 * @see ccd_pci.html#CCD_PCI_Open
 */
int CCD_Interface_Open(void)
{
	Interface_Error_Number = 0;
	/* call the device specific open routine */
	switch(Interface_Current_Device)
	{
		case CCD_INTERFACE_DEVICE_TEXT:
			return CCD_Text_Open();
		case CCD_INTERFACE_DEVICE_PCI:
			return CCD_PCI_Open();
		default:
			Interface_Error_Number = 3;
			sprintf(Interface_Error_String,"CCD_Interface_Open failed:No device selected.");
			return FALSE;
	}
}

/**
 * This routine sorts out the memory mapping, or emulation, for the specified interface.
 * @param buffer_size The size of the buffer, in bytes.
 * @return The routine returns TRUE if the operation was successfully completed, 
 *         or FALSE if it failed in some way.
 * @see ccd_text.html#CCD_Text_Memory_Map
 * @see ccd_pci.html#CCD_PCI_Memory_Map
 */
int CCD_Interface_Memory_Map(int buffer_size)
{
	Interface_Error_Number = 0;
	/* call the device specific open routine */
	switch(Interface_Current_Device)
	{
		case CCD_INTERFACE_DEVICE_TEXT:
			return CCD_Text_Memory_Map(buffer_size);
		case CCD_INTERFACE_DEVICE_PCI:
			return CCD_PCI_Memory_Map(buffer_size);
		default:
			Interface_Error_Number = 8;
			sprintf(Interface_Error_String,"CCD_Interface_Memory_Map failed:No device selected.");
			return FALSE;
	}
}

/**
 * This routine frees the memory mapping, or emulation, for the specified interface.
 * @return The routine returns TRUE if the operation was successfully completed, 
 *         or FALSE if it failed in some way.
 * @see ccd_text.html#CCD_Text_Memory_UnMap
 * @see ccd_pci.html#CCD_PCI_Memory_UnMap
 */
int CCD_Interface_Memory_UnMap(void)
{
	Interface_Error_Number = 0;
	/* call the device specific open routine */
	switch(Interface_Current_Device)
	{
		case CCD_INTERFACE_DEVICE_TEXT:
			return CCD_Text_Memory_UnMap();
		case CCD_INTERFACE_DEVICE_PCI:
			return CCD_PCI_Memory_UnMap();
		default:
			Interface_Error_Number = 9;
			sprintf(Interface_Error_String,"CCD_Interface_Memory_UnMap failed:No device selected.");
			return FALSE;
	}
}

/**
 * This routine sends a request to the device the library is currently using. It is usually called from
 * <a href="ccd_dsp.html#DSP_Send_Command">DSP_Send_Command</a>.
 * @param request The request number sent to the device.
 * @param argument The address of the data to send as a parameter to the request. Upon a successfull return from
 * 	the routine, the return value from the DSP code may be in the argument.
 * @return The routine returns the return value from the command routine it called. This will normally be TRUE
 * 	if the request was sent correctly, or FALSE if it failed in some way.
 * @see ccd_text.html#CCD_Text_Command
 * @see ccd_pci.html#CCD_PCI_Command
 * @see ccd_dsp.html#DSP_Send_Command
 */
int CCD_Interface_Command(int request,int *argument)
{
	Interface_Error_Number = 0;
	/* call the device specific command routine */
	switch(Interface_Current_Device)
	{
		case CCD_INTERFACE_DEVICE_TEXT:
			return CCD_Text_Command(request,argument);
		case CCD_INTERFACE_DEVICE_PCI:
			return CCD_PCI_Command(request,argument);
		default:
			Interface_Error_Number = 4;
			sprintf(Interface_Error_String,"CCD_Interface_Command failed:No device selected.");
			return FALSE;
	}
}

/**
 * This routine sends a request to the device the library is currently using. It is usually called from
 * <a href="ccd_dsp.html#DSP_Send_Command">DSP_Send_Command</a>.
 * @param request The ioctl request number sent to the device.
 * @param argument_list A list of arguments to send as a parameter to the request. Upon a successfull return from
 * 	the routine, the return value from the DSP code may be in the argument list.
 * @param argument_count The number of arguments in argument_list.
 * @return The routine returns the return value from the command routine it called. This will normally be TRUE
 * 	if the request was sent correctly, or FALSE if it failed in some way.
 * @see ccd_text.html#CCD_Text_Command_List
 * @see ccd_pci.html#CCD_PCI_Command_List
 * @see ccd_dsp.html#DSP_Send_Command
 */
int CCD_Interface_Command_List(int request,int *argument_list,int argument_count)
{
	Interface_Error_Number = 0;
	/* call the device specific command routine */
	switch(Interface_Current_Device)
	{
		case CCD_INTERFACE_DEVICE_TEXT:
			return CCD_Text_Command_List(request,argument_list,argument_count);
		case CCD_INTERFACE_DEVICE_PCI:
			return CCD_PCI_Command_List(request,argument_list,argument_count);
		default:
			Interface_Error_Number = 5;
			sprintf(Interface_Error_String,"CCD_Interface_Command_List failed:No device selected.");
			return FALSE;
	}
}

/**
 * This routine gets reply data from the device the library is currently using. 
 * @param data The address of an unsigned short pointer, which on return from this routine will point to
 *        an area of memory containing the read out CCD image.
 * @return The routine returns TRUE on success, and FALSE if a failure occured.
 * @see ccd_text.html#CCD_Text_Get_Reply_Data
 * @see ccd_pci.html#CCD_PCI_Get_Reply_Data
 */
int CCD_Interface_Get_Reply_Data(unsigned short **data)
{
	Interface_Error_Number = 0;
	/* call the device specific get reply data routine */
	switch(Interface_Current_Device)
	{
		case CCD_INTERFACE_DEVICE_TEXT:
			return CCD_Text_Get_Reply_Data(data);
		case CCD_INTERFACE_DEVICE_PCI:
			return CCD_PCI_Get_Reply_Data(data);
		default:
			Interface_Error_Number = 6;
			sprintf(Interface_Error_String,"CCD_Interface_Get_Reply_Data failed:No device selected.");
			return FALSE;
	}
}

/**
 * This routine closes the interface for the device the library is currently using.
 * @return The routine returns the return value from the close routine it called. This will normally be TRUE
 * 	if the device was successfully closed, or FALSE if it failed in some way.
 * @see ccd_text.html#CCD_Text_Close
 * @see ccd_pci.html#CCD_PCI_Close
 */
int CCD_Interface_Close(void)
{
	Interface_Error_Number = 0;
	/* call the device specific close routine */
	switch(Interface_Current_Device)
	{
		case CCD_INTERFACE_DEVICE_TEXT:
			return CCD_Text_Close();
		case CCD_INTERFACE_DEVICE_PCI:
			return CCD_PCI_Close();
		default:
			Interface_Error_Number = 7;
			sprintf(Interface_Error_String,"CCD_Interface_Close failed:No device selected.");
			return FALSE;
	}
}

/**
 * Routine that returns the current value of ccd_interfaces's error number.
 * @return The current value of ccd_interfaces's error number.
 */
int CCD_Interface_Get_Error_Number(void)
{
	return Interface_Error_Number;
}

/**
 * The error routine that reports any errors occuring in ccd_interface in a standard way.
 * @see ccd_global.html#CCD_Global_Get_Current_Time_String
 */
void CCD_Interface_Error(void)
{
	char time_string[32];

	CCD_Global_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Interface_Error_Number == 0)
		sprintf(Interface_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s CCD_Interface:Error(%d) : %s\n",time_string,Interface_Error_Number,Interface_Error_String);
	Interface_Error_Number = 0;
}

/**
 * The error routine that reports any errors occuring in ccd_interface in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 * @see ccd_global.html#CCD_Global_Get_Current_Time_String
 */
void CCD_Interface_Error_String(char *error_string)
{
	char time_string[32];

	CCD_Global_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Interface_Error_Number == 0)
		sprintf(Interface_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s CCD_Interface:Error(%d) : %s\n",time_string,
		Interface_Error_Number,Interface_Error_String);
	Interface_Error_Number = 0;
}

/*
** $Log: not supported by cvs2svn $
** Revision 0.3  2000/06/13 17:14:13  cjm
** Changes to make Ccs agree with voodoo.
**
** Revision 0.2  2000/04/13 13:15:33  cjm
** Added current time to error routines.
**
** Revision 0.1  2000/01/25 14:57:27  cjm
** initial revision (PCI version).
**
*/

/* ccd_pci.c -*- mode: Fundamental;-*-
** low level ccd library
** $Header: /home/cjm/cvs/frodospec/ccd/c/ccd_pci.c,v 0.1 2000-01-25 14:57:27 cjm Exp $
*/
/**
 * ccd_pci.c will implement a specific interface that connects the SDSU CCD Controller system with a host
 * computer using a PCI interface.
 * @author SDSU, Chris Mottram
 * @version $Revision: 0.1 $
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include "ccd_global.h"
#include "ccd_pci.h"

/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id: ccd_pci.c,v 0.1 2000-01-25 14:57:27 cjm Exp $";

/* #defines */
/**
 * The maximum size for the device name strings.
 */
#define	PCI_MAX_DEV_SIZE	255

/* structures */
/**
 * Internal Data type needed by the device. PCI_Dev0 and PCI_Dev1 hold the names of the two devices
 * the device driver can connect to, and PCI_Fd is the opened file descriptor for the connected device.
 */
typedef struct PCI_Attr_Struct
{
	char PCI_Dev0[PCI_MAX_DEV_SIZE+1];
	char PCI_Dev1[PCI_MAX_DEV_SIZE+1];
	int PCI_Fd;
} PCI_ATTR_T;

/* external variables */

/* local variables */
/**
 * Variable holding error code of last operation performed by ccd_pci.
 */
static int PCI_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 */
static char PCI_Error_String[CCD_GLOBAL_ERROR_STRING_LENGTH] = "";
/**
 * Local variable holding data used to communicate with the interface.
 */
static PCI_ATTR_T PCI_Data;

/* external functions */
/**
 * This routine should be called at startup if this device is to be used to communicate with the SDSU CCD Controler. 
 * It will initialise the connection information ready for the device to be opened.
 * @see ccd_interface.html#CCD_Interface_Initialise
 */
void CCD_PCI_Initialise(void)
{
	PCI_Error_Number = 0;
	strcpy(PCI_Data.PCI_Dev0,CCD_PCI_DEFAULT_DEVICE_ZERO);
	strcpy(PCI_Data.PCI_Dev1,CCD_PCI_DEFAULT_DEVICE_ONE);
	PCI_Data.PCI_Fd = 0;
}

/**
 * This routine will open the device so that commands and replies can be sent and received across the
 * connection. Trys to open the first device only.
 * @return Returns TRUE if a device can be opened, otherwise it returns FALSE.
 * @see ccd_interface.html#CCD_Interface_Open
 */
int CCD_PCI_Open(void)
{
	int error_number;

	PCI_Error_Number = 0;
	/* try to open the first device */
	if((PCI_Data.PCI_Fd = open(PCI_Data.PCI_Dev0,O_RDWR,0))==-1)
	{
		error_number = errno;
		PCI_Error_Number = 1;
		sprintf(PCI_Error_String,"CCD_PCI_Open:failed(%d,%s).",
			error_number,PCI_Data.PCI_Dev0);
		return FALSE;
	}
	return TRUE;
}

/**
 * This routine will send a command to the device driver to be sent to the controller. 
 * @return Returns TRUE if the command was sent to the device driver successfully, FALSE if an error occured.
 * @param request The type of request sent.
 * @param argument The address of an integer that contains the data to be sent as a parameter to this request,
 * 	or to receive the results of this request.
 * @see ccd_interface.html#CCD_Interface_Command
 */
int CCD_PCI_Command(int request,int *argument)
{
	int retval,error_number;

	PCI_Error_Number = 0;
	if(argument == NULL)
	{
		PCI_Error_Number = 2;
		sprintf(PCI_Error_String,"CCD_PCI_Command:data is NULL");
		return FALSE;
	}
	/* send the command 'request' to the PCI interface, using the passed in memory */
	retval = ioctl(PCI_Data.PCI_Fd,request,argument);
	if (retval < 0)
	{
		error_number = errno;
		PCI_Error_Number = 3;
		sprintf(PCI_Error_String,"CCD_PCI_Command:failed(%d,%d,%d,%d).",
			PCI_Data.PCI_Fd,request,(*argument),error_number);
	}
	return (retval == 0);
}

/**
 * This routine will get reply data from the SDSU CCD Controller via the PCI interface. The data is read into
 * the data paramater, up to byte_count bytes of it.
 * @param data An area of memory previously allocated to store the reply data. The area must be able to store
 * 	at least byte_count bytes.
 * @param byte_count The number of bytes to read in from the interface to the data area.
 * @return The routine returns the number of bytes actually read from the interface, or -1 if some error occurs.
 * @see ccd_interface.html#CCD_Interface_Get_Reply_Data
 */
int CCD_PCI_Get_Reply_Data(void *data,int byte_count)
{
	int retval,error_number;

	PCI_Error_Number = 0;
	/* if the data parameter is null we can't save anything in it ! */
	if(data == NULL)
	{
		PCI_Error_Number = 4;
		sprintf(PCI_Error_String,"CCD_PCI_Get_Reply_Data:data is NULL");
		return -1;
	}
	/* why are we calling this function if we don't want any bytes of data? */
	if(byte_count <= 0)
	{
		PCI_Error_Number = 5;
		sprintf(PCI_Error_String,"CCD_PCI_Get_Reply_Data:byte_count is too small:%d.",byte_count);
		return -1;
	}
	/* read returns the number of bytes read or -1 on failure according to the manual pages */
	/* According to SDSU documentation, read returns zero for success!! */
	retval = read(PCI_Data.PCI_Fd,data,byte_count);
	if(retval < 0)
	{
		error_number = errno;
		PCI_Error_Number = 6;
		sprintf(PCI_Error_String,"CCD_PCI_Get_Reply_Data returned %d(%d)",retval,error_number);
	}
	else if(retval < byte_count)
	{
		PCI_Error_Number = 7;
		sprintf(PCI_Error_String,"CCD_PCI_Get_Reply_Data returned %d of %d",retval,byte_count);
		CCD_PCI_Warning();
	}
	return retval;
}

/**
 * This routine will close the PCI interface.
 * @return The routine returns the return value from the close routine it called. This will normally be TRUE
 * 	if the device was successfully closed, or FALSE if it failed in some way.
 * @see ccd_interface.html#CCD_Interface_Close
 */
int CCD_PCI_Close(void)
{
	int error_number;

	PCI_Error_Number = 0;
	/* close the interface - close returns -1 on failure */
	if((close(PCI_Data.PCI_Fd))==-1)
	{
		error_number = errno;
		PCI_Error_Number = 8;
		sprintf(PCI_Error_String,"CCD_PCI_Close:failed %d",error_number);
		return FALSE;
	}
	return TRUE;
}
/**
 * Returns the current value of ccd_pci's error number.
 * @return The current value of ccd_pci's error number.
 */
int CCD_PCI_Get_Error_Number(void)
{
	return PCI_Error_Number;
}

/**
 * The error routine that reports any errors occuring in ccd_pci in a standard way.
 */
void CCD_PCI_Error(void)
{
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(PCI_Error_Number == 0)
		sprintf(PCI_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"CCD_PCI:Error(%d) : %s\n",PCI_Error_Number,PCI_Error_String);
	PCI_Error_Number = 0;
}

/**
 * The error routine that reports any errors occuring in ccd_pci in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 */
void CCD_PCI_Error_String(char *error_string)
{
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(PCI_Error_Number == 0)
		sprintf(PCI_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"CCD_PCI:Error(%d) : %s\n",PCI_Error_Number,PCI_Error_String);
	PCI_Error_Number = 0;
}

/**
 * The warning routine that reports any warnings occuring in ccd_pci in a standard way.
 */
void CCD_PCI_Warning(void)
{
	/* if the error number is zero an warning message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an warning to display */
	if(PCI_Error_Number == 0)
		sprintf(PCI_Error_String,"Logic Error:No Warning defined");
	fprintf(stderr,"CCD_PCI:Warning(%d) : %s\n",PCI_Error_Number,PCI_Error_String);
	PCI_Error_Number = 0;
}

/*
** $Log: not supported by cvs2svn $
*/

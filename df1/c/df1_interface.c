/* df1_interface.c
** FrodoSpec Micrologix 1100 df1 library
** $Header: /home/cjm/cvs/frodospec/df1/c/df1_interface.c,v 1.1 2023-03-21 14:34:10 cjm Exp $
*/
/**
 * DF1 protocol device independant interface routines.
 * Set the interface to be either Serial or Socket and Open/Close/Read/Write calls are directed as appropriate.
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
#ifdef DF1_MUTEXED
#include <pthread.h>
#endif
#include "df1_general.h"
#include "df1_serial.h"
#include "df1_socket.h"

/* data types */
/**
 * Interface Handle structure.
 * <ul>
 * <li><b>Interface_Device</b> Enumeration of type DF1_INTERFACE_DEVICE_ID, used to determine which type of
 *        handle (serial or socket) we are talking to.
 * <li><b>Handle</b> A union of:
 *     <ul>
 *     <li><b>Serial</b> Of type Df1_Serial_Handle_T, holding serial specific data.
 *     <li><b>Socket</b> Of type Df1_Socket_Handle_T, holding socket specific data.
 *     </ul>
 * <li><b>Mutex</b> Optionally compiled mutex locking over sending commands down the comms link 
 *                and receiving a reply.
 * </ul>
 * @see #DF1_INTERFACE_DEVICE_ID
 * @see df1_serial.html#Df1_Serial_Handle_T
 * @see df1_socket.html#Df1_Socket_Handle_T
 */
struct Df1_Interface_Handle_Struct
{
	enum DF1_INTERFACE_DEVICE_ID Interface_Device;
	union
	{
		Df1_Serial_Handle_T Serial;
		Df1_Socket_Handle_T Socket;
	} Handle;
#ifdef DF1_MUTEXED
	pthread_mutex_t Mutex;
#endif
};

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id: df1_interface.c,v 1.1 2023-03-21 14:34:10 cjm Exp $";

/* =======================================
**  external functions 
** ======================================= */
/**
 * Routine to allocate memeory for the interface handle, and initialise the mutex.
 * @param handle The address of a pointer to allocate the handle.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Df1_Interface_Handle_T
 */
int Df1_Interface_Handle_Create(Df1_Interface_Handle_T **handle)
{
	if(handle == NULL)
	{
		Df1_Error_Number = 100;
		sprintf(Df1_Error_String,"Df1_Interface_Handle_Create: handle was NULL.");
		return FALSE;
	}
	/* allocate handle */
	(*handle) = (Df1_Interface_Handle_T *)malloc(sizeof(Df1_Interface_Handle_T));
	if((*handle) == NULL)
	{
		Df1_Error_Number = 103;
		sprintf(Df1_Error_String,"Df1_Interface_Handle_Create: Failed to allocate handle.");
		return FALSE;
	}
	/* initialise mutex - according to man page, pthread_mutex_init always returns 0. */
#ifdef DF1_MUTEXED
	pthread_mutex_init(&((*handle)->Mutex),NULL);
#endif
	return TRUE;
}

/**
 * Routine to open a connection to the specified interface.
 * @param device_id Which sort of device to open, should be one of: DF1_INTERFACE_DEVICE_SERIAL, 
 *        DF1_INTERFACE_DEVICE_SOCKET.
 * @param device_name The name of the device. For serial devices, this is something like "/dev/ttyS0", 
 *        for socket devices this is the IP address or a resolvable name of the Arcom ESS (i.e. 150.204.240.115).
 * @param port_number The port number to communicate over (only valid for DF1_INTERFACE_DEVICE_SOCKET) i.e. 3040.
 * @param handle A Df1_Interface_Handle_T pointer to store the opening information.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #DF1_INTERFACE_DEVICE_NAME_STRING_LENGTH
 * @see #DF1_INTERFACE_DEVICE_ID
 * @see #Df1_Interface_Handle_T
 * @see df1_socket.html#Df1_Socket_Open
 * @see df1_serial.html#Df1_Serial_Open
 */
int Df1_Interface_Open(enum DF1_INTERFACE_DEVICE_ID device_id,char *device_name,int port_number,
			      Df1_Interface_Handle_T *handle)
{
	if(!DF1_INTERFACE_IS_INTERFACE_DEVICE(device_id))
	{
		Df1_Error_Number = 101;
		sprintf(Df1_Error_String,"Df1_Interface_Open: Illegal interface device ID %d.",device_id);
		return FALSE;
	}
	if(device_name == NULL)
	{
		Df1_Error_Number = 102;
		sprintf(Df1_Error_String,"Df1_Interface_Open: device_name was NULL.");
		return FALSE;
	}
	if(handle == NULL)
	{
		Df1_Error_Number = 120;
		sprintf(Df1_Error_String,"Df1_Interface_Open: handle was NULL.");
		return FALSE;
	}
	/* set the device type */
	handle->Interface_Device = device_id;
	/* call the device specific open routine */
	switch(handle->Interface_Device)
	{
		case DF1_INTERFACE_DEVICE_SERIAL:
			if(strlen(device_name) > DF1_INTERFACE_DEVICE_NAME_STRING_LENGTH)
			{
				Df1_Error_Number = 104;
				sprintf(Df1_Error_String,"Df1_Interface_Open: Device name was too long (%d).",
					strlen(device_name));
				return FALSE;
			}
			strcpy(handle->Handle.Serial.Device_Name,device_name);
			return Df1_Serial_Open(&(handle->Handle.Serial));
		case DF1_INTERFACE_DEVICE_SOCKET:
			if(strlen(device_name) > DF1_INTERFACE_DEVICE_NAME_STRING_LENGTH)
			{
				Df1_Error_Number = 105;
				sprintf(Df1_Error_String,"Df1_Interface_Open: Device name was too long (%d).",
					strlen(device_name));
				return FALSE;
			}
			strcpy(handle->Handle.Socket.Address,device_name);
			handle->Handle.Socket.Port_Number = port_number;
			return Df1_Socket_Open(&(handle->Handle.Socket));
		default:
			Df1_Error_Number = 106;
			sprintf(Df1_Error_String,"Df1_Interface_Open failed:Illegal device selected(%d).",
				handle->Interface_Device);
			return FALSE;
	}
}

/**
 * Routine to close a connection to the specified interface.
 * @param handle The connection information spcfying which connection to close.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Df1_Interface_Handle_T
 * @see df1_socket.html#Df1_Socket_Close
 * @see df1_serial.html#Df1_Serial_Close
 */
int Df1_Interface_Close(Df1_Interface_Handle_T *handle)
{
	/* check parameters */
	if(handle == NULL)
	{
		Df1_Error_Number = 107;
		sprintf(Df1_Error_String,"Df1_Interface_Close: handle was NULL.");
		return FALSE;
	}
	/* call the device specific close routine */
	switch(handle->Interface_Device)
	{
		case DF1_INTERFACE_DEVICE_SERIAL:
			if(!Df1_Serial_Close(&(handle->Handle.Serial)))
				return FALSE;
			break;
		case DF1_INTERFACE_DEVICE_SOCKET:
			if(!Df1_Socket_Close(&(handle->Handle.Socket)))
				return FALSE;
			break;
		default:
			Df1_Error_Number = 109;
			sprintf(Df1_Error_String,"Df1_Interface_Close failed:Illegal device selected(%d).",
				handle->Interface_Device);
			return FALSE;
	}
	return TRUE;
}

/**
 * Routine to destroy the specified handle.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Df1_Interface_Handle_T
 */
int Df1_Interface_Handle_Destroy(Df1_Interface_Handle_T **handle)
{
	/* check parameters */
	if(handle == NULL)
	{
		Df1_Error_Number = 121;
		sprintf(Df1_Error_String,"Df1_Interface_Handle_Destroy: handle was NULL.");
		return FALSE;
	}
	if((*handle) == NULL)
	{
		Df1_Error_Number = 108;
		sprintf(Df1_Error_String,"Df1_Interface_Handle_Destroy: handle pointer was NULL.");
		return FALSE;
	}
	/* free alocated handle */
	free((*handle));
	(*handle) = NULL;
	return TRUE;

}

/**
 * Routine to write data to an open connection to the specified interface.
 * @param handle The handle specifying which connection to write to.
 * @param message A buffer containing some data to be written.
 * @param message_length The length of the buffer.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Df1_Interface_Handle_T
 * @see df1_socket.html#Df1_Socket_Write
 * @see df1_serial.html#Df1_Serial_Write
 */
int Df1_Interface_Write(Df1_Interface_Handle_T *handle,void* message,size_t message_length)
{
	if(handle == NULL)
	{
		Df1_Error_Number = 110;
		sprintf(Df1_Error_String,"Df1_Interface_Write: handle was NULL.");
		return FALSE;
	}
	if(message == NULL)
	{
		Df1_Error_Number = 111;
		sprintf(Df1_Error_String,"Df1_Interface_Write: message was NULL.");
		return FALSE;
	}
	/* call the device specific close routine */
	switch(handle->Interface_Device)
	{
		case DF1_INTERFACE_DEVICE_SERIAL:
			if(!Df1_Serial_Write(handle->Handle.Serial,message,message_length))
				return FALSE;
			break;
		case DF1_INTERFACE_DEVICE_SOCKET:
			if(!Df1_Socket_Write(handle->Handle.Socket,message,message_length))
				return FALSE;
			break;
		default:
			Df1_Error_Number = 112;
			sprintf(Df1_Error_String,"Df1_Interface_Write failed:Illegal device selected(%d).",
				handle->Interface_Device);
			return FALSE;
	}
	return TRUE;
}

/**
 * Routine to read data to an open connection to the specified interface.
 * @param handle The handle specifying which connection to write to.
 * @param message A pointer to the buffer to write read data into.
 * @param message_length The length of the buffer.
 * @param bytes_read The address of an integer to fill in how many bytes were read. This can be NULL,
 *        in which case the number of bytes read is not returned.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Df1_Interface_Handle_T
 * @see df1_socket.html#Df1_Socket_Read
 * @see df1_serial.html#Df1_Serial_Read
 */
int Df1_Interface_Read(Df1_Interface_Handle_T *handle,void* message,int message_length,int* bytes_read)
{
	if(handle == NULL)
	{
		Df1_Error_Number = 113;
		sprintf(Df1_Error_String,"Df1_Interface_Read: handle was NULL.");
		return FALSE;
	}
	if(message == NULL)
	{
		Df1_Error_Number = 114;
		sprintf(Df1_Error_String,"Df1_Interface_Read: message was NULL.");
		return FALSE;
	}
	/* call the device specific close routine */
	switch(handle->Interface_Device)
	{
		case DF1_INTERFACE_DEVICE_SERIAL:
			if(!Df1_Serial_Read(handle->Handle.Serial,message,message_length,bytes_read))
				return FALSE;
			break;
		case DF1_INTERFACE_DEVICE_SOCKET:
			if(!Df1_Socket_Read(handle->Handle.Socket,message,message_length,bytes_read))
				return FALSE;
			break;
		default:
			Df1_Error_Number = 115;
			sprintf(Df1_Error_String,"Df1_Interface_Read failed:Illegal device selected(%d).",
				handle->Interface_Device);
			return FALSE;
	}
	return TRUE;
}

#ifdef DF1_MUTEXED
/**
 * Routine to lock the comms access mutex. This will block until the mutex has been acquired,
 * unless an error occurs. The mutex is held within the interface handle, i.e. each connection to a PLC
 * has it's own mutex. This allows us to talk to several different PLCs at the same time, but each PLC
 * can only have one thread talking to it at any one time.
 * @param handle The handle specifying which connection (PLC) to lock access to.
 * @return Returns TRUE if the mutex has been locked for access by this thread,
 * 	FALSE if an error occured.
 * @see #Df1_Interface_Handle_t
 */
int Df1_Interface_Mutex_Lock(Df1_Interface_Handle_T *handle)
{
	int error_number;

	if(handle == NULL)
	{
		Df1_Error_Number = 116;
		sprintf(Df1_Error_String,"Df1_Interface_Mutex_Lock: handle was NULL.");
		return FALSE;
	}
	error_number = pthread_mutex_lock(&(handle->Mutex));
	if(error_number != 0)
	{
		Df1_Error_Number = 117;
		sprintf(Df1_Error_String,"Df1_Interface_Mutex_Lock:Mutex lock failed '%d'.",error_number);
		return FALSE;
	}
	return TRUE;
}

/**
 * Routine to unlock the comms access mutex. 
 * @param handle The handle specifying which connection (PLC) to unlock access to.
 * @return Returns TRUE if the mutex has been unlocked, FALSE if an error occured.
 * @see #Df1_Interface_Handle_t
 */
int Df1_Interface_Mutex_Unlock(Df1_Interface_Handle_T *handle)
{
	int error_number;

	if(handle == NULL)
	{
		Df1_Error_Number = 118;
		sprintf(Df1_Error_String,"Df1_Interface_Mutex_Unlock: handle was NULL.");
		return FALSE;
	}
	error_number = pthread_mutex_unlock(&(handle->Mutex));
	if(error_number != 0)
	{
		Df1_Error_Number = 119;
		sprintf(Df1_Error_String,"Df1_Interface_Mutex_Unlock:Mutex unlock failed '%d'.",error_number);
		return FALSE;
	}
	return TRUE;
}
#endif

/*
** $Log: not supported by cvs2svn $
*/

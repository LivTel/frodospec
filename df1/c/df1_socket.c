/* df1_socket.c
** FrodoSpec Micrologix 1100 DF1 protocol library
** $Header: /home/cjm/cvs/frodospec/df1/c/df1_socket.c,v 1.1 2023-03-21 14:34:10 cjm Exp $
*/
/**
 * Basic operations, open close etc. For driving the Micrologix 1100 using the DF1 protocol 
 * via a Arcom ethernet-RS232 ESS serial socket server.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
/**
 * Define BSD Source to get BSD prototypes, including FNDELAY.
 */
#define _BSD_SOURCE
#include <arpa/inet.h>
#include <errno.h>   /* Error number definitions */
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <termios.h> /* POSIX terminal control definitions */
#include <unistd.h> /* UNIX standard function definitions */
#include "df1_general.h"
#include "df1_socket.h"

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id: df1_socket.c,v 1.1 2023-03-21 14:34:10 cjm Exp $";

/* external functions */

/**
 * Open the socket device, and configure accordingly.
 * @param handle A pointer to an instance of Df1_Socket_Handle_T containing the address and port number to open.
 * @return TRUE if succeeded, FALSE otherwise.
 * @see #Df1_Socket_Handle_T
 */
int Df1_Socket_Open(Df1_Socket_Handle_T *handle)
{
	char host_ip[256];
	struct hostent *host;
	struct sockaddr_in address;
	int open_errno;

	if(handle == NULL)
	{
		Df1_Error_Number = 412;
		sprintf(Df1_Error_String,"Df1_Socket_Open: handle was NULL.");
		return FALSE;
	}
	/* Open socket. */
#if LOGGING > 0
	Df1_Log_Format(DF1_LOG_BIT_SOCKET,"Df1_Socket_Open(%s,%d).",handle->Address,
		       handle->Port_Number);
#endif /* LOGGING */
	handle->Socket_Fd = socket(PF_INET,SOCK_STREAM,0);
	if(handle->Socket_Fd < 0)
	{
		open_errno = errno;
		Df1_Error_Number = 401;
		sprintf(Df1_Error_String,"Df1_Socket_Open: Address %s, port %d failed to open (%d).",
			handle->Address,handle->Port_Number,open_errno);
		return FALSE;
	}
#if LOGGING > 1
	Df1_Log_Format(DF1_LOG_BIT_SOCKET,"Df1_Socket_Open with FD %d.",
		       handle->Socket_Fd);
#endif /* LOGGING */
	/* find host address */
#if LOGGING > 1
	Df1_Log(DF1_LOG_BIT_SOCKET,"Df1_Socket_Open:Find host address.");
#endif /* LOGGING */
	host = gethostbyname(handle->Address);
	if(host == NULL)
	{
		open_errno = errno;
		Df1_Error_Number = 402;
		sprintf(Df1_Error_String,"Df1_Socket_Open: Address %s failed to gethostbyname (%d).",
			handle->Address,open_errno);
		return FALSE;
	}
	strcpy(host_ip,inet_ntoa(*(struct in_addr *)(host->h_addr_list[0])));
	address.sin_family = PF_INET;
	address.sin_addr.s_addr = inet_addr(host_ip);
	address.sin_port = htons(handle->Port_Number);
#if LOGGING > 1
	Df1_Log_Format(DF1_LOG_BIT_SOCKET,"Df1_Socket_Open:trying to connect to %s[%s:%d].",
		       handle->Address,host_ip,handle->Port_Number);
#endif /* LOGGING */
	if(connect(handle->Socket_Fd,(struct sockaddr *)&(address),sizeof(address)) == -1)
	{
		open_errno = errno;
		Df1_Error_Number = 403;
		sprintf(Df1_Error_String,"Df1_Socket_Open: Connect %s:%d failed (%s,%d).",
			host_ip,handle->Port_Number,strerror(open_errno),open_errno);
		return FALSE;
	}
	/* make non-blocking */
	/*
#if LOGGING > 1
	Df1_Log_Format(DF1_LOG_BIT_SOCKET,"Df1_Socket_Open:Set non-blocking.");
#endif 
	*/
/* LOGGING */
/*
	retval = fcntl(handle->Socket_Fd, F_SETFL, FNDELAY);
	if(retval != 0)
	{
		open_errno = errno;
		Df1_Error_Number = 410;
		sprintf(Df1_Error_String,"Df1_Socket_Open: fcntl failed (%d).",open_errno);
		return FALSE;
	}
*/
#if LOGGING > 0
	Df1_Log(DF1_LOG_BIT_SOCKET,"Df1_Socket_Open:Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Routine to close a previously open socket device. 
 * @param handle A pointer to an instance of Df1_Socket_Handle_T containing the socket to close.
 * @return TRUE if succeeded, FALSE otherwise.
 * @see #Df1_Socket_Handle_T
 */
int Df1_Socket_Close(Df1_Socket_Handle_T *handle)
{
	int retval,close_errno;

#if LOGGING > 0
	Df1_Log(DF1_LOG_BIT_SOCKET,"Df1_Socket_Close:Started.");
#endif /* LOGGING */
#if LOGGING > 1
	Df1_Log(DF1_LOG_BIT_SOCKET,"Df1_Socket_Close:Closing file descriptor.");
#endif /* LOGGING */
	retval = close(handle->Socket_Fd);
	if(retval < 0)
	{
		close_errno = errno;
		Df1_Error_Number = 404;
		sprintf(Df1_Error_String,"Df1_Socket_Close: failed (%d,%d,%d).",
			handle->Socket_Fd,retval,close_errno);
		return FALSE;
	}
#if LOGGING > 0
	Df1_Log(DF1_LOG_BIT_SOCKET,"Df1_Socket_Close:Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Routine to write a message to the opened socket link.
 * @param handle An instance of Df1_Socket_Handle_T containing the connection info.
 * @param message A buffer containing the bytes to write.
 * @param message_length The size of the buffer in bytes.
 * @return TRUE if succeeded, FALSE otherwise.
 * @see #Df1_Socket_Handle_T
 */
int Df1_Socket_Write(Df1_Socket_Handle_T handle,void *message,size_t message_length)
{
	int write_errno,retval;

	if(message == NULL)
	{
		Df1_Error_Number = 405;
		sprintf(Df1_Error_String,"Df1_Socket_Write:Message was NULL.");
		return FALSE;
	}
#if LOGGING > 0
	Df1_Log_Format(DF1_LOG_BIT_SOCKET,"Df1_Socket_Write(%d bytes).",message_length);
#endif /* LOGGING */
	retval = write(handle.Socket_Fd,message,message_length);
#if LOGGING > 1
	Df1_Log_Format(DF1_LOG_BIT_SOCKET,"Df1_Socket_Write returned %d.",retval);
#endif /* LOGGING */
	if(retval != message_length)
	{
		write_errno = errno;
		Df1_Error_Number = 406;
		sprintf(Df1_Error_String,"Df1_Socket_Write: failed (%d,%d,%d).",
			handle.Socket_Fd,retval,write_errno);
		return FALSE;
	}
#if LOGGING > 0
	Df1_Log(DF1_LOG_BIT_SOCKET,"Df1_Socket_Write:Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Routine to read a message from the opened socket link. 
 * @param handle An instance of Df1_Socket_Handle_T containing the connection info.
 * @param message A buffer of message_length bytes, to fill with any socket data returned.
 * @param message_length The length of the message buffer.
 * @param bytes_read The address of an integer. On return this will be filled with the number of bytes read from
 *        the socket interface. The address can be NULL, if this data is not needed.
 * @return TRUE if succeeded, FALSE otherwise.
 * @see #Socket_Data
 */
int Df1_Socket_Read(Df1_Socket_Handle_T handle,void *message,int message_length,int *bytes_read)
{
	int read_errno,retval;

	/* check input parameters */
	if(message == NULL)
	{
		Df1_Error_Number = 407;
		sprintf(Df1_Error_String,"Df1_Socket_Read:Message was NULL.");
		return FALSE;
	}
	if(message_length < 0)
	{
		Df1_Error_Number = 408;
		sprintf(Df1_Error_String,"Df1_Socket_Read:Message length was too small:%d.",
			message_length);
		return FALSE;
	}
	/* initialise bytes_read */
	if(bytes_read != NULL)
		(*bytes_read) = 0;
#if LOGGING > 0
	Df1_Log_Format(DF1_LOG_BIT_SOCKET,"Df1_Socket_Read:Max length %d.",message_length);
#endif /* LOGGING */
	retval = read(handle.Socket_Fd,message,message_length);
#if LOGGING > 1
	Df1_Log_Format(DF1_LOG_BIT_SOCKET,"Df1_Socket_Read:returned %d.",retval);
#endif /* LOGGING */
	if(retval < 0)
	{
		read_errno = errno;
		/* if the errno is EAGAIN, a non-blocking read has failed to return any data. */
		if(read_errno != EAGAIN)
		{
			Df1_Error_Number = 409;
			sprintf(Df1_Error_String,"Df1_Socket_Read: failed (%d,%d,%d).",
				handle.Socket_Fd,retval,read_errno);
			return FALSE;
		}
		else
		{
			if(bytes_read != NULL)
				(*bytes_read) = 0;
		}
	}
	else
	{
		if(bytes_read != NULL)
			(*bytes_read) = retval;
	}
#if LOGGING > 0
	Df1_Log_Format(DF1_LOG_BIT_SOCKET,"Df1_Socket_Read:returned %d bytes.",retval);
#endif /* LOGGING */
	return TRUE;
}

/*
** $Log: not supported by cvs2svn $
*/

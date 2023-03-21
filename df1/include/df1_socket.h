/* df1_socket.h
** $Header: /home/cjm/cvs/frodospec/df1/include/df1_socket.h,v 1.1 2023-03-21 14:34:52 cjm Exp $
*/

#ifndef DF1_SOCKET_H
#define DF1_SOCKET_H

/* to get DF1_INTERFACE_DEVICE_NAME_STRING_LENGTH */
#include "df1_interface.h"

/* structures */
/**
 * Structure holding local data pertinent to the socket module. This consists of:
 * <ul>
 * <li><b>Address</b> The address string of the machine/IP number to connect to. 
 *        A fixed length array of length DF1_INTERFACE_DEVICE_NAME_STRING_LENGTH.
 * <li><b>Port_Number</b> The port number of the server socket to connect to.
 * <li><b>Socket_Fd</b> The opened socket port's file descriptor.
 * </ul>
 * @see df1_interface.html#DF1_INTERFACE_DEVICE_NAME_STRING_LENGTH
 */
typedef struct Df1_Socket_Handle_Struct
{
	char Address[DF1_INTERFACE_DEVICE_NAME_STRING_LENGTH];
	int Port_Number;
	int Socket_Fd;
} Df1_Socket_Handle_T;

extern int Df1_Socket_Open(Df1_Socket_Handle_T *handle);
extern int Df1_Socket_Close(Df1_Socket_Handle_T *handle);
extern int Df1_Socket_Write(Df1_Socket_Handle_T handle,void *message,size_t message_length);
extern int Df1_Socket_Read(Df1_Socket_Handle_T handle,void *message,int message_length,int *bytes_read);

#endif

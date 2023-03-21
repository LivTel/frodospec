/* df1_serial.h
** $Header: /home/cjm/cvs/frodospec/df1/include/df1_serial.h,v 1.1 2023-03-21 14:34:52 cjm Exp $
*/

#ifndef DF1_SERIAL_H
#define DF1_SERIAL_H

#include <termios.h>
#include <unistd.h>
/* to get DF1_INTERFACE_DEVICE_NAME_STRING_LENGTH */
#include "df1_interface.h"

/* structures */
/**
 * Structure holding local data pertinent to the serial module. This consists of:
 * <ul>
 * <li><b>Device_Name</b> The device name string of the serial port (e.g. /dev/ttyS0). 
 *     Maximum length DF1_INTERFACE_DEVICE_NAME_STRING_LENGTH.
 * <li><b>Serial_Options_Saved</b> The saved set of serial options.
 * <li><b>Serial_Options</b> The set of serial options configured.
 * <li><b>Serial_Fd</b> The opened serial port's file descriptor.
 * </ul>
 * @see df1_interface.html#DF1_INTERFACE_DEVICE_NAME_STRING_LENGTH
 */
typedef struct Df1_Serial_Handle_Struct
{
	char Device_Name[DF1_INTERFACE_DEVICE_NAME_STRING_LENGTH];
	struct termios Serial_Options_Saved;
	struct termios Serial_Options;
	int Serial_Fd;
} Df1_Serial_Handle_T;

extern int Df1_Serial_Open(Df1_Serial_Handle_T *handle);
extern int Df1_Serial_Close(Df1_Serial_Handle_T *handle);
extern int Df1_Serial_Write(Df1_Serial_Handle_T handle,void *message,size_t message_length);
extern int Df1_Serial_Read(Df1_Serial_Handle_T handle,void *message,int message_length,int *bytes_read);

#endif

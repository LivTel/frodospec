/* df1_interface.h
** FrodoSpec Micrologix 1100 df1 library
** $Header: /home/cjm/cvs/frodospec/df1/include/df1_interface.h,v 1.1 2023-03-21 14:34:52 cjm Exp $
*/
#ifndef DF1_INTERFACE_H
#define DF1_INTERFACE_H

/**
 * Maximum length of the device name.
 */
#define DF1_INTERFACE_DEVICE_NAME_STRING_LENGTH   (256)

/* These enum definitions should match with those in Df1Library.java */
/**
 * Enum for the whether we are talking to the Micrologix over a serial link, or over a socket connection
 * via a Arcom ESS.
 * <ul>
 * <li>DF1_INTERFACE_DEVICE_NONE Interface device number, showing that commands will currently be sent nowhere.
 * <li>DF1_INTERFACE_DEVICE_SERIAL Interface device number, showing that commands will be sent using
 * the serial interface.
 * <li>DF1_INTERFACE_DEVICE_SOCKET Interface device number, showing that commands will currently be sent to the 
 * Micrologix PLC via an Arcom ESS (Ethernet Serial Server).
 * </ul>
 */
enum DF1_INTERFACE_DEVICE_ID
{
	DF1_INTERFACE_DEVICE_NONE,DF1_INTERFACE_DEVICE_SERIAL,DF1_INTERFACE_DEVICE_SOCKET
};

/**
 * Macro to check whether the interface device number is in range.
 */
#define DF1_INTERFACE_IS_INTERFACE_DEVICE(interface_device)	(((interface_device) == DF1_INTERFACE_DEVICE_NONE)|| \
	((interface_device) == DF1_INTERFACE_DEVICE_SERIAL)||((interface_device) == DF1_INTERFACE_DEVICE_SOCKET))

/**
 * Typedef for the interface handle pointer, which is an instance of Df1_Interface_Handle_Struct.
 * @see #Df1_Interface_Handle_Struct
 */
typedef struct Df1_Interface_Handle_Struct Df1_Interface_Handle_T;

extern int Df1_Interface_Handle_Create(Df1_Interface_Handle_T **handle);
extern int Df1_Interface_Open(enum DF1_INTERFACE_DEVICE_ID device_id,char *device_name,int port_number,
			      Df1_Interface_Handle_T *handle);
extern int Df1_Interface_Close(Df1_Interface_Handle_T *handle);
extern int Df1_Interface_Handle_Destroy(Df1_Interface_Handle_T **handle);
extern int Df1_Interface_Write(Df1_Interface_Handle_T *handle,void* message,size_t message_length);
extern int Df1_Interface_Read(Df1_Interface_Handle_T *handle,void* message,int message_length,int* bytes_read);
extern int Df1_Interface_Mutex_Lock(Df1_Interface_Handle_T *handle);
extern int Df1_Interface_Mutex_Unlock(Df1_Interface_Handle_T *handle);

#endif
/*
** $Log: not supported by cvs2svn $
*/

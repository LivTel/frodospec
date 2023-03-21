/* df1_read_write.c
** FrodoSpec Micrologix 1100 df1 library
** $Header: /home/cjm/cvs/frodospec/df1/c/df1_read_write.c,v 1.1 2023-03-21 14:34:10 cjm Exp $
*/
/**
 * DF1 protocol routines for reading and writing values.
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
/**
 * Define BSD Source to get BSD prototypes, including bzero. Consider replacing with memset instead.
 */
#define _BSD_SOURCE

#include <errno.h>   /* Error number definitions */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include "df1_general.h"
#include "df1.h"
#include "df1_interface.h"
#include "df1_read_write.h"

/* data types */
/**
 * Local data.
 * <dl>
 * <dt>Tns</dt> <dd>Transaction Number?</dd>
 * </dl>
 */
struct Df1_Read_Write_Struct
{
	int Tns;
};

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id: df1_read_write.c,v 1.1 2023-03-21 14:34:10 cjm Exp $";
/**
 * Instance of local data. Initialised as follows:
 * <dl>
 * <dt>Tns</dt> <dd>-1</dd>
 * </dl>
 * @see #Df1_Read_Write_Struct
 */
static struct Df1_Read_Write_Struct Df1_Read_Write_Data = {-1};

/* internal functions */
static int Df1_Write_AB(Df1_Interface_Handle_T *handle,TThree_Address_Fields address, word value, word mask);
static int Df1_Read_A2(Df1_Interface_Handle_T *handle,TThree_Address_Fields address, void *value, byte size);
static int Df1_Write_AA(Df1_Interface_Handle_T *handle,TThree_Address_Fields address, void *value, byte size);


/* ----------------------------------------------------------------
** external functions 
** ---------------------------------------------------------------- */
/**
 * Routine to write a boolean value to the PLC.
 * @param handle A pointer to an instance of Df1_Interface_Handle_T containing connection data to the PLC.
 * @param plctype The type of PLC, use SLC if you want it to do anything.
 * @param straddress The PLC address as a string, e.g B3:0/5 , T4:0.dn.
 * @param value The boolean value to write to the PLC's address.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Df1_Write_AB
 * @see df1.html#Df1_Calc_Address
 * @see df1.html#SLC
 * @see df1_general.html#DF1_IS_BOOLEAN
 * @see df1_interface.html#Df1_Interface_Handle_T
 * @see df1.html#TThree_Address_Fields
 */
int Df1_Write_Boolean(Df1_Interface_Handle_T *handle,int plctype, char *straddress, int value)
{
	TThree_Address_Fields address;
	byte posit;
	word valuebool;
	word mask;

	if(straddress == NULL)
	{
		Df1_Error_Number = 504;
		sprintf(Df1_Error_String,"Df1_Write_Boolean: Address was NULL.");
		return FALSE;
	}
#if LOGGING > 1
	Df1_Log_Format(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Write_Boolean(%d,%s,%d) Started.",plctype,straddress,value);
#endif /* LOGGING */
	if(!DF1_IS_BOOLEAN(value))
	{
		Df1_Error_Number = 505;
		sprintf(Df1_Error_String,"Df1_Write_Boolean: Value %d was not a boolean.",value);
		return FALSE;
	}
	/* parse address */
	if(!Df1_Calc_Address(straddress,&address))
		return FALSE;
	posit = address.s_eleNumber;
	if(plctype==SLC)
	{
		mask = 0x0001<<posit;
		if (value)
			valuebool = 0x0001<<posit;
		else
			valuebool = 0x0000;
		if(!Df1_Write_AB(handle,address,valuebool,mask))
			return FALSE;
	}
#if LOGGING > 1
	Df1_Log(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Write_Boolean Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Routine to read a boolean value from the PLC.
 * @param handle A pointer to an instance of Df1_Interface_Handle_T containing connection data to the PLC.
 * @param plctype The type of PLC, use SLC if you want it to do anything.
 * @param straddress The PLC address as a string, e.g B3:0/5 , T4:0.dn.
 * @param value The addres of an integer to hold the boolean value at the PLC's address.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Df1_Read_A2
 * @see df1.html#Df1_Calc_Address
 * @see df1.html#SLC
 * @see df1_interface.html#Df1_Interface_Handle_T
 * @see df1.html#TThree_Address_Fields
 */
int Df1_Read_Boolean(Df1_Interface_Handle_T *handle,int plctype, char *straddress, int *value)
{
	TThree_Address_Fields address;
	byte posit;
	int tempvalue;
	
	if(straddress == NULL)
	{
		Df1_Error_Number = 506;
		sprintf(Df1_Error_String,"Df1_Read_Boolean: Address was NULL.");
		return FALSE;
	}
	if(value == NULL)
	{
		Df1_Error_Number = 507;
		sprintf(Df1_Error_String,"Df1_Read_Boolean: Value was NULL.");
		return FALSE;
	}
#if LOGGING > 1
	Df1_Log_Format(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Read_Boolean(%d,%s) Started.",plctype,straddress);
#endif /* LOGGING */
	tempvalue=0;
	/* parse address */
	if(!Df1_Calc_Address(straddress,&address))
		return FALSE;
	posit=address.s_eleNumber;
	address.s_eleNumber=0;
	if (plctype==SLC)
	{
		if(!Df1_Read_A2(handle,address,&tempvalue,sizeof(tempvalue)))
			return FALSE;
	}
	(*value) = ((tempvalue>>posit)&0x1);
#if LOGGING > 1
	Df1_Log_Format(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Read_Boolean(%d,%s) Finished with value %d.",
		       plctype,straddress,(*value));
#endif /* LOGGING */
	return TRUE;
}

/**
 * Routine to write am integer value to the PLC.
 * @param handle A pointer to an instance of Df1_Interface_Handle_T containing connection data to the PLC.
 * @param plctype The type of PLC, use SLC if you want it to do anything.
 * @param straddress The PLC address as a string, e.g N7:1 , T4:0.PRE.
 * @param value The integer value (as an unsigned short/word) to write to the PLC's address.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Df1_Write_AA
 * @see df1.html#Df1_Calc_Address
 * @see df1.html#SLC
 * @see df1_general.html#DF1_IS_BOOLEAN
 * @see df1_interface.html#Df1_Interface_Handle_T
 * @see df1.html#TThree_Address_Fields
 */
int Df1_Write_Integer(Df1_Interface_Handle_T *handle,int plctype, char *straddress,word value)
{
	TThree_Address_Fields address;

	if(straddress == NULL)
	{
		Df1_Error_Number = 508;
		sprintf(Df1_Error_String,"Df1_Write_Integer: Address was NULL.");
		return FALSE;
	}
#if LOGGING > 1
	Df1_Log_Format(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Write_Integer(%d,%s,%hu) Started.",plctype,straddress,value);
#endif /* LOGGING */
	/* parse address */
	if(!Df1_Calc_Address(straddress,&address))
		return FALSE;
	if(plctype==SLC)
	{
		if(!Df1_Write_AA(handle,address,&value,sizeof(word)))
			return FALSE;
	}
#if LOGGING > 1
	Df1_Log(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Write_Integer Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Routine to read an integer value from the PLC.
 * @param handle A pointer to an instance of Df1_Interface_Handle_T containing connection data to the PLC.
 * @param plctype The type of PLC, use SLC if you want it to do anything.
 * @param straddress The PLC address as a string, e.g N7:1 , T4:0.PRE.
 * @param value The addres of an integer (as an unsigned short/word) to hold the integer's value at the PLC's address.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Df1_Read_A2
 * @see df1.html#Df1_Calc_Address
 * @see df1.html#SLC
 * @see df1_interface.html#Df1_Interface_Handle_T
 * @see df1.html#TThree_Address_Fields
 */
int Df1_Read_Integer(Df1_Interface_Handle_T *handle,int plctype, char *straddress,word *value)
{
	TThree_Address_Fields address;
	
	if(straddress == NULL)
	{
		Df1_Error_Number = 511;
		sprintf(Df1_Error_String,"Df1_Read_Integer: Address was NULL.");
		return FALSE;
	}
	if(value == NULL)
	{
		Df1_Error_Number = 512;
		sprintf(Df1_Error_String,"Df1_Read_Integer: Value was NULL.");
		return FALSE;
	}
#if LOGGING > 1
	Df1_Log_Format(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Read_Integer(%d,%s) Started.",plctype,straddress);
#endif /* LOGGING */
	(*value)=0;
	/* parse address */
	if(!Df1_Calc_Address(straddress,&address))
		return FALSE;
	if (plctype==SLC)
	{
		if(!Df1_Read_A2(handle,address,value,sizeof(word)))
			return FALSE;
	}
#if LOGGING > 1
	Df1_Log_Format(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Read_Integer(%d,%s) Finished with value %hu.",
		       plctype,straddress,(*value));
#endif /* LOGGING */
	return TRUE;
}

/**
 * Routine to write a float value to the PLC.
 * @param handle A pointer to an instance of Df1_Interface_Handle_T containing connection data to the PLC.
 * @param plctype The type of PLC, use SLC if you want it to do anything.
 * @param straddress The PLC address as a string, e.g F8:5 , F8:2.
 * @param value The float value to write to the PLC's address.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Df1_Write_AA
 * @see df1.html#Df1_Calc_Address
 * @see df1.html#SLC
 * @see df1_general.html#DF1_IS_BOOLEAN
 * @see df1_interface.html#Df1_Interface_Handle_T
 * @see df1.html#TThree_Address_Fields
 */
int Df1_Write_Float(Df1_Interface_Handle_T *handle,int plctype, char *straddress,float value)
{
	TThree_Address_Fields address;

	if(straddress == NULL)
	{
		Df1_Error_Number = 513;
		sprintf(Df1_Error_String,"Df1_Write_Float: Address was NULL.");
		return FALSE;
	}
#if LOGGING > 1
	Df1_Log_Format(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Write_Float(%d,%s,%f) Started.",plctype,straddress,value);
#endif /* LOGGING */
	/* parse address */
	if(!Df1_Calc_Address(straddress,&address))
		return FALSE;
	if(plctype==SLC)
	{
		if(!Df1_Write_AA(handle,address,&value,sizeof(float)))
			return FALSE;
	}
#if LOGGING > 1
	Df1_Log(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Write_Float Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Routine to read a float value from the PLC.
 * @param handle A pointer to an instance of Df1_Interface_Handle_T containing connection data to the PLC.
 * @param plctype The type of PLC, use SLC if you want it to do anything.
 * @param straddress The PLC address as a string, e.g F8:5 , F8:2.
 * @param value The addres of a float to hold the float's value at the PLC's address.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Df1_Read_A2
 * @see df1.html#Df1_Calc_Address
 * @see df1.html#SLC
 * @see df1_interface.html#Df1_Interface_Handle_T
 * @see df1.html#TThree_Address_Fields
 */
int Df1_Read_Float(Df1_Interface_Handle_T *handle,int plctype, char *straddress,float *value)
{
	TThree_Address_Fields address;
	
	if(straddress == NULL)
	{
		Df1_Error_Number = 514;
		sprintf(Df1_Error_String,"Df1_Read_Float: Address was NULL.");
		return FALSE;
	}
	if(value == NULL)
	{
		Df1_Error_Number = 515;
		sprintf(Df1_Error_String,"Df1_Read_Float: Value was NULL.");
		return FALSE;
	}
#if LOGGING > 1
	Df1_Log_Format(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Read_Float(%d,%s) Started.",plctype,straddress);
#endif /* LOGGING */
	(*value)=0.0f;
	/* parse address */
	if(!Df1_Calc_Address(straddress,&address))
		return FALSE;
	if (plctype==SLC)
	{
		if(!Df1_Read_A2(handle,address,value,sizeof(float)))
			return FALSE;
	}
#if LOGGING > 1
	Df1_Log_Format(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Read_Float(%d,%s) Finished with value %f.",
		       plctype,straddress,(*value));
#endif /* LOGGING */
	return TRUE;
}

/* ----------------------------------------------------------------
** internal functions 
** ---------------------------------------------------------------- */
/**
 * Cmd:0F Fnc:AB > write W/4 fields & mask in SLC500. Write 1 bit in SLC.
 * If mutex locking is compiled in, the handl's mutex is locked around the Df1_Send/Df1_Receive calls.
 * @param handle A pointer to an instance of Df1_Interface_Handle_T containing connection data to the PLC.
 * @param address Which PLC address to write to.
 * @param value The value to write.
 * @param mask The mask.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Df1_Read_Write_Data
 * @see df1.html#DEST
 * @see df1.html#SOURCE
 * @see df1.html#TCmd4
 * @see df1.html#TThree_Address_Fields
 * @see df1.html#Df1_Send
 * @see df1.html#Df1_Receive
 * @see df1_interface.html#Df1_Interface_Handle_T
 * @see df1_interface.html#Df1_Interface_Mutex_Lock
 * @see df1_interface.html#Df1_Interface_Mutex_Unlock
 */
static int Df1_Write_AB(Df1_Interface_Handle_T *handle,TThree_Address_Fields address, word value, word mask)
{
	TCmd4 cmd;
	TMsg send_msg,rcv_msg;
	
#if LOGGING > 1
	Df1_Log(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Write_AB Started.");
#endif /* LOGGING */
	bzero(&send_msg,sizeof(send_msg));
	bzero(&rcv_msg,sizeof(rcv_msg));
	bzero(&cmd,sizeof(cmd));
	send_msg.dst = DEST;
	send_msg.src = SOURCE;
	send_msg.cmd = 0x0F;
	send_msg.sts = 0x00;
	/* initialise Tns if it has not been used before */
	if(Df1_Read_Write_Data.Tns < 0)
		Df1_Read_Write_Data.Tns = (word)time(NULL);
	send_msg.tns = Df1_Read_Write_Data.Tns++;
	cmd.fnc = 0xAB;
	cmd.size = address.size;
	cmd.fileNumber = address.fileNumber;
	cmd.fileType = address.fileType;
	cmd.eleNumber = address.eleNumber;
	cmd.s_eleNumber = 0x00;
	cmd.maskbyte = mask;
	cmd.value = value;
	memcpy(&send_msg.data,&cmd,sizeof(cmd));
	send_msg.size = sizeof(cmd);
	/* lock mutex */
#ifdef DF1_MUTEXED
#if LOGGING > 5
	Df1_Log(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Write_AB: Locking handle mutex.");
#endif /* LOGGING */
	if(!Df1_Interface_Mutex_Lock(handle))
		return FALSE;
#endif
	/* send message */
#if LOGGING > 5
	Df1_Log(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Write_AB: Sending message.");
#endif /* LOGGING */
	if(!Df1_Send(handle,send_msg))
	{
#ifdef DF1_MUTEXED
		Df1_Interface_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
	/* get reply message */
#if LOGGING > 5
	Df1_Log(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Write_AB: Receiving reply message.");
#endif /* LOGGING */
	if(!Df1_Receive(handle,&rcv_msg))
	{
#ifdef DF1_MUTEXED
		Df1_Interface_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
	/* unlock mutex */
#ifdef DF1_MUTEXED
#if LOGGING > 5
	Df1_Log(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Write_AB: Unlocking handle mutex.");
#endif /* LOGGING */
	if(!Df1_Interface_Mutex_Unlock(handle))
		return FALSE;
#endif
	/* check transaction number */
	if(rcv_msg.tns != send_msg.tns)
	{
		Df1_Error_Number = 500;
		sprintf(Df1_Error_String,"Df1_Write_AB: Send and Receive tns mismatch(%d vs %d).",
			send_msg.tns,rcv_msg.tns);
		return FALSE;
	}
	/* check received message status */
	if(rcv_msg.sts != 0)
	{
		Df1_Error_Number = 501;
		sprintf(Df1_Error_String,"Df1_Write_AB: Receive Mesage sts = %d.",rcv_msg.sts);
		return FALSE;
	}
#if LOGGING > 1
	Df1_Log(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Write_AB Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Cmd:0F Fnc:A2 > Read 3 address fields in SLC500. Read Protected Typed Logical.
 * If mutex locking is compiled in, the handl's mutex is locked around the Df1_Send/Df1_Receive calls.
 * @param handle A pointer to an instance of Df1_Interface_Handle_T containing connection data to the PLC.
 * @param address Which PLC address to read from.
 * @param value The address of a variable to fill in with the value.
 * @param size The size.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Df1_Read_Write_Data
 * @see df1.html#DEST
 * @see df1.html#SOURCE
 * @see df1.html#TCmd
 * @see df1.html#TMsg
 * @see df1.html#Df1_Send
 * @see df1.html#Df1_Receive
 * @see df1_interface.html#Df1_Interface_Handle_T
 * @see df1_interface.html#Df1_Interface_Mutex_Lock
 * @see df1_interface.html#Df1_Interface_Mutex_Unlock
 */
static int Df1_Read_A2(Df1_Interface_Handle_T *handle,TThree_Address_Fields address, void *value, byte size) 
{
	TCmd cmd;
	TMsg send_msg,rcv_msg;

#if LOGGING > 1
	Df1_Log(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Read_A2 Finished.");
#endif /* LOGGING */
	bzero(value,size);
	bzero(&send_msg,sizeof(send_msg));
	bzero(&rcv_msg,sizeof(rcv_msg));
	bzero(&cmd,sizeof(cmd));
	send_msg.dst = DEST;
	send_msg.src = SOURCE;
	send_msg.cmd = 0x0F;
	send_msg.sts = 0x00;
	/* initialise Tns if it has not been used before */
	if(Df1_Read_Write_Data.Tns < 0)
		Df1_Read_Write_Data.Tns = (word)time(NULL);
	send_msg.tns = Df1_Read_Write_Data.Tns++;
	cmd.fnc = 0xA2;
	cmd.size = address.size;
	cmd.fileNumber = address.fileNumber;
	cmd.fileType = address.fileType;
	cmd.eleNumber = address.eleNumber;
	cmd.s_eleNumber = address.s_eleNumber;
	memcpy(&send_msg.data,&cmd,sizeof(cmd));
	send_msg.size = sizeof(cmd);
	/* lock mutex */
#ifdef DF1_MUTEXED
#if LOGGING > 5
	Df1_Log(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Read_A2: Locking handle mutex.");
#endif /* LOGGING */
	if(!Df1_Interface_Mutex_Lock(handle))
		return FALSE;
#endif
	/* send message */
#if LOGGING > 5
	Df1_Log(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Read_A2: Sending message.");
#endif /* LOGGING */
	if(!Df1_Send(handle,send_msg))
	{
#ifdef DF1_MUTEXED
		Df1_Interface_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
	/* get reply message */
#if LOGGING > 5
	Df1_Log(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Read_A2: Receiving reply message.");
#endif /* LOGGING */
	if(!Df1_Receive(handle,&rcv_msg))
	{
#ifdef DF1_MUTEXED
		Df1_Interface_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
	/* unlock mutex */
#ifdef DF1_MUTEXED
#if LOGGING > 5
	Df1_Log(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Read_A2: Unlocking handle mutex.");
#endif /* LOGGING */
	if(!Df1_Interface_Mutex_Unlock(handle))
		return FALSE;
#endif
	/* check transaction number */
	if (rcv_msg.tns!=send_msg.tns)
	{
		Df1_Error_Number = 502;
		sprintf(Df1_Error_String,"Df1_Read_A2: Send and Receive tns mismatch(%d vs %d).",
			send_msg.tns,rcv_msg.tns);
		return FALSE;
	}
	/* check received message status */
	if(rcv_msg.sts!=0)
	{
		Df1_Error_Number = 503;
		sprintf(Df1_Error_String,"Df1_Read_A2: Receive Mesage sts = %d.",rcv_msg.sts);
		return FALSE;
	}
	memcpy(value,rcv_msg.data,address.size);
#if LOGGING > 1
	Df1_Log(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Read_A2 Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Cmd:0F Fnc:AA > write 3 address fields in SLC500.
 * Write Protected Typed Logical.
 * @param handle A pointer to an instance of Df1_Interface_Handle_T containing connection data to the PLC.
 * @param address Which PLC address to write to.
 * @param value The address of a variable to fill in with the value.
 * @param size The size of value.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Df1_Read_Write_Data
 * @see df1.html#DEST
 * @see df1.html#SOURCE
 * @see df1.html#TCmd
 * @see df1.html#TMsg
 * @see df1.html#Df1_Send
 * @see df1.html#Df1_Receive
 * @see df1_interface.html#Df1_Interface_Handle_T
 * @see df1_interface.html#Df1_Interface_Mutex_Lock
 * @see df1_interface.html#Df1_Interface_Mutex_Unlock
 */
static int Df1_Write_AA(Df1_Interface_Handle_T *handle,TThree_Address_Fields address, void *value, byte size) 
{
	TCmd cmd;
	TMsg send_msg,rcv_msg;

#if LOGGING > 1
	Df1_Log(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Write_AA Started.");
#endif /* LOGGING */
	bzero(&send_msg,sizeof(send_msg));
	bzero(&rcv_msg,sizeof(rcv_msg));
	bzero(&cmd,sizeof(cmd));
	send_msg.dst = DEST;
	send_msg.src = SOURCE;
	send_msg.cmd = 0x0F;
	send_msg.sts = 0x00;
	/* initialise Tns if it has not been used before */
	if(Df1_Read_Write_Data.Tns < 0)
		Df1_Read_Write_Data.Tns = (word)time(NULL);
	send_msg.tns = Df1_Read_Write_Data.Tns++;
	cmd.fnc = 0xAA;
	cmd.size = address.size;
	cmd.fileNumber = address.fileNumber;
	cmd.fileType = address.fileType;
	cmd.eleNumber = address.eleNumber;
	cmd.s_eleNumber = address.s_eleNumber;
	memcpy(&send_msg.data,&cmd,sizeof(cmd));
	send_msg.size = sizeof(cmd);
	/* diddly cjm - this looks dodgy
	memcpy(&send_msg.data[send_msg.size],value,cmd.size);
	send_msg.size += cmd.size; 
	*/
	memcpy(&send_msg.data[send_msg.size],value,size);
	send_msg.size += size; 
	/* lock mutex */
#ifdef DF1_MUTEXED
#if LOGGING > 5
	Df1_Log(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Write_AA: Locking handle mutex.");
#endif /* LOGGING */
	if(!Df1_Interface_Mutex_Lock(handle))
		return FALSE;
#endif
	/* send message */
#if LOGGING > 5
	Df1_Log(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Write_AA: Sending message.");
#endif /* LOGGING */
	if(!Df1_Send(handle,send_msg))
	{
#ifdef DF1_MUTEXED
		Df1_Interface_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
	/* get reply message */
#if LOGGING > 5
	Df1_Log(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Write_AA: Receiving reply message.");
#endif /* LOGGING */
	if(!Df1_Receive(handle,&rcv_msg))
	{
#ifdef DF1_MUTEXED
		Df1_Interface_Mutex_Unlock(handle);
#endif
		return FALSE;
	}
	/* unlock mutex */
#ifdef DF1_MUTEXED
#if LOGGING > 5
	Df1_Log(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Write_AA: Unlocking handle mutex.");
#endif /* LOGGING */
	if(!Df1_Interface_Mutex_Unlock(handle))
		return FALSE;
#endif
	/* check transaction number */
	if (rcv_msg.tns!=send_msg.tns)
	{
		Df1_Error_Number = 509;
		sprintf(Df1_Error_String,"Df1_Write_AA: Send and Receive tns mismatch(%d vs %d).",
			send_msg.tns,rcv_msg.tns);
		return FALSE;
	}
	/* check received message status */
	if(rcv_msg.sts!=0)
	{
		Df1_Error_Number = 510;
		sprintf(Df1_Error_String,"Df1_Write_AA: Receive Mesage sts = %d.",rcv_msg.sts);
		return FALSE;
	}
#if LOGGING > 1
	Df1_Log(DF1_LOG_BIT_DF1_READ_WRITE,"Df1_Write_AA Finished.");
#endif /* LOGGING */
	return TRUE;
}

/*
** $Log: not supported by cvs2svn $
*/

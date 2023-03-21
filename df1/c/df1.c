/* df1.c
** FrodoSpec Micrologix 1100 df1 library
** $Header: /home/cjm/cvs/frodospec/df1/c/df1.c,v 1.1 2023-03-21 14:34:10 cjm Exp $
*/
/**
 * DF1 protocol handling routines.
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

#include <ctype.h>
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

/* defines */
/**
 * One millosecond in nanoseconds (1000000).
 */
#define ONE_MILLISECOND_NS   (1000000)

/* data types */
/**
 * Local data.
 * <dl>
 * <dt>Reply_Pause_Ms</dt> <dd>Number of milliseconds to pause whilst awaiting ACKs.</dd>
 * </dl>
 */
struct Df1_Struct
{
	int Reply_Pause_Ms;
};

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id: df1.c,v 1.1 2023-03-21 14:34:10 cjm Exp $";
/**
 * Instance of local data. Initialised as follows:
 * <dl>
 * <dt>Reply_Pause_Ms</dt> <dd>100.</dd>
 * </dl>
 * @see #Df1_Struct
 */
static struct Df1_Struct Df1_Data = {100};

/* internal function prototypes */
static int Df1_Send_Response(Df1_Interface_Handle_T *handle,byte response);
static int Df1_Get_Symbol(Df1_Interface_Handle_T *handle,byte * b,int *flag);
static word Df1_Bytes2Word(byte lowb, byte highb);
static int Df1_Add_Word2Buffer(TBuffer * buffer, word value);
static int Df1_Add_Byte2Buffer(TBuffer * buffer, byte value);
static int Df1_Add_Data2Buffer(TBuffer * buffer, void * data, byte size);
static int Df1_Add_Data2BufferWithDLE(TBuffer * buffer, TMsg msg);
static char * Df1_Print_Symbol(byte c);
static word Df1_Compute_Crc(TBuffer * buffer);
static word Df1_Calc_Crc(word crc,word buffer);

/* ----------------------------------------------------------------
** external functions 
** ---------------------------------------------------------------- */
/**
 * Routine to send a DF1 protocol command.
 * @param handle A pointer to an instance of Df1_Interface_Handle_T containing connection data to the PLC.
 * @param df1_data An instance of TMsg containing the data to send to the PLC.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Df1_Send_Response
 * @see #Df1_Get_Symbol
 * @see #TBuffer
 * @see #TMsg
 * @see #Df1_Add_Byte2Buffer
 * @see #Df1_Add_Data2Buffer
 * @see #Df1_Add_Word2Buffer
 * @see #Df1_Add_Data2BufferWithDLE
 * @see #Df1_Compute_Crc
 * @see #Df1_Print_Symbol
 * @see df1_interface.html#Df1_Interface_Handle_T
 * @see df1_interface.html#Df1_Interface_Write
 */
int Df1_Send(Df1_Interface_Handle_T *handle,TMsg df1_data)
{
	TBuffer crc_buffer,data_send,data_rcv;
	struct timespec sleep_time;
	int nbr_NAK=0;
	int nbr_ENQ=0;
	int retval,sleep_errno,flag;
	byte c;

#if LOGGING > 5
	Df1_Log(DF1_LOG_BIT_DF1,"Df1_Send Started.");
#endif /* LOGGING */
	/* initialise buffers */
	bzero(&crc_buffer,sizeof(crc_buffer));
	bzero(&data_send,sizeof(data_send));
	bzero(&data_rcv,sizeof(data_rcv));
	/* create message to send */
	Df1_Add_Byte2Buffer(&data_send,DLE);	
	Df1_Add_Byte2Buffer(&data_send,STX);
	Df1_Add_Data2BufferWithDLE(&data_send,df1_data);
	Df1_Add_Byte2Buffer(&data_send,DLE);	
	Df1_Add_Byte2Buffer(&data_send,ETX);	
	Df1_Add_Data2Buffer(&crc_buffer,&df1_data,df1_data.size+6);
	Df1_Add_Word2Buffer(&data_send, Df1_Compute_Crc(&crc_buffer));
	/* enter loop to send buffer */
	do
	{
#if LOGGING > 5
		Df1_Log_Format(DF1_LOG_BIT_DF1,"Df1_Send writing %d bytes.",data_send.size);
#endif /* LOGGING */
		if(!Df1_Interface_Write(handle,&data_send, data_send.size))
			return FALSE;
		/* read ACK or NAK */
		do
		{
			/* sleep for Reply_Pause_Ms */
			sleep_time.tv_sec = (Df1_Data.Reply_Pause_Ms/1000);
			sleep_time.tv_nsec = (Df1_Data.Reply_Pause_Ms%1000)*ONE_MILLISECOND_NS;
			retval = nanosleep(&sleep_time,NULL);
			if(retval != 0)
			{
				sleep_errno = errno;
				Df1_Error_Number = 300;
				sprintf(Df1_Error_String,"Df1_Send:nanosleep failed(%d).",retval);
				Df1_Error();
				/* not a fatal error, don't return */
			}
			/* increment counter */
			nbr_ENQ++;
			if(nbr_ENQ > 3)
			{
				Df1_Error_Number = 301;
				sprintf(Df1_Error_String,"Df1_Send:ENQ Timeout.");
				return FALSE;
			}
			/* enquire response */
#if LOGGING > 5
			Df1_Log(DF1_LOG_BIT_DF1,"Df1_Send : Df1_Send_Response(ENQ).");
#endif /* LOGGING */
			if(!Df1_Send_Response(handle,ENQ))
				Df1_Error();
#if LOGGING > 5
			Df1_Log_Format(DF1_LOG_BIT_DF1,"Df1_Send : Calling Df1_Get_Symbol.",ENQ);
#endif /* LOGGING */
			if(!Df1_Get_Symbol(handle,&c,&flag))
				Df1_Error();
#if LOGGING > 5
			Df1_Log_Format(DF1_LOG_BIT_DF1,"Df1_Send : Df1_Get_Symbol returned byte=%s,flag=%d.",
				       Df1_Print_Symbol(c),flag);
#endif /* LOGGING */
		} 
		while ((flag!=CONTROL_FLAG) & ((c!=ACK) | (c!=NAK)));
		if (c==ACK)
		{
#if LOGGING > 5
			Df1_Log(DF1_LOG_BIT_DF1,"Df1_Send Finished.");
#endif /* LOGGING */
			 return TRUE;
		}
		if (c==NAK) 
		{
			nbr_NAK++;
		}
	} while (nbr_NAK<=3);
#if LOGGING > 5
	Df1_Log(DF1_LOG_BIT_DF1,"Df1_Send Finished with error.");
#endif /* LOGGING */
	Df1_Error_Number = 302;
	sprintf(Df1_Error_String,"Df1_Send:Too many NAKs (%d).",nbr_NAK);
	return FALSE;
}

/**
 * Receive a DF1 message.
 * @param handle A pointer to an instance of Df1_Interface_Handle_T containing connection data to the PLC.
 * @param df1_data The address of a TMsg structure to fill with the received data.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #TMsg
 * @see #TBuffer
 * @see #Df1_Get_Symbol
 * @see #Df1_Send_Response
 * @see #Df1_Add_Byte2Buffer
 * @see #Df1_Bytes2Word
 * @see #Df1_Compute_Crc
 * @see #Df1_Print_Symbol
 * @see df1_interface.html#Df1_Interface_Handle_T
 * @see df1_interface.html#Df1_Interface_Read
 */
int Df1_Receive(Df1_Interface_Handle_T *handle,TMsg *df1_data) 
{
	byte c,crcb1,crcb2;
	word crc;
	TBuffer data_rcv;
	int flag,done,crc_ok,bytes_read;
	
#if LOGGING > 5
	Df1_Log(DF1_LOG_BIT_DF1,"Df1_Receive Started.");
#endif /* LOGGING */
	crc_ok=FALSE;
	do
	{
		bzero(&data_rcv,sizeof(data_rcv));
		do
		{
#if LOGGING > 5
			Df1_Log(DF1_LOG_BIT_DF1,"Df1_Receive: Looking for control flag symbol.");
#endif /* LOGGING */
			if(!Df1_Get_Symbol(handle,&c,&flag))
				return FALSE;
		}
		while (flag!=CONTROL_FLAG);
#if LOGGING > 5
		Df1_Log_Format(DF1_LOG_BIT_DF1,"Df1_Receive: Found control flag %d, symbol %s.",flag,
			       Df1_Print_Symbol(c));
#endif /* LOGGING */
		switch (c)
		{	
			case ENQ: 
#if LOGGING > 5
				Df1_Log_Format(DF1_LOG_BIT_DF1,
					       "Df1_Receive: Found control flag ENQ: Sending response NAK.");
#endif /* LOGGING */
				/* send NAK - was last_response, but that was always set to NAK */
				if(!Df1_Send_Response(handle,NAK))
					return FALSE;
				break;
			case STX:
#if LOGGING > 5
				Df1_Log(DF1_LOG_BIT_DF1,"Df1_Receive: Found control flag STX: Reading data.");
#endif /* LOGGING */
				done = FALSE;
				while(done == FALSE)
				{
					if(!Df1_Get_Symbol(handle,&c,&flag))
						return FALSE;
					if(flag !=CONTROL_FLAG)
					{
						if(!Df1_Add_Byte2Buffer(&data_rcv,c))
							return FALSE;
					}
					else
					{
						done = TRUE;
					}
				}
				if (c==ETX)
				{
#if LOGGING > 5
					Df1_Log(DF1_LOG_BIT_DF1,"Df1_Receive: Data terminated with ETX: Getting CRC.");
#endif /* LOGGING */
					/* read crcb1 */
					if(!Df1_Interface_Read(handle,&crcb1,1,&bytes_read))
						return FALSE;
					if(bytes_read < 1)
					{
						Df1_Error_Number = 307;
						sprintf(Df1_Error_String,
							"Df1_Receive:crcb1 bytes_read was too small(%d).",bytes_read);
						return FALSE;
					}
					/* read crcb2 */
					if(!Df1_Interface_Read(handle,&crcb2,1,&bytes_read))
						return FALSE;
					if(bytes_read < 1)
					{
						Df1_Error_Number = 308;
						sprintf(Df1_Error_String,
							"Df1_Receive:crcb2 bytes_read was too small(%d).",bytes_read);
						return FALSE;
					}
					/* create crc and check it */
					crc = Df1_Bytes2Word(crcb1,crcb2);
					if (crc==Df1_Compute_Crc(&data_rcv)) 
					{
#if LOGGING > 5
						Df1_Log(DF1_LOG_BIT_DF1,
							"Df1_Receive: CRC was good - sending ACK response.");
#endif /* LOGGING */
						if(!Df1_Send_Response(handle,ACK))
							return FALSE;
						crc_ok=TRUE;
					}	
					else
					{
#if LOGGING > 5
						Df1_Log(DF1_LOG_BIT_DF1,
							"Df1_Receive: CRC was bad - sending NAK response.");
#endif /* LOGGING */
						if(!Df1_Send_Response(handle,NAK))
							return FALSE;
						crc_ok=FALSE;	
					}
				}
				else
				{
#if LOGGING > 5
					Df1_Log_Format(DF1_LOG_BIT_DF1,
					    "Df1_Receive: Data was not terminated with ETX(%s): Sending NAK response.",
						       Df1_Print_Symbol(c));
#endif /* LOGGING */
					if(!Df1_Send_Response(handle,NAK))
						return FALSE;
				} 
				break;
			default:
#if LOGGING > 5
				Df1_Log_Format(DF1_LOG_BIT_DF1,
					"Df1_Receive: Control flag was not STX or ENQ(%s): Sending NAK response.",
					       Df1_Print_Symbol(c));
#endif /* LOGGING */
				if(!Df1_Send_Response(handle,NAK))
					return FALSE;
		}
	} 
	while (crc_ok != TRUE);
#if LOGGING > 5
	Df1_Log_Format(DF1_LOG_BIT_DF1,"Df1_Receive: Received %d bytes of data : Copying to df1_data.",data_rcv.size);
#endif /* LOGGING */
	/* diddly dodgy, check size vs TMsg data size (255 + a bit) */
	memcpy(df1_data,data_rcv.data,data_rcv.size);
#if LOGGING > 5
	Df1_Log(DF1_LOG_BIT_DF1,"Df1_Receive: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Routine to take a string representation of a PLC address and turn it into an instance of TThree_Address_Fields.
 * @param straddress The string version of the address.
 * @param address The address (memory location) of an instance of TThree_Address_Fields to fill in with
 *                 the parsed PLC address.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #TThree_Address_Fields
 */
int Df1_Calc_Address(char *straddress,TThree_Address_Fields *address)
{
	int x,l;

	if(straddress == NULL)
	{
		Df1_Error_Number = 309;
		sprintf(Df1_Error_String,"Df1_Calc_Address:String address was NULL.");
		return FALSE;
	}
#if LOGGING > 5
	Df1_Log_Format(DF1_LOG_BIT_DF1,"Df1_Calc_Address(address=%s): Starteded.",straddress);
#endif /* LOGGING */
	if(address == NULL)
	{
		Df1_Error_Number = 310;
		sprintf(Df1_Error_String,"Df1_Calc_Address:Address was NULL.");
		return FALSE;
	}
	bzero(address,sizeof(*address));
	address->size=0;
	address->fileNumber=0;
	address->fileType=0;
	address->eleNumber=0;
	address->s_eleNumber=0;
	for (x=0;x<strlen(straddress);x++)
	{
		switch (straddress[x])
		{
			case 'O':
				address->fileType = 0x8b;
				address->fileNumber = 0;
				address->size = 2;
				break;
			case 'I':
				address->fileType = 0x8c;
				address->fileNumber = 1;
				address->size = 2;
				break;
			case 'S':
				x++;
				address->fileType = 0x84;
				address->fileNumber = atoi(&straddress[x]);
				address->size = 2;
				break;
			case 'B':
				x++;
				address->fileType = 0x85;
				address->fileNumber = atoi(&straddress[x]);
				address->size = 2;
				break;
			case 'T':
				x++;
				address->fileType = 0x86;
				address->fileNumber = atoi(&straddress[x]);
				address->size = 2;
				break;
			case 'C':
				x++;
				address->fileType = 0x87;
				address->fileNumber = atoi(&straddress[x]);
				address->size = 2;
				break;
			case 'R':
				x++;
				address->fileType = 0x88;
				address->fileNumber = atoi(&straddress[x]);
				address->size = 2;
				break;
			case 'N':
				x++;
				address->fileType = 0x89;
				address->fileNumber = atoi(&straddress[x]);
				address->size=2;
				break;
			case 'F':
				x++;
				address->fileType = 0x8a;
				address->fileNumber = atoi(&straddress[x]);
				address->size = 4;
				break;
			case ':':
				address->eleNumber = atoi(&straddress[++x]);
				break;
			case '.':
			case '/':
				x++;
				if (isdigit(straddress[x]))
				{
					address->s_eleNumber = atoi(&straddress[x]);
				}
				l=strlen(straddress) - x;
				if (strncasecmp (&straddress[x],"acc",l) == 0 )
					address->s_eleNumber = 2;
				if (strncasecmp (&straddress[x],"pre",l) == 0 )
					address->s_eleNumber = 1;
				if (strncasecmp (&straddress[x],"len",l) == 0 )
					address->s_eleNumber = 1;
				if (strncasecmp (&straddress[x],"pos",l) == 0 )
					address->s_eleNumber = 2;
				if (strncasecmp (&straddress[x],"en",l) == 0 )
					address->s_eleNumber = 13;
				if (strncasecmp (&straddress[x],"tt",l) == 0 )
					address->s_eleNumber = 14;
				if (strncasecmp (&straddress[x],"dn",l) == 0 )
					address->s_eleNumber = 15;				
				x = strlen(straddress)-1;
		}/* end switch */
	}/* end for */
#if LOGGING > 5
	Df1_Log_Format(DF1_LOG_BIT_DF1,"Df1_Calc_Address: %s = size = %d,file_number = %d,file_type = %d, "
		       "ele_number = %d,s_ele_number = %d.",straddress,address->size,
		       address->fileNumber,address->fileType,address->eleNumber,address->s_eleNumber);
#endif /* LOGGING */
#if LOGGING > 5
	Df1_Log(DF1_LOG_BIT_DF1,"Df1_Calc_Address: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/* ----------------------------------------------------------------
** internal functions 
** ---------------------------------------------------------------- */
/**
 * Send a reponse request (NAK or ACK).
 * @param handle A pointer to an instance of Df1_Interface_Handle_T containing connection data to the PLC.
 * @param response What to send.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #DLE
 * @see #Df1_Bytes2Word
 * @see #Df1_Print_Symbol
 * @see df1_interface.html#Df1_Interface_Handle_T
 * @see df1_interface.html#Df1_Interface_Write
 */
static int Df1_Send_Response(Df1_Interface_Handle_T *handle,byte response)
{
	word w;
	byte dle=DLE;

#if LOGGING > 5
	Df1_Log_Format(DF1_LOG_BIT_DF1,"Df1_Send_Response(%s).",Df1_Print_Symbol(response));
#endif /* LOGGING */
	w = Df1_Bytes2Word(dle,response);
	if(!Df1_Interface_Write(handle,&w,2))
		return FALSE;
#if LOGGING > 5
	Df1_Log_Format(DF1_LOG_BIT_DF1,"Df1_Send_Response finished.");
#endif /* LOGGING */
	return TRUE;
}	

/**
 * Get a symbol from a read byte.
 * @param handle A pointer to an instance of Df1_Interface_Handle_T containing connection data to the PLC.
 * @param b The address of a byte to return.
 * @param flag What sort of byte was read: DATA_FLAG, CONTROL_FLAG
 * @see #DATA_FLAG
 * @see #CONTROL_FLAG
 * @see df1_interface.html#Df1_Interface_Handle_T
 * @see df1_interface.html#Df1_Interface_Read
 */
static int Df1_Get_Symbol(Df1_Interface_Handle_T *handle,byte * b,int *flag)
{
	byte c1,c2;
	int bytes_read;

#if LOGGING > 5
	Df1_Log_Format(DF1_LOG_BIT_DF1,"Df1_Get_Symbol started.");
#endif /* LOGGING */
	if(b == NULL)
	{
		Df1_Error_Number = 303;
		sprintf(Df1_Error_String,"Df1_Get_Symbol:b was NULL.");
		return FALSE;
	}
	if(flag == NULL)
	{
		Df1_Error_Number = 304;
		sprintf(Df1_Error_String,"Df1_Get_Symbol:flag was NULL.");
		return FALSE;
	}
	if(!Df1_Interface_Read(handle,&c1,1,&bytes_read))
		return FALSE;
	if(bytes_read < 1)
	{
		Df1_Error_Number = 305;
		sprintf(Df1_Error_String,"Df1_Get_Symbol:bytes_read was too small(%d).",bytes_read);
		return FALSE;
	}
#if LOGGING > 5
	Df1_Log_Format(DF1_LOG_BIT_DF1,"Df1_Get_Symbol: First byte %s.",Df1_Print_Symbol(c1));
#endif /* LOGGING */
	if (c1==DLE)
	{
#if LOGGING > 5
		Df1_Log_Format(DF1_LOG_BIT_DF1,"Df1_Get_Symbol: c1 was DLE: Attempting retry.");
#endif /* LOGGING */
		if(!Df1_Interface_Read(handle,&c2,1,&bytes_read))
			return FALSE;
		if(bytes_read < 1)
		{
			Df1_Error_Number = 306;
			sprintf(Df1_Error_String,"Df1_Get_Symbol:bytes_read was too small(%d).",bytes_read);
			return FALSE;
		}
#if LOGGING > 5
		Df1_Log_Format(DF1_LOG_BIT_DF1,"Df1_Get_Symbol: Second byte %s.",Df1_Print_Symbol(c2));
#endif /* LOGGING */
		(*b) = c2;
		switch (c2)
		{
			case DLE: 
				(*flag) = DATA_FLAG;
				break;
			case ETX: 
			case STX:
			case ENQ:
			case ACK:
			case NAK:
				(*flag) =  CONTROL_FLAG;
				break;
			default:
				(*flag) = DATA_FLAG;
				break;
		}/* end switch */
#if LOGGING > 5
		Df1_Log_Format(DF1_LOG_BIT_DF1,"Df1_Get_Symbol: (Second) Byte = %s, Flag = %d.",
			       Df1_Print_Symbol((*b)),(*flag));
#endif /* LOGGING */
	}
	else
	{
		(*b) = c1;
		(*flag) = DATA_FLAG;
#if LOGGING > 5
		Df1_Log_Format(DF1_LOG_BIT_DF1,"Df1_Get_Symbol: Byte = %s, Flag = %d.",Df1_Print_Symbol((*b)),(*flag));
#endif /* LOGGING */
	}
	return TRUE;
}	

/**
 * Put two bytes into a word.
 * @param lowb The low byte.
 * @param highb The high byte.
 * @return A word containing the 2 bytes.
 */
static word Df1_Bytes2Word(byte lowb, byte highb)
{
	word w;
	char c[2];/*{lowb,highb};*/
	c[0]=lowb;
	c[1]=highb;
	memcpy(&w,c,sizeof(w));
	return w;
}

/**
 * Add a word to the buffer.
 * @param buffer The buffer to add the word to.
 * @param value The word to add.
 * @return The new buffer size.
 * @see #TBuffer
 */
static int Df1_Add_Word2Buffer(TBuffer * buffer, word value)
{
	memcpy(buffer->data+buffer->size, &value,sizeof(value));
	buffer->size += sizeof(value);
	return buffer->size;
}	

/**
 * Add a byte to the buffer.
 * @param buffer The buffer to add the byte to.
 * @param value The byte to add.
 * @return The new buffer size.
 * @see #TBuffer
 */
static int Df1_Add_Byte2Buffer(TBuffer * buffer, byte value) 
{
	memcpy(buffer->data+buffer->size, &value,sizeof(value));
	buffer->size += sizeof(value);
	return buffer->size;
}	

/**
 * Add data to the buffer.
 * @param buffer The buffer to add the data to.
 * @param data The data to add.
 * @param size The length of the data.
 * @return The new buffer size.
 * @see #TBuffer
 */
static int Df1_Add_Data2Buffer(TBuffer * buffer, void * data, byte size) 
{
	memcpy(buffer->data+buffer->size, data,size);
	buffer->size += size;
	return buffer->size;
}	

/**
 *  Add DLE if DLE exist in buffer.
 * @param buffer The buffer to add the data to, of type TBuffer.
 * @param msg The message to add.
 * @return The new buffer size.
 * @see #TBuffer
 * @see #TMsg
 * @see #Df1_Add_Byte2Buffer
 */
static int Df1_Add_Data2BufferWithDLE(TBuffer * buffer, TMsg msg)
{
	byte  i;
	byte databyte[262];
	memcpy(&databyte, &msg,sizeof(msg));
	for (i=0;i<msg.size+6;i++)
	{
		if (databyte[i]==DLE)
			Df1_Add_Byte2Buffer(buffer,DLE);
		Df1_Add_Byte2Buffer(buffer,databyte[i]);
	}	
	return buffer->size;
}

/**
 * Return a string representation of a symbol.
 * @param c The byte to translate.
 * @return A pointer to a string containing the symbol.
 * @see #STX
 * @see #ETX
 * @see #ENQ
 * @see #ACK
 * @see #NAK
 * @see #DLE
 */
static char * Df1_Print_Symbol(byte c)
{
	static char buff[32];

	switch (c)
	{
		case STX :
			return "STX";
			break;
		case ETX :
			return "ETX";
			break;	
		case ENQ :
			return "ENQ";
			break;
		case ACK :
			return "ACK";
			break;		
		case NAK :
		        return "NAK";
			break;		
		case DLE :
			return "DLE";
			break;		
		default : 
			sprintf(buff,"%#02x",c);
			return buff;
	}
}

/**
 * Compute CRC for a TBuffer
 * @param buffer The buffer to compute for.
 * @return A word containing the CRC.
 * @see #Df1_Calc_Crc
 */
static word Df1_Compute_Crc (TBuffer * buffer) 
{
	byte x;
	word crc = 0;

	for (x=0;x<buffer->size;x++)
	{ 
		crc = Df1_Calc_Crc (crc, buffer->data[x]);
	}
	crc = Df1_Calc_Crc (crc,ETX);
	return (crc);
}

/**
 * Calculate the CRC from the current CRC and the specified word.
 * @param crc The current CRC.
 * @param buffer The next word.
 * @return The updated CRC.
 * @see #Df1_Compute_Crc
 */
static word Df1_Calc_Crc (word crc, word buffer) 
{
	word temp1, y;
	temp1 = crc ^ buffer;
	crc = (crc & 0xff00) | (temp1 & 0xff);
	for (y = 0; y < 8; y++)
	{
		if (crc & 1)
		{	  
			crc = crc >> 1;
			crc ^= 0xa001;
		}
		else
			crc = crc >> 1;
	}
	return crc;
}

/*
** $Log: not supported by cvs2svn $
*/

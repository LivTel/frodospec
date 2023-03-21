/* df1.h
** FrodoSpec Micrologix 1100 df1 library
** $Header: /home/cjm/cvs/frodospec/df1/include/df1.h,v 1.1 2023-03-21 14:34:52 cjm Exp $
*/
#ifndef DF1_H
#define DF1_H

#include "df1_interface.h"

/* hash defines */
/**
 * DF1 protocol hash define.
 */
#define DLE 0x10
/**
 * DF1 protocol hash define.
 */
#define STX 0x02
/**
 * DF1 protocol hash define.
 */
#define ETX 0X03
/**
 * DF1 protocol hash define.
 */
#define ENQ 0x05
/**
 * DF1 protocol hash define.
 */
#define ACK 0x06
/**
 * DF1 protocol hash define.
 */
#define NAK 0x15

/**
 * DF1 protocol hash define.
 */
#define DEST 0x01
/**
 * DF1 protocol hash define.
 */
#define SOURCE 0x00

/**
 * DF1 protocol hash define. PLC Type.
 */
#define SLC 1
/**
 * DF1 protocol hash define. PLC Type.
 */
#define PLC 2
/**
 * Df1_Get_Symbol Flag type.
 * @see #Df1_Get_Symbol
 */
#define DATA_FLAG    1
/**
 * Df1_Get_Symbol Flag type.
 * @see #Df1_Get_Symbol
 */
#define CONTROL_FLAG 2

/* type definitions */
/**
 * DF1 Protocol type definition.
 */
typedef unsigned char byte;	/* create byte type */
/**
 * DF1 Protocol type definition.
 */
typedef unsigned short word;	/* create word type */

/**
 * DF1 Protocol TMsg structure.
 */
typedef struct 
{
	byte dst;
	byte src;
	byte cmd;
	byte sts;
	word tns;
	byte data[255];
	byte size;
} TMsg;

/**
 * DF1 Protocol TBuffer structure.
 */
typedef struct
{
	byte data[512];
	byte size;
} TBuffer;

/**
 * DF1 Protocol TThree_Address_Fields structure.
 */
typedef struct {
	byte size;
	byte fileNumber;
	byte fileType;
	byte eleNumber;
	byte s_eleNumber;
} TThree_Address_Fields;

/**
 * DF1 Protocol TCmd.
 */
typedef struct 
{
	byte fnc;
	byte size;
	byte fileNumber;
	byte fileType;
	byte eleNumber;
	byte s_eleNumber;
} TCmd;

/**
 * DF1 Protocol TCmd4.
 */
typedef struct
{
	byte fnc;
	byte size;
	byte fileNumber;
	byte fileType;
	byte eleNumber;
	byte s_eleNumber;
	word maskbyte;
	word value;
} TCmd4;

/* external functions */
extern int Df1_Send(Df1_Interface_Handle_T *handle,TMsg df1_data);
extern int Df1_Receive(Df1_Interface_Handle_T *handle,TMsg *df1_data);
extern int Df1_Calc_Address(char *straddress,TThree_Address_Fields *address);

#endif
/*
** $Log: not supported by cvs2svn $
*/

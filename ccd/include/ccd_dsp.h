/* ccd_dsp.h  -*- mode: Fundamental;-*-
** $Header: /home/cjm/cvs/frodospec/ccd/include/ccd_dsp.h,v 0.3 2000-02-10 12:00:14 cjm Exp $
*/
#ifndef CCD_DSP_H
#define CCD_DSP_H

#include "ccd_global.h"

/**
 * This enum defines board identifiers.
 * These are usually written to the PCI boards using CCD_PCI_IOCTL_SET_DESTINATION, after being ored with
 * the number of arguments left shifted 16 bits.
 * <ul>
 * <li>CCD_DSP_HOST_BOARD_ID The source/destination byte for the host computer.
 * <li>CCD_DSP_INTERFACE_BOARD_ID The source/destination byte for the computer/controller interface device. 
 * <li>CCD_DSP_TIM_BOARD_ID The source/destination byte for the timing board.
 * <li>CCD_DSP_UTIL_BOARD_ID The source/destination byte for the utility board.
 * </ul>
 */
enum CCD_DSP_BOARD_ID
{
	CCD_DSP_HOST_BOARD_ID=0,CCD_DSP_INTERFACE_BOARD_ID=1,CCD_DSP_TIM_BOARD_ID=2,
	CCD_DSP_UTIL_BOARD_ID=3
};

/**
 * Macro to check whether the board_id is a legal board_id. Note, currently does not include the
 * manual board in the test, this should never be used.
 */
#define CCD_DSP_IS_BOARD_ID(board_id)	(((board_id) == CCD_DSP_HOST_BOARD_ID)|| \
				((board_id) == CCD_DSP_INTERFACE_BOARD_ID)||((board_id) == CCD_DSP_TIM_BOARD_ID)|| \
				((board_id) == CCD_DSP_UTIL_BOARD_ID))

/**
 * Enum with values identifying which memory space an address refers to, for operations sich as reading and writing
 * memory locations.
 * <ul>
 * <li>CCD_DSP_MEM_SPACE_D is the DRAM memory space.
 * <li>CCD_DSP_MEM_SPACE_R is the ROM memory space.
 * <li>CCD_DSP_MEM_SPACE_P is the P DSP memory space.
 * <li>CCD_DSP_MEM_SPACE_X is the X DSP memory space.
 * <li>CCD_DSP_MEM_SPACE_Y is the Y DSP memory space.
 * </ul>
 * @see #CCD_DSP_Command_RDM
 * @see #CCD_DSP_Command_WRM
 */
enum CCD_DSP_MEM_SPACE
{
	CCD_DSP_MEM_SPACE_D=0x5f5f44,	/*  Ascii __D */
	CCD_DSP_MEM_SPACE_R=0x5f5f52,	/*  Ascii __R */
	CCD_DSP_MEM_SPACE_P=0x5f5f50,	/*  Ascii __P */
	CCD_DSP_MEM_SPACE_X=0x5f5f58,	/*  Ascii __X */
	CCD_DSP_MEM_SPACE_Y=0x5f5f59	/*  Ascii __Y */
};

/**
 * Macro to check whether the mem_space  is a legal memory space.
 */
#define CCD_DSP_IS_MEMORY_SPACE(mem_space)	(((mem_space) == CCD_DSP_MEM_SPACE_P)|| \
	((mem_space) == CCD_DSP_MEM_SPACE_X)||((mem_space) == CCD_DSP_MEM_SPACE_Y)|| \
	((mem_space) == CCD_DSP_MEM_SPACE_R))

/**
 * These are allowable paramaters for the gains (Gen two only).  Please
 * note that unlike the other commmands listed in this file, the hex numbers
 * are NOT the ASCII (character) values of the commands.
 * Gain paramater sent with the <a href="#CCD_DSP_SGN">SGN</a>(set gain) command. 
 * <ul>
 * <li>CCD_DSP_GAIN_ONE Sets the gain to 1.
 * <li>CCD_DSP_GAIN_TWO Sets the gain to 2.
 * <li>CCD_DSP_GAIN_FOUR Sets the gain to 4.75.
 * <li>CCD_DSP_GAIN_NINE Sets the gain to 9.5.
 * </ul>
 */
enum CCD_DSP_GAIN
{
	CCD_DSP_GAIN_ONE=0x1,
	CCD_DSP_GAIN_TWO=0x2,	/* 2 gain */
	CCD_DSP_GAIN_FOUR=0x5,	/* 4.75 gain */
	CCD_DSP_GAIN_NINE=0xa	/* 9.5 gain */
};

/**
 * Macro to check whether the gain is a legal value to passed into the <a href="#CCD_DSP_SGN">SGN</a> command.
 */
#define CCD_DSP_IS_GAIN(gain)	(((gain) == CCD_DSP_GAIN_ONE)||((gain) == CCD_DSP_GAIN_TWO)|| \
	((gain) == CCD_DSP_GAIN_FOUR)||((gain) == CCD_DSP_GAIN_NINE))

/* These #define/enum definitions should match with those in CCDLibrary.java */
/**
 * Return value from <a href="#CCD_DSP_Get_Exposure_Status">CCD_DSP_Get_Exposure_Status</a>. 
 * <ul>
 * <li>CCD_DSP_EXPOSURE_STATUS_NONE means the library is not currently performing an exposure.
 * <li>CCD_DSP_EXPOSURE_STATUS_EXPOSE means the library is currently performing an exposure.
 * <li>CCD_DSP_EXPOSURE_STATUS_READOUT means the library is currently reading out data from the ccd.
 * </ul>
 * @see #CCD_DSP_Get_Exposure_Status
 */
enum CCD_DSP_EXPOSURE_STATUS
{
	CCD_DSP_EXPOSURE_STATUS_NONE,CCD_DSP_EXPOSURE_STATUS_EXPOSE,CCD_DSP_EXPOSURE_STATUS_READOUT
};

/* These enum definitions should match with those in CCDLibrary.java */
/**
 * Deinterlace type. The possible values are:
 * <ul>
 * <li>CCD_DSP_DEINTERLACE_SINGLE - This setting does no deinterlacing, 
 * 	as the CCD was read out from a single readout.
 * <li>CCD_DSP_DEINTERLACE_SPLIT_PARALLEL - This setting deinterlaces split parallel readout.
 * <li>CCD_DSP_DEINTERLACE_SPLIT_SERIAL - This setting deinterlaces split serial readout.
 * <li>CCD_DSP_DEINTERLACE_SPLIT_QUAD - This setting deinterlaces split quad readout.
 * </ul>
 */
enum CCD_DSP_DEINTERLACE_TYPE
{
	CCD_DSP_DEINTERLACE_SINGLE,CCD_DSP_DEINTERLACE_SPLIT_PARALLEL,
	CCD_DSP_DEINTERLACE_SPLIT_SERIAL,CCD_DSP_DEINTERLACE_SPLIT_QUAD
};

/**
 * Macro to check whether the deinterlace type is a legal value.
 */
#define CCD_DSP_IS_DEINTERLACE_TYPE(type)	(((type) == CCD_DSP_DEINTERLACE_SINGLE)|| \
	((type) == CCD_DSP_DEINTERLACE_SPLIT_PARALLEL)||((type) == CCD_DSP_DEINTERLACE_SPLIT_SERIAL)|| \
	((type) == CCD_DSP_DEINTERLACE_SPLIT_QUAD))

/* Various CCD_DSp routine return these values to indicate success/failure */
/**
 * Return value from the SDSU CCD Controller. This means the last command succeeded.
 */
#define CCD_DSP_DON		0x444f4e /* DON */
/**
 * Return value from the SDSU CCD Controller. This means the last command failed.
 */
#define CCD_DSP_ERR		0x455252 /* ERR */
/**
 * Timing board command that means Set GaiN. This sets the gains of all the video processors. The integrator
 * speed is also set using this command to slow or fast.
 */
#define CCD_DSP_SGN		0x53474e	/* SGN */
/**
 * Utility board command that means SYstem Reset. This is sent in reply to a reset
 * controller command.
 */
#define CCD_DSP_SYR		0x535952 /* SYR */

extern void CCD_DSP_Initialise(void);
/* Boot commands */
extern int CCD_DSP_Command_LDA(enum CCD_DSP_BOARD_ID board_id,int application_number);
extern int CCD_DSP_Command_RDM(enum CCD_DSP_BOARD_ID board_id,enum CCD_DSP_MEM_SPACE mem_space,int address);
extern int CCD_DSP_Command_TDL(enum CCD_DSP_BOARD_ID board_id,int data);
extern int CCD_DSP_Command_WRM(enum CCD_DSP_BOARD_ID board_id,enum CCD_DSP_MEM_SPACE mem_space,int address,int data);
extern int CCD_DSP_Command_WRM_No_Reply(enum CCD_DSP_BOARD_ID board_id,enum CCD_DSP_MEM_SPACE mem_space,int address,
	int data);
/* timing board commands */
extern int CCD_DSP_Command_ABR(void);
extern int CCD_DSP_Command_CLR(void);
extern int CCD_DSP_Command_IDL(void);
extern int CCD_DSP_Command_RDC(int ncols,int nrows,int numbytes,
	enum CCD_DSP_DEINTERLACE_TYPE deinterlace_type,char *filename);
extern int CCD_DSP_Command_SBV(void);
extern int CCD_DSP_Command_SGN(enum CCD_DSP_GAIN gain,int speed);
extern int CCD_DSP_Command_STP(void);
extern int CCD_DSP_Command_Set_NCols(int ncols);
extern int CCD_DSP_Command_Set_NRows(int nrows);
/* utility board commands */
extern int CCD_DSP_Command_AEX(void);
extern int CCD_DSP_Command_CSH(void);
extern int CCD_DSP_Command_OSH(void);
extern int CCD_DSP_Command_PEX(void);
extern int CCD_DSP_Command_PON(void);
extern int CCD_DSP_Command_POF(void);
extern int CCD_DSP_Command_REX(void);
extern int CCD_DSP_Command_Read_Temperature(void);
extern int CCD_DSP_Command_Set_Temperature(int adu);
extern int CCD_DSP_Command_SEX(int ncols,int nrows,int numbytes,int msecs,
	enum CCD_DSP_DEINTERLACE_TYPE deinterlace_type,char *filename);
extern int CCD_DSP_Command_Reset(void);
extern int CCD_DSP_Command_Set_Util_Options(int bit_value);
extern int CCD_DSP_Command_Set_Exposure_Time(int msecs);
extern int CCD_DSP_Download(enum CCD_DSP_BOARD_ID board_id,char *filename);
extern int CCD_DSP_Get_Abort(void);
extern int CCD_DSP_Set_Abort(int value);
extern enum CCD_DSP_EXPOSURE_STATUS CCD_DSP_Get_Exposure_Status(void);
extern int CCD_DSP_Get_Error_Number(void);
extern void CCD_DSP_Error(void);
extern void CCD_DSP_Error_String(char *error_string);
extern void CCD_DSP_Warning(void);

#endif

/* ccd_dsp.h  -*- mode: Fundamental;-*-
** $Header: /home/cjm/cvs/frodospec/ccd/include/ccd_dsp.h,v 0.19 2001-01-23 18:23:34 cjm Exp $
*/
#ifndef CCD_DSP_H
#define CCD_DSP_H
#include <time.h>
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

/* These enum definitions should match with those in CCDLibrary.java */
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
 * <li>CCD_DSP_EXPOSURE_STATUS_CLEAR means the library is currently clearing the ccd.
 * <li>CCD_DSP_EXPOSURE_STATUS_EXPOSE means the library is currently performing an exposure.
 * <li>CCD_DSP_EXPOSURE_STATUS_READOUT means the library is currently reading out data from the ccd.
 * </ul>
 * @see #CCD_DSP_Get_Exposure_Status
 */
enum CCD_DSP_EXPOSURE_STATUS
{
	CCD_DSP_EXPOSURE_STATUS_NONE,CCD_DSP_EXPOSURE_STATUS_CLEAR,
	CCD_DSP_EXPOSURE_STATUS_EXPOSE,CCD_DSP_EXPOSURE_STATUS_READOUT
};

/**
 * The maximum exposure length that the controller can expose the CCD for, in milliseconds.
 * This is limited by the size of a DSP word (24 bits). The word is signed, so this value is
 * (2^23)-1. This is 8388 seconds, or 2 hours 19 minutes,
 */
#define CCD_DSP_EXPOSURE_MAX_LENGTH		(8388607)

/* These #define/enum definitions should match with those in CCDLibrary.java */
/**
 * Return value from <a href="#CCD_DSP_Get_Filter_Wheel_Status">CCD_DSP_Get_Filter_Wheel_Status</a>. 
 * <ul>
 * <li>CCD_DSP_FILTER_WHEEL_STATUS_NONE means the library is not currently doing anything with the filter wheels.
 * <li>CCD_DSP_FILTER_WHEEL_STATUS_MOVING means the library is moving the filter wheels.
 * <li>CCD_DSP_FILTER_WHEEL_STATUS_RESETING means the library is reseting the filter wheels.
 * <li>CCD_DSP_FILTER_WHEEL_STATUS_ABORTED means the library is aborting the filter wheels.
 * </ul>
 * @see #CCD_DSP_Get_Filter_Wheel_Status
 */
enum CCD_DSP_FILTER_WHEEL_STATUS
{
	CCD_DSP_FILTER_WHEEL_STATUS_NONE,CCD_DSP_FILTER_WHEEL_STATUS_MOVING,CCD_DSP_FILTER_WHEEL_STATUS_RESETING,
	CCD_DSP_FILTER_WHEEL_STATUS_ABORTED
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

/* These enum definitions should match with those in CCDLibrary.java */
/**
 * Enum with values identifying which output amplifier to select for reading out the CCD.
 * Used with the CCD_DSP_SOS (select output source) manual command.
 * <ul>
 * <li>CCD_DSP_AMPLIFIER_LEFT selects the left amplifier.
 * <li>CCD_DSP_AMPLIFIER_RIGHT selects the right amplifier.
 * <li>CCD_DSP_AMPLIFIER_BOTH selects both amplifiers.
 * </ul>
 * @see #CCD_DSP_SOS
 * @see #CCD_DSP_Command_SOS
 */
enum CCD_DSP_AMPLIFIER
{
	CCD_DSP_AMPLIFIER_LEFT =0x5f5f4c, 	/* Ascii __L */
	CCD_DSP_AMPLIFIER_RIGHT=0x5f5f52, 	/* Ascii __R */
	CCD_DSP_AMPLIFIER_BOTH =0x5f4c52  	/* Ascii _LR */
};

/**
 * Macro to check whether the amplifier is a legal value to passed into the <a href="#CCD_DSP_SOS">SOS</a> command.
 */
#define CCD_DSP_IS_AMPLIFIER(amplifier)	(((amplifier) == CCD_DSP_AMPLIFIER_LEFT)|| \
	((amplifier) == CCD_DSP_AMPLIFIER_RIGHT)||((amplifier) == CCD_DSP_AMPLIFIER_BOTH))

/* Various CCD_DSP routine return these values to indicate success/failure */
/**
 * Return value from the SDSU CCD Controller. This means the last command succeeded.
 */
#define CCD_DSP_DON		(0x444f4e) /* DON */
/**
 * Return value from the SDSU CCD Controller. This means the last command failed.
 */
#define CCD_DSP_ERR		(0x455252) /* ERR */
/**
 * Device Driver value, used by the device driver when it times out whilst waiting for a reply value.
 * This means the GET_REPLY command sent to the device driver did not receive a reply from the PCI DSP.
 */
#define CCD_DSP_TOUT		(0x544F5554) /* TOUT */
/**
 * Timing board command that means SYstem Reset. This is sent in reply to a reset
 * controller command.
 */
#define CCD_DSP_SYR		(0x535952) /* SYR */
/**
 * Another kind of reset command. This one isn't used at the moment.
 */
#define CCD_DSP_RST		(0x00525354) /* RST */

/* Manual DSP commands. */
/**
 * Timing board command that means Set GaiN. This sets the gains of all the video processors. The integrator
 * speed is also set using this command to slow or fast.
 */
#define CCD_DSP_SGN		(0x53474e)	/* SGN */
/**
 * Timing board command that means Set Output Source. This sets which output amplifiers on the chip to read
 * out from when a readout is performed. It takes one argument, which selects the amplifier.
 * @see #CCD_DSP_AMPLIFIER
 */
#define CCD_DSP_SOS		(0x534f53)	/* SOS */
/**
 * Utility board command that means Filter Wheel Abort. This stops any filter wheel movement taking place. 
 * It takes no arguments.
 * @see #CCD_DSP_FWM
 * @see #CCD_DSP_FWR
 */
#define CCD_DSP_FWA		(0x465741)	/* FWA */
/**
 * Utility board command that means Filter Wheel Move. This moves a specified filter wheel in a specified direction
 * a specified number of positions. 
 * It takes three arguments:
 * <ul>
 * <li><b>wheel</b>. Which wheel to move, [0|1].
 * <li><b>direction</b>. Which direction to move the wheel, either [0|1].
 * <li><b>no. of positions</b>. The number of positions to move in the specified direction, a number, usually less than
 * 	seven and greater than zero.
 * </ul>
 */
#define CCD_DSP_FWM		(0x46574d)	/* FWM */
/**
 * Utility board command that means Filter Wheel Reset. This drives a specified filter wheel into it's home position. 
 * It takes one argument:
 * <ul>
 * <li><b>wheel</b>. Which wheel to move, [0|1].
 * </ul>
 */
#define CCD_DSP_FWR		(0x465752)	/* FWR */

/**
 * This hash definition represents one of the bits present in the controller status word, which is on
 * the timing board in X memory at location 0.
 * This is retrieved using READ_MEMORY and set using WRITE_MEMORY.
 * When set, this bit means START_EXPOSURE commands sent to the controller will open the shutter.
 */
#define CCD_DSP_CONTROLLER_STATUS_OPEN_SHUTTER_BIT	(1 << 11)

/**
 * This hash definition represents one of the bits present in the timing board controller configuration word.
 * This is retrieved using READ_CONTROLLER_STATUS and set using WRITE_CONTROLLER_STATUS.
 */
#define CCD_DSP_CONTROLLER_CONFIG_BIT_CCD_REV3B			(0x0)
#define CCD_DSP_CONTROLLER_CONFIG_BIT_CCD_GENI			(0x1)
#define CCD_DSP_CONTROLLER_CONFIG_BIT_IR_REV4C			(0x2)
#define CCD_DSP_CONTROLLER_CONFIG_BIT_IR_COADDER		(0x3)
#define CCD_DSP_CONTROLLER_CONFIG_BIT_TIM_REV4B			(0x0)
#define CCD_DSP_CONTROLLER_CONFIG_BIT_TIM_GENI			(0x8)
#define CCD_DSP_CONTROLLER_CONFIG_BIT_UTILITY_REV3		(0x20)
#define CCD_DSP_CONTROLLER_CONFIG_BIT_SHUTTER			(0x80)
#define CCD_DSP_CONTROLLER_CONFIG_BIT_NONLINEAR_TEMP_CONV	(0x100)
#define CCD_DSP_CONTROLLER_CONFIG_BIT_LINEAR_TEMP_CONV		(0x200)
#define CCD_DSP_CONTROLLER_CONFIG_BIT_SUBARRAY			(0x400)
#define CCD_DSP_CONTROLLER_CONFIG_BIT_BINNING			(0x800)
#define CCD_DSP_CONTROLLER_CONFIG_BIT_SERIAL_SPLIT		(0x1000)
#define CCD_DSP_CONTROLLER_CONFIG_BIT_PARALLEL			(0x2000)
#define CCD_DSP_CONTROLLER_CONFIG_BIT_BOTH_READOUTS		(0x3000)
#define CCD_DSP_CONTROLLER_CONFIG_BIT_MPP_CAPABLE		(0x4000)

extern int CCD_DSP_Initialise(void);
/* Boot commands */
extern int CCD_DSP_Command_LDA(enum CCD_DSP_BOARD_ID board_id,int application_number);
extern int CCD_DSP_Command_RDM(enum CCD_DSP_BOARD_ID board_id,enum CCD_DSP_MEM_SPACE mem_space,int address);
extern int CCD_DSP_Command_TDL(enum CCD_DSP_BOARD_ID board_id,int data);
extern int CCD_DSP_Command_WRM(enum CCD_DSP_BOARD_ID board_id,enum CCD_DSP_MEM_SPACE mem_space,int address,int data);
/* timing board commands */
extern int CCD_DSP_Command_ABR(void);
extern int CCD_DSP_Command_CLR(void);
extern int CCD_DSP_Command_RDC(void);
extern int CCD_DSP_Command_IDL(void);
extern int CCD_DSP_Command_RDI(int ncols,int nrows,enum CCD_DSP_DEINTERLACE_TYPE deinterlace_type,int send_rdi,
	char *filename);
extern int CCD_DSP_Command_SBV(void);
extern int CCD_DSP_Command_SGN(enum CCD_DSP_GAIN gain,int speed);
extern int CCD_DSP_Command_SOS(enum CCD_DSP_AMPLIFIER amplifier);
extern int CCD_DSP_Command_STP(void);
extern int CCD_DSP_Command_Set_NCols(int ncols);
extern int CCD_DSP_Command_Set_NRows(int nrows);

extern int CCD_DSP_Command_AEX(void);
extern int CCD_DSP_Command_CSH(void);
extern int CCD_DSP_Command_OSH(void);
extern int CCD_DSP_Command_PEX(void);
extern int CCD_DSP_Command_PON(void);
extern int CCD_DSP_Command_POF(void);
extern int CCD_DSP_Command_REX(void);
extern int CCD_DSP_Command_Read_Temperature(void);
extern int CCD_DSP_Command_Set_Temperature(int adu);
extern int CCD_DSP_Command_SEX(struct timespec start_time,int exposure_time,int ncols,int nrows,
	enum CCD_DSP_DEINTERLACE_TYPE deinterlace_type,char *filename);
extern int CCD_DSP_Command_Reset(void);
extern int CCD_DSP_Command_Flush_Reply_Buffer(void);
extern int CCD_DSP_Command_Read_Controller_Status(void);
extern int CCD_DSP_Command_Write_Controller_Status(int bit_value);
extern int CCD_DSP_Command_PCI_PC_Reset(void);
extern int CCD_DSP_Command_Read_PCI_Status(void);
extern int CCD_DSP_Command_Set_Exposure_Time(int msecs);
extern int CCD_DSP_Command_Read_Exposure_Time(void);
extern int CCD_DSP_Command_FWA(void);
extern int CCD_DSP_Command_FWM(int wheel,int direction,int posn_count);
extern int CCD_DSP_Command_FWR(int wheel);
extern int CCD_DSP_Download(enum CCD_DSP_BOARD_ID board_id,char *filename);
extern int CCD_DSP_Get_Abort(void);
extern int CCD_DSP_Set_Abort(int value);
extern enum CCD_DSP_EXPOSURE_STATUS CCD_DSP_Get_Exposure_Status(void);
extern struct timespec CCD_DSP_Get_Exposure_Start_Time(void);
extern int CCD_DSP_Get_Exposure_Length(void);
extern void CCD_DSP_Set_Start_Exposure_Clear_Time(int time);
extern int CCD_DSP_Get_Start_Exposure_Clear_Time(void);
extern void CCD_DSP_Set_Start_Exposure_Offset_Time(int time);
extern int CCD_DSP_Get_Start_Exposure_Offset_Time(void);
extern void CCD_DSP_Set_Readout_Remaining_Time(int time);
extern int CCD_DSP_Get_Readout_Remaining_Time(void);
extern void CCD_DSP_Set_Exposure_Start_Time(void);
extern enum CCD_DSP_FILTER_WHEEL_STATUS CCD_DSP_Get_Filter_Wheel_Status(void);
extern void CCD_DSP_Set_Filter_Wheel_Steps_Per_Position(int steps);
extern void CCD_DSP_Set_Filter_Wheel_Milliseconds_Per_Step(int ms);
extern int CCD_DSP_Get_Error_Number(void);
extern void CCD_DSP_Error(void);
extern void CCD_DSP_Error_String(char *error_string);
extern void CCD_DSP_Warning(void);

#endif

/* ccd_setup.h  -*- mode: Fundamental;-*-
** $Header: /home/cjm/cvs/frodospec/ccd/include/ccd_setup.h,v 0.1 2000-01-25 15:03:32 cjm Exp $
*/
#ifndef CCD_SETUP_H
#define CCD_SETUP_H

#include "ccd_dsp.h"

/* These #define definitions should match with those in CCDLibrary.java */
/**
 * Setup flag passed to CCD_Setup_Setup_CCD, to reset the sdsu controller.
 * @see #CCD_Setup_Setup_CCD
 */
#define CCD_SETUP_FLAG_RESET_CONTROLLER		(1<<0)
/**
 * Setup flag passed to CCD_Setup_Setup_CCD, to do a hardware test.
 * @see #CCD_Setup_Setup_CCD
 */
#define CCD_SETUP_FLAG_HARDWARE_TEST		(1<<1)
/**
 * Setup flag passed to CCD_Setup_Setup_CCD, to send an application to the timing board.
 * @see #CCD_Setup_Setup_CCD
 */
#define CCD_SETUP_FLAG_TIMING_BOARD		(1<<2)
/**
 * Setup flag passed to CCD_Setup_Setup_CCD, to send an application to the utility board.
 * @see #CCD_Setup_Setup_CCD
 */
#define CCD_SETUP_FLAG_UTILITY_BOARD		(1<<3)
/**
 * Setup flag passed to CCD_Setup_Setup_CCD, to turn the power on.
 * @see #CCD_Setup_Setup_CCD
 */
#define CCD_SETUP_FLAG_POWER_ON			(1<<4)
/**
 * Setup flag passed to CCD_Setup_Setup_CCD, to set a target CCD temperature.
 * @see #CCD_Setup_Setup_CCD
 */
#define CCD_SETUP_FLAG_TARGET_CCD_TEMP		(1<<5)
/**
 * Setup flag passed to CCD_Setup_Setup_CCD, to set the CCD gain/speed.
 * @see #CCD_Setup_Setup_CCD
 */
#define CCD_SETUP_FLAG_GAIN			(1<<6)
/**
 * Setup flag passed to CCD_Setup_Setup_CCD, to decide whether to set the idle status.
 * @see #CCD_Setup_Setup_CCD
 */
#define CCD_SETUP_FLAG_IDLE			(1<<7)
/**
 * Setup flag passed to CCD_Setup_Setup_CCD, to send the CCD dimensions/binning to the boards.
 * @see #CCD_Setup_Setup_CCD
 */
#define CCD_SETUP_FLAG_DIMENSIONS		(1<<8)
/**
 * Setup flag passed to CCD_Setup_Setup_CCD, to decide to set the deinterlace type.
 * @see #CCD_Setup_Setup_CCD
 */
#define CCD_SETUP_FLAG_DEINTERLACE		(1<<9)
/**
 * Default setup flag passed to CCD_Setup_Setup_CCD, which sets up all information.
 * @see #CCD_Setup_Setup_CCD
 */
#define CCD_SETUP_FLAG_ALL			(CCD_SETUP_FLAG_RESET_CONTROLLER|CCD_SETUP_FLAG_HARDWARE_TEST| \
						CCD_SETUP_FLAG_TIMING_BOARD| \
						CCD_SETUP_FLAG_UTILITY_BOARD|CCD_SETUP_FLAG_POWER_ON| \
						CCD_SETUP_FLAG_TARGET_CCD_TEMP| \
						CCD_SETUP_FLAG_GAIN|CCD_SETUP_FLAG_IDLE|CCD_SETUP_FLAG_DIMENSIONS| \
						CCD_SETUP_FLAG_DEINTERLACE)

/* These enum definitions should match with those in CCDLibrary.java */
/**
 * Setup Load Type passed to CCD_Setup_Setup_CCD as a load_type parameter, to load DSP application code from
 * a certain location. The possible values are:
 * <ul>
 * <li>CCD_SETUP_LOAD_APPLICATION - Load DSP application code from EEPROM.
 * <li>CCD_SETUP_LOAD_FILENAME - Load DSP application code from a file.
 * </ul>
 * @see #CCD_Setup_Setup_CCD
 */
enum CCD_SETUP_LOAD_TYPE
{
	CCD_SETUP_LOAD_APPLICATION,CCD_SETUP_LOAD_FILENAME
};

/**
 * Macro to check whether the load_type is a legal load type to load DSP applications during setup.
 */
#define CCD_SETUP_IS_LOAD_TYPE(load_type)	(((load_type) == CCD_SETUP_LOAD_APPLICATION)|| \
	((load_type) == CCD_SETUP_LOAD_FILENAME))

extern void CCD_Setup_Initialise(void);
extern int CCD_Setup_Setup_CCD(int setup_flags,
	enum CCD_SETUP_LOAD_TYPE timing_load_type,int timing_application_number,char *timing_filename,
	enum CCD_SETUP_LOAD_TYPE utility_load_type,int utility_application_number,char *utility_filename,
	double target_temperature,enum CCD_DSP_GAIN gain,int gain_speed,int idle,
	int ncols,int nrows,int nsbin,int npbin,enum CCD_DSP_DEINTERLACE_TYPE deinterlace_setting);
extern void CCD_Setup_Abort(void);
extern int CCD_Setup_Get_NCols(void);
extern int CCD_Setup_Get_NRows(void);
extern enum CCD_DSP_DEINTERLACE_TYPE CCD_Setup_Get_DeInterlace_Type(void);
extern int CCD_Setup_Get_Setup_Complete(void);
extern int CCD_Setup_Get_Setup_In_Progress(void);
extern int CCD_Setup_Get_Error_Number(void);
extern void CCD_Setup_Error(void);
extern void CCD_Setup_Error_String(char *error_string);
#endif

/* ccd_setup.h  -*- mode: Fundamental;-*-
** $Header: /home/cjm/cvs/frodospec/ccd/include/ccd_setup.h,v 0.2 2000-02-02 15:55:24 cjm Exp $
*/
#ifndef CCD_SETUP_H
#define CCD_SETUP_H

#include "ccd_dsp.h"

/**
 * The number of windows the controller can put on the CCD.
 * @see #Setup_Struct
 */
#define CCD_SETUP_WINDOW_COUNT			(4)

/* These hash definitions should match with those in CCDLibrary.java */
/**
 * Window flag used as part of the window_flags bit-field parameter of CCD_Setup_Dimensions to specify the
 * first window position is to be used.
 * @see #CCD_Setup_Dimensions
 */
#define CCD_SETUP_WINDOW_ONE	(1<<0)
/**
 * Window flag used as part of the window_flags bit-field parameter of CCD_Setup_Dimensions to specify the
 * second window position is to be used.
 * @see #CCD_Setup_Dimensions
 */
#define CCD_SETUP_WINDOW_TWO	(1<<1)
/**
 * Window flag used as part of the window_flags bit-field parameter of CCD_Setup_Dimensions to specify the
 * third window position is to be used.
 * @see #CCD_Setup_Dimensions
 */
#define CCD_SETUP_WINDOW_THREE	(1<<2)
/**
 * Window flag used as part of the window_flags bit-field parameter of CCD_Setup_Dimensions to specify the
 * fourth window position is to be used.
 * @see #CCD_Setup_Dimensions
 */
#define CCD_SETUP_WINDOW_FOUR	(1<<3)
/**
 * Window flag used as part of the window_flags bit-field parameter of CCD_Setup_Dimensions to specify all the
 * window positions are to be used.
 * @see #CCD_Setup_Dimensions
 */
#define CCD_SETUP_WINDOW_ALL	(CCD_SETUP_WINDOW_ONE|CCD_SETUP_WINDOW_TWO| \
					CCD_SETUP_WINDOW_THREE|CCD_SETUP_WINDOW_FOUR)

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

/**
 * Structure holding position information for one window on the CCD. Fields are:
 * <dl>
 * <dt>X_Start</dt> <dd>The pixel number of the X start position of the window (upper left corner).</dd>
 * <dt>Y_Start</dt> <dd>The pixel number of the Y start position of the window (upper left corner).</dd>
 * <dt>X_End</dt> <dd>The pixel number of the X end position of the window (lower right corner).</dd>
 * <dt>Y_End</dt> <dd>The pixel number of the Y end position of the window (lower right corner).</dd>
 * </dl>
 * @see #CCD_Setup_Dimensions
 */
struct CCD_Setup_Window_Struct
{
	int X_Start;
	int Y_Start;
	int X_End;
	int Y_End;
};

extern void CCD_Setup_Initialise(void);
extern int CCD_Setup_Startup(
	enum CCD_SETUP_LOAD_TYPE timing_load_type,int timing_application_number,char *timing_filename,
	enum CCD_SETUP_LOAD_TYPE utility_load_type,int utility_application_number,char *utility_filename,
	double target_temperature,enum CCD_DSP_GAIN gain,int gain_speed,int idle);
extern int CCD_Setup_Shutdown(void);
extern int CCD_Setup_Dimensions(int ncols,int nrows,int nsbin,int npbin,
	enum CCD_DSP_DEINTERLACE_TYPE deinterlace_setting,int window_flags,
	struct CCD_Setup_Window_Struct window_list[]);
extern int CCD_Setup_Filter_Wheel(int position_one,int position_two);
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

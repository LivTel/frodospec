/* ccd_setup.h
** $Header: /home/cjm/cvs/frodospec/ccd/include/ccd_setup.h,v 0.16 2011-01-17 10:58:44 cjm Exp $
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
 * Setup Load Type passed to CCD_Setup_Startup as a load_type parameter, to load DSP application code from
 * a certain location. The possible values are:
 * <ul>
 * <li>CCD_SETUP_LOAD_ROM - Load DSP application code from boot ROM. This means CCD_Setup_Startup does nothing.
 * <li>CCD_SETUP_LOAD_APPLICATION - Load DSP application code from EEPROM.
 * <li>CCD_SETUP_LOAD_FILENAME - Load DSP application code from a file.
 * </ul>
 * @see #CCD_Setup_Startup
 */
enum CCD_SETUP_LOAD_TYPE
{
	CCD_SETUP_LOAD_ROM,CCD_SETUP_LOAD_APPLICATION,CCD_SETUP_LOAD_FILENAME
};

/**
 * Macro to check whether the load_type is a legal load type to load DSP applications during setup.
 * @see #CCD_SETUP_LOAD_TYPE
 */
#define CCD_SETUP_IS_LOAD_TYPE(load_type)	(((load_type) == CCD_SETUP_LOAD_ROM)|| \
	((load_type) == CCD_SETUP_LOAD_APPLICATION)||((load_type) == CCD_SETUP_LOAD_FILENAME))

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
extern void CCD_Setup_Data_Initialise(CCD_Interface_Handle_T* handle);
extern int CCD_Setup_Startup(char *class,char *source,CCD_Interface_Handle_T* handle,
        enum CCD_SETUP_LOAD_TYPE pci_load_type,char *pci_filename,
	enum CCD_SETUP_LOAD_TYPE timing_load_type,int timing_application_number,char *timing_filename,
	enum CCD_SETUP_LOAD_TYPE utility_load_type,int utility_application_number,char *utility_filename,
	double target_temperature,enum CCD_DSP_GAIN gain,int gain_speed,int idle);
extern int CCD_Setup_Shutdown(char *class,char *source,CCD_Interface_Handle_T* handle);
extern int CCD_Setup_Dimensions(char *class,char *source,CCD_Interface_Handle_T* handle,
        int ncols,int nrows,int nsbin,int npbin,
	enum CCD_DSP_AMPLIFIER amplifier,enum CCD_DSP_DEINTERLACE_TYPE deinterlace_setting,
	int window_flags,struct CCD_Setup_Window_Struct window_list[]);
extern int CCD_Setup_Hardware_Test(char *class,char *source,CCD_Interface_Handle_T* handle,int test_count);
extern void CCD_Setup_Abort(char *class,char *source,CCD_Interface_Handle_T* handle);
extern int CCD_Setup_Get_NCols(CCD_Interface_Handle_T* handle);
extern int CCD_Setup_Get_NRows(CCD_Interface_Handle_T* handle);
extern int CCD_Setup_Get_NSBin(CCD_Interface_Handle_T* handle);
extern int CCD_Setup_Get_NPBin(CCD_Interface_Handle_T* handle);
extern int CCD_Setup_Get_Readout_Pixel_Count(CCD_Interface_Handle_T* handle);
extern int CCD_Setup_Get_Window_Pixel_Count(CCD_Interface_Handle_T* handle,int window_index);
extern int CCD_Setup_Get_Window_Width(CCD_Interface_Handle_T* handle,int window_index);
extern int CCD_Setup_Get_Window_Height(CCD_Interface_Handle_T* handle,int window_index);
extern enum CCD_DSP_DEINTERLACE_TYPE CCD_Setup_Get_DeInterlace_Type(CCD_Interface_Handle_T* handle);
extern enum CCD_DSP_GAIN CCD_Setup_Get_Gain(CCD_Interface_Handle_T* handle);
extern enum CCD_DSP_AMPLIFIER CCD_Setup_Get_Amplifier(CCD_Interface_Handle_T* handle);
extern int CCD_Setup_Get_Idle(CCD_Interface_Handle_T* handle);
extern int CCD_Setup_Get_Window_Flags(CCD_Interface_Handle_T* handle);
extern int CCD_Setup_Get_Window(CCD_Interface_Handle_T* handle,int window_index,
				struct CCD_Setup_Window_Struct *window);
extern int CCD_Setup_Get_Setup_Complete(CCD_Interface_Handle_T* handle);
extern int CCD_Setup_Get_Setup_In_Progress(CCD_Interface_Handle_T* handle);
extern int CCD_Setup_Get_High_Voltage_Analogue_ADU(char *class,char *source,CCD_Interface_Handle_T* handle,
						   int *hv_adu);
extern int CCD_Setup_Get_Low_Voltage_Analogue_ADU(char *class,char *source,CCD_Interface_Handle_T* handle,int *lv_adu);
extern int CCD_Setup_Get_Minus_Low_Voltage_Analogue_ADU(char *class,char *source,CCD_Interface_Handle_T* handle,
							int *minus_lv_adu);
extern int CCD_Setup_Get_Error_Number(void);
extern void CCD_Setup_Error(void);
extern void CCD_Setup_Error_String(char *error_string);
extern void CCD_Setup_Warning(void);
#endif

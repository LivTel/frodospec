/* ccd_setup.c -*- mode: Fundamental;-*-
** low level ccd library
** $Header: /home/cjm/cvs/frodospec/ccd/c/ccd_setup.c,v 0.3 2000-02-02 15:58:32 cjm Exp $
*/
/**
 * ccd_setup.c contains routines to perform the setting of the SDSU CCD Controller, prior to performing
 * exposures.
 * @author SDSU, Chris Mottram
 * @version $Revision: 0.3 $
 */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include "ccd_global.h"
#include "ccd_dsp.h"
#include "ccd_temperature.h"
#include "ccd_setup.h"

/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id: ccd_setup.c,v 0.3 2000-02-02 15:58:32 cjm Exp $";

/* #defines */
/**
 * The number of filter wheels this controller knows about.
 * @see #Setup_Struct
 */
#define SETUP_FILTER_WHEEL_POSITION_COUNT	(2)
/**
 * Used when performing a short hardware test (in <a href="#Setup_Hardware_Test">Setup_Hardware_Test</a>),
 * This is the number of times each board's data link is tested using the 
 * <a href="ccd_dsp.html#CCD_DSP_TDL">TDL</a> command.
 */
#define SETUP_SHORT_TEST_COUNT		(10)

/**
 * The maximum value that can be sent as a data value argument to the Test Data Link SDSU command.
 * According to the documentation, this can be any 24bit number.
 */
#define TDL_MAX_VALUE 		(16777216)	/* 2^24 */

/* data types */
/**
 * Data type used to hold local data to ccd_setup. Fields are:
 * <dl>
 * <dt>NCols</dt> <dd>The number of columns that will be used on the CCD.</dd>
 * <dt>NRows</dt> <dd>The number of rows that will be used on the CCD.</dd>
 * <dt>NSBin</dt> <dd>The amount of binning of columns on the CCD.</dd>
 * <dt>NPBin</dt> <dd>The amount of binning of rows on the CCD.</dd>
 * <dt>DeInterlace_Type</dt> <dd>The type of deinterlacing the image will require. This depends on the way the
 * 	SDSU CCD Controller reads out the CCD. Acceptable values in 
 * 	<a href="ccd_dsp.html#CCD_DSP_DEINTERLACE_TYPE">CCD_DSP_DEINTERLACE_TYPE</a> are:
 *	CCD_DSP_DEINTERLACE_SINGLE,
 *	CCD_DSP_DEINTERLACE_SPLIT_PARALLEL,
 * 	CCD_DSP_DEINTERLACE_SPLIT_SERIAL or
 * 	CCD_DSP_DEINTERLACE_SPLIT_QUAD.</dd>
 * <dt>Window_Flags</dt> <dd>The window flags for this setup. Determines which of the four possible windows
 * 	are in use for this setup.</dd>
 * <dt>Window_List</dt> <dd>A list of window positions on the CCD. Theere are a maximum of CCD_SETUP_WINDOW_COUNT
 * 	windows. The windows should not overlap in either dimension.</dd>
 * <dt>Filter_Wheel_Position_List</dt> <dd>A list of positions the filter wheel has attained. There are
 * 	SETUP_FILTER_WHEEL_COUNT filter wheels.</dd>
 * <dt>Power_Complete</dt> <dd>A boolean value indicating whether the power cycling operation was completed
 * 	successfully.</dd>
 * <dt>Timing_Complete</dt> <dd>A boolean value indicating whether the timing program was completed
 * 	successfully.</dd>
 * <dt>Utility_Complete</dt> <dd>A boolean value indicating whether the utility program was completed
 * 	successfully.</dd>
 * <dt>Dimension_Complete</dt> <dd>A boolean value indicating whether the dimension setup was completed
 * 	successfully.</dd>
 * <dt>Setup_In_Progress</dt> <dd>A boolean value indicating whether the setup operation is in progress.</dd>
 * </dl>
 */
struct Setup_Struct
{
	int NCols;
	int NRows;
	int NSBin;
	int NPBin;
	enum CCD_DSP_DEINTERLACE_TYPE DeInterlace_Type;
	int Window_Flags;
	struct CCD_Setup_Window_Struct Window_List[CCD_SETUP_WINDOW_COUNT];
	int Filter_Wheel_Position_List[SETUP_FILTER_WHEEL_POSITION_COUNT];
	int Power_Complete;
	int Timing_Complete;
	int Utility_Complete;
	int Dimension_Complete;
	int Setup_In_Progress;
};

/* external variables */

/* local variables */
/**
 * Variable holding error code of last operation performed by ccd_dsp.
 */
static int Setup_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 */
static char Setup_Error_String[CCD_GLOBAL_ERROR_STRING_LENGTH] = "";
/**
 * Data holding the current status of ccd_setup.
 */
static struct Setup_Struct Setup_Data;

/* local function definitions */
static int Setup_Reset_Controller(void);
static int Setup_Hardware_Test(void);
static int Setup_Timing_Board(enum CCD_SETUP_LOAD_TYPE load_type,int load_application_number,char *filename);
static int Setup_Utility_Board(enum CCD_SETUP_LOAD_TYPE load_type,int load_application_number,char *filename);
static int Setup_Power_On(void);
static int Setup_Power_Off(void);
static int Setup_Gain(enum CCD_DSP_GAIN gain,int speed);
static int Setup_Idle(int idle);
static int Setup_Binning(void);
static int Setup_DeInterlace(void);
static int Setup_Dimensions(void);

/* external functions */
/**
 * This routine sets up ccd_setup internal variables.
 * It should be called at startup.
 */
void CCD_Setup_Initialise(void)
{
	int i;

	Setup_Error_Number = 0;
	Setup_Data.NCols = 0;
	Setup_Data.NRows = 0;
	Setup_Data.NSBin = 0;
	Setup_Data.NPBin = 0;
	Setup_Data.DeInterlace_Type = CCD_DSP_DEINTERLACE_SINGLE;
	Setup_Data.Window_Flags = 0;
	for(i=0;i<CCD_SETUP_WINDOW_COUNT;i++)
	{
		Setup_Data.Window_List[i].X_Start = -1;
		Setup_Data.Window_List[i].Y_Start = -1;
		Setup_Data.Window_List[i].X_End = -1;
		Setup_Data.Window_List[i].Y_End = -1;
	}
	for(i=0;i<SETUP_FILTER_WHEEL_POSITION_COUNT;i++)
		Setup_Data.Filter_Wheel_Position_List[i] = -1;
	Setup_Data.Power_Complete = FALSE;
	Setup_Data.Timing_Complete = FALSE;
	Setup_Data.Utility_Complete = FALSE;
	Setup_Data.Dimension_Complete = FALSE;
}

/**
 * The routine that sets up the SDSU CCD Controller. This routine does the following:
 * <ul>
 * <li>Resets setup completion flags.</li>
 * <li>Resets the SDSU CCD Controller.</li>
 * <li>Does a hardware test on the data links to each board in the controller.</li>
 * <li>Loads a timing board application from ROM/file.</li>
 * <li>Loads a utility board application from ROM/file.</li>
 * <li>Switches the boards analogue power on.</li>
 * <li>Sets the arrays target temperature.</li>
 * <li>Setup the array's gain and readout speed.</li>
 * <li>Setup the readout clocks to idle(or not!).</li>
 * </ul>
 * Array dimension information also needs to be setup before the controller can take exposures.
 * This routine can be aborted with CCD_Setup_Abort.
 * @param timing_load_type Where the routine is going to load the timing board application from. One of
 * 	<a href="#CCD_SETUP_LOAD_TYPE">CCD_SETUP_LOAD_TYPE</a>:
 * 	CCD_SETUP_LOAD_APPLICATION or
 * 	CCD_SETUP_LOAD_FILENAME.
 * @param timing_application_number The application number of the DSP code on EEPROM that will be loaded if the 
 * 	timing_load_type is CCD_SETUP_LOAD_APPLICATION.
 * @param timing_filename The filename of the DSP code on disc that will be loaded if the
 * 	timing_load_type is CCD_SETUP_LOAD_FILENAME.
 * @param utility_load_type Where the routine is going to load the utility board application from. One of
 * 	<a href="#CCD_SETUP_LOAD_TYPE">CCD_SETUP_LOAD_TYPE</a>:
 * 	CCD_SETUP_LOAD_APPLICATION or
 * 	CCD_SETUP_LOAD_FILENAME.
 * @param utility_application_number The application number of the DSP code on EEPROM that will be loaded if the 
 * 	utility_load_type is CCD_SETUP_LOAD_APPLICATION.
 * @param utility_filename The filename of the DSP code on disc that will be loaded if the
 * 	utility_load_type is CCD_SETUP_LOAD_FILENAME.
 * @param target_temperature Specifies the target temperature the CCD is meant to run at. 
 * @param gain Specifies the gain to use for the CCD video processors. Acceptable values are
 * 	<a href="ccd_dsp.html#CCD_DSP_GAIN">CCD_DSP_GAIN</a>:
 * 	CCD_DSP_GAIN_ONE, CCD_DSP_GAIN_TWO,
 * 	CCD_DSP_GAIN_FOUR and CCD_DSP_GAIN_NINE.
 * @param gain_speed Set to true for fast integrator speed, false for slow integrator speed.
 * @param idle If true puts CCD clocks in readout sequence, but not transferring any data, whenever a
 * 	command is not executing.
 * @return Returns TRUE if the setup is successfully completed, FALSE if the setup fails or is aborted.
 * @see #CCD_Setup_Dimensions
 * @see #CCD_Setup_Abort
 */
int CCD_Setup_Startup(enum CCD_SETUP_LOAD_TYPE timing_load_type,int timing_application_number,char *timing_filename,
	enum CCD_SETUP_LOAD_TYPE utility_load_type,int utility_application_number,char *utility_filename,
	double target_temperature,enum CCD_DSP_GAIN gain,int gain_speed,int idle)
{
	Setup_Error_Number = 0;
/* we are in a setup routine */
	Setup_Data.Setup_In_Progress = TRUE;
/* reset abort flag - we havn't aborted yet! */
	CCD_DSP_Set_Abort(FALSE);
/* reset completion flags - even dimension flag is reset, as the controller itself is reset */
	Setup_Data.Power_Complete = FALSE;
	Setup_Data.Timing_Complete = FALSE;
	Setup_Data.Utility_Complete = FALSE;
	Setup_Data.Dimension_Complete = FALSE;
/* diddly - magical Voodoo.java write memory
** WRM PCI X:1 = 1.
** Is this needed?
*/
/* Inform the PCI board that replies are ready to be recieved. */
	if(CCD_DSP_Command_WRM(CCD_DSP_INTERFACE_BOARD_ID,CCD_DSP_MEM_SPACE_X,1,1)!=CCD_DSP_DON)
	{
		Setup_Data.Setup_In_Progress = FALSE;
		CCD_DSP_Set_Abort(FALSE);
		Setup_Error_Number = 1;
		sprintf(Setup_Error_String,"CCD_Setup_Startup:Replies ready to be received failed.");
		return FALSE;
	}
/* reset controller */
	if(!Setup_Reset_Controller())
	{
		Setup_Data.Setup_In_Progress = FALSE;
		CCD_DSP_Set_Abort(FALSE);
		return(FALSE);
	}
/* do a hardware test (data link) */
	if(!Setup_Hardware_Test())
	{
		Setup_Data.Setup_In_Progress = FALSE;
		CCD_DSP_Set_Abort(FALSE);
		return FALSE;
	}
/* load a timing board application from ROM or a filename */
	if(!Setup_Timing_Board(timing_load_type,timing_application_number,timing_filename))
	{
		Setup_Data.Setup_In_Progress = FALSE;
		CCD_DSP_Set_Abort(FALSE);
		return FALSE;
	}
	else   /*acknowlege timing load complete*/
		Setup_Data.Timing_Complete = TRUE;
/* load a utility board application from ROM or a filename */
	if(!Setup_Utility_Board(utility_load_type,utility_application_number,utility_filename))
	{
		Setup_Data.Setup_In_Progress = FALSE;
		CCD_DSP_Set_Abort(FALSE);
		return FALSE;
	}
	else /*acknowlege utility load complete*/ 
		Setup_Data.Utility_Complete = TRUE;
/* turn analogue power on */
	if(!Setup_Power_On())
	{
		Setup_Data.Setup_In_Progress = FALSE;
		CCD_DSP_Set_Abort(FALSE);
		return FALSE;
	}
	else /*acknowlege power on complete*/ 
		Setup_Data.Power_Complete = TRUE;
/* set the temperature */
	if(!CCD_Temperature_Set(target_temperature))
	{
		Setup_Data.Setup_In_Progress = FALSE;
		CCD_DSP_Set_Abort(FALSE);
		Setup_Error_Number = 2;
		sprintf(Setup_Error_String,"CCD_Setup_Startup:CCD_Temperature_Set failed(%.2f)",
			target_temperature);
		return FALSE;
	}
/* setup gain */
	if(!Setup_Gain(gain,gain_speed))
	{
		Setup_Data.Setup_In_Progress = FALSE;
		CCD_DSP_Set_Abort(FALSE);
		return FALSE;
	}
/* setup idling of the readout clocks */
	if(!Setup_Idle(idle))
	{
		Setup_Data.Setup_In_Progress = FALSE;
		CCD_DSP_Set_Abort(FALSE);
		return FALSE;
	}
/* tidy up flags and return */
	Setup_Data.Setup_In_Progress = FALSE;
	CCD_DSP_Set_Abort(FALSE);
	return TRUE;
}

/**
 * Routine to shut down the SDSU CCD Controller board. This consists of:
 * <ul>
 * <li>Reseting the setup completion flags.
 * <li>Performing a power off command to switch off analogue voltages.
 * </ul>
 * It then just remains to close the connection to the astro device driver.
 * @see #CCD_Setup_Startup
 */
int CCD_Setup_Shutdown(void)
{
	int i;

	Setup_Error_Number = 0;
/* reset abort flag */
	CCD_DSP_Set_Abort(FALSE);
/* reset completion flags  */
	Setup_Data.NCols = 0;
	Setup_Data.NRows = 0;
	Setup_Data.NSBin = 0;
	Setup_Data.NPBin = 0;
	Setup_Data.DeInterlace_Type = CCD_DSP_DEINTERLACE_SINGLE;
	Setup_Data.Window_Flags = 0;
	for(i=0;i<CCD_SETUP_WINDOW_COUNT;i++)
	{
		Setup_Data.Window_List[i].X_Start = -1;
		Setup_Data.Window_List[i].Y_Start = -1;
		Setup_Data.Window_List[i].X_End = -1;
		Setup_Data.Window_List[i].Y_End = -1;
	}
	for(i=0;i<SETUP_FILTER_WHEEL_POSITION_COUNT;i++)
		Setup_Data.Filter_Wheel_Position_List[i] = -1;
	Setup_Data.Power_Complete = FALSE;
	Setup_Data.Timing_Complete = FALSE;
	Setup_Data.Utility_Complete = FALSE;
	Setup_Data.Dimension_Complete = FALSE;
/* perform a power off */
	if(!Setup_Power_Off())
	{
		CCD_DSP_Set_Abort(FALSE);
		return FALSE;
	}
	return TRUE;
}

/**
 * Routine to setup dimension information in the controller. This needs to be setup before an exposure
 * can take place. This routine must be called <b>after</b> the CCD_Setup_Startup routine.
 * This routine can be aborted with CCD_Setup_Abort.
 * @param ncols The number of columns in the image.
 * @param nrows The number of rows in the image to be readout from the CCD.
 * @param nsbin The amount of binning applied to pixels in columns. This parameter will change internally
 *	ncols.
 * @param npbin The amount of binning applied to pixels in rows.This parameter will change internally
 *	nrows.
 * @param deinterlace_type The algorithm to use for deinterlacing the resulting data. The data needs to be
 * 	deinterlaced if the CCD is read out from multiple readouts. One of
 * 	<a href="ccd_dsp.html#CCD_DSP_DEINTERLACE_TYPE">CCD_DSP_DEINTERLACE_TYPE</a>:
 * 	CCD_DSP_DEINTERLACE_SINGLE,
 * 	CCD_DSP_DEINTERLACE_SPLIT_PARALLEL,
 * 	CCD_DSP_DEINTERLACE_SPLIT_SERIAL,
 * 	CCD_DSP_DEINTERLACE_SPLIT_QUAD.
 * @param window_flags Information on which of the sets of window positions supplied contain windows to be used.
 * @param window_list A list of CCD_Setup_Window_Structs defining the position of the windows. The list should
 * 	<b>always</b> contain <b>four</b> entries, one for each possible window. The window_flags parameter
 * 	determines which items in the list are used.
 * @see #CCD_Setup_Startup
 * @see #CCD_Setup_Abort
 * @see #CCD_Setup_Window_Struct
 */
int CCD_Setup_Dimensions(int ncols,int nrows,int nsbin,int npbin,
	enum CCD_DSP_DEINTERLACE_TYPE deinterlace_type,int window_flags,
	struct CCD_Setup_Window_Struct window_list[])
{
	Setup_Error_Number = 0;
/* we are in a setup routine */
	Setup_Data.Setup_In_Progress = TRUE;
/* reset abort flag - we havn't aborted yet! */
	CCD_DSP_Set_Abort(FALSE);
/* reset dimension flag */
	Setup_Data.Dimension_Complete = FALSE;

/* The binning needs to be done first to set the final
** image dimensions.  Then Setup_DeInterlace is called
** to ensure that the dimensions agree with the deinterlace
** setting.  When all is said and done, the final dimensions
** are written to the boards.
*/
/* first do the binning */
	if(nrows <= 0)
	{
		Setup_Data.Setup_In_Progress = FALSE;
		Setup_Error_Number = 24;
		sprintf(Setup_Error_String,"CCD_Setup_Dimensions:Illegal value:Number of Rows '%d'",
			nrows);
		return FALSE;
	}
	Setup_Data.NRows = nrows;
	if(ncols <= 0)
	{
		Setup_Data.Setup_In_Progress = FALSE;
		Setup_Error_Number = 25;
		sprintf(Setup_Error_String,"CCD_Setup_Dimensions:Illegal value:Number of Columns '%d'",
			ncols);
		return FALSE;
	}
	Setup_Data.NCols = ncols;
	if(nsbin <= 0)
	{
		Setup_Data.Setup_In_Progress = FALSE;
		Setup_Error_Number = 26;
		sprintf(Setup_Error_String,"CCD_Setup_Dimensions:Illegal value:Horizontal Binning '%d'",
			nsbin);
		return FALSE;
	}
	Setup_Data.NSBin = nsbin;
	if(npbin <= 0)
	{
		Setup_Data.Setup_In_Progress = FALSE;
		Setup_Error_Number = 27;
		sprintf(Setup_Error_String,"CCD_Setup_Dimensions:Illegal value:Vertical Binning '%d'",
			npbin);
		return FALSE;
	}
	Setup_Data.NPBin = npbin;

	if(!Setup_Binning())
	{
		/* Setup_Binning only sends back 1 of 2 results */
		Setup_Data.Setup_In_Progress = FALSE;
		CCD_DSP_Set_Abort(FALSE);
		return FALSE; 
	}

/* do de-interlacing */
	if(!CCD_DSP_IS_DEINTERLACE_TYPE(deinterlace_type))
	{
		Setup_Data.Setup_In_Progress = FALSE;
		Setup_Error_Number = 28;
		sprintf(Setup_Error_String,"CCD_Setup_Dimensions:Illegal value:"
			"DeInterlace Type '%d'",deinterlace_type);
		return FALSE;
	}
	Setup_Data.DeInterlace_Type = deinterlace_type;
	if(!Setup_DeInterlace())
	{
		Setup_Data.Setup_In_Progress = FALSE;
		CCD_DSP_Set_Abort(FALSE);
		return FALSE;
	}

/* setup final calculated dimensions */
	if(!Setup_Dimensions())
	{
		Setup_Data.Setup_In_Progress = FALSE;
		CCD_DSP_Set_Abort(FALSE);
		return FALSE;
	}
	else /*acknowlege dimensions complete*/ 
		Setup_Data.Dimension_Complete = TRUE;
/* setup windowing data */
/* diddly 
** check windowing arguments - must be non-overlapping in both dimensions - 
** only check window dimensions with the window_flags bit set */
	if(window_flags&CCD_SETUP_WINDOW_ONE)
		Setup_Data.Window_List[0] = window_list[0];
	if(window_flags&CCD_SETUP_WINDOW_TWO)
		Setup_Data.Window_List[1] = window_list[1];
	if(window_flags&CCD_SETUP_WINDOW_THREE)
		Setup_Data.Window_List[2] = window_list[2];
	if(window_flags&CCD_SETUP_WINDOW_FOUR)
		Setup_Data.Window_List[3] = window_list[3];

	Setup_Data.Window_Flags = window_flags;
/* reset in progress information */
	Setup_Data.Setup_In_Progress = FALSE;
	CCD_DSP_Set_Abort(FALSE);
	return TRUE;
}

/**
 * Routine to setup the filter wheel at the required positions.
 * This routine can be aborted with CCD_Setup_Abort.
 * @param position_one The position the first filter wheel has to attain.
 * @param position_two The position the second filter wheel has to attain.
 * @return Returns TRUE if the configuration has succeeded, FALSE if it fails.
 * @see #CCD_Setup_Abort
 */
int CCD_Setup_Filter_Wheel(int position_one,int position_two)
{
	Setup_Error_Number = 0;
/* we are in a setup routine */
	Setup_Data.Setup_In_Progress = TRUE;
/* reset abort flag - we havn't aborted yet! */
	CCD_DSP_Set_Abort(FALSE);
/* set data */
/* diddly
** calls CCD_DSP routine to move filter wheel to new position, possibly based on old position */
	Setup_Data.Filter_Wheel_Position_List[0] = position_one;
	Setup_Data.Filter_Wheel_Position_List[1] = position_two;
/* reset in progress information */
	Setup_Data.Setup_In_Progress = FALSE;
	CCD_DSP_Set_Abort(FALSE);
	return TRUE;
}

/**
 * Routine to abort a setup that is underway. This will cause CCD_Setup_Startup and CCD_Setup_Dimensions
 * to return FALSE as it will fail to complete the setup.
 * @see #CCD_Setup_Startup
 * @see #CCD_Setup_Dimensions
 * @see #CCD_Setup_Filter_Wheel
 */
void CCD_Setup_Abort(void)
{
	CCD_DSP_Set_Abort(TRUE);
}

/**
 * Routine that returns the number of columns setup has set the SDSU CCD Controller to readout. This is the
 * number passed into CCD_Setup_Dimensions, however, binning will have
 * reduced the value (ncols = ncols passed in / nsbin), and some deinterlacing options require an even
 * number of columns.
 * @return The number of columns.
 * @see #CCD_Setup_Dimensions
 */
int CCD_Setup_Get_NCols(void)
{
	return Setup_Data.NCols;
}

/**
 * Routine that returns the number of rows setup has set the SDSU CCD Controller to readout. This is the
 * number passed into CCD_Setup_Dimensions, however, binning will have
 * reduced the value (nrows = nrows passed in / npbin), and some deinterlacing options require an even
 * number of rows.
 * @return The number of rows.
 * @see #CCD_Setup_Dimensions
 */
int CCD_Setup_Get_NRows(void)
{
	return Setup_Data.NRows;
}

/**
 * Routine to return the current setting of the deinterlace type, used to unjumble data received from the CCD
 * when the CCD is being read out from multiple ports.
 * @return The current deinterlace type, one of
 * <a href="ccd_dsp.html#CCD_DSP_DEINTERLACE_TYPE">CCD_DSP_DEINTERLACE_TYPE</a>:
 * 	CCD_DSP_DEINTERLACE_SINGLE,
 * 	CCD_DSP_DEINTERLACE_SPLIT_PARALLEL,
 * 	CCD_DSP_DEINTERLACE_SPLIT_SERIAL,
 * 	CCD_DSP_DEINTERLACE_SPLIT_QUAD.
 */
enum CCD_DSP_DEINTERLACE_TYPE CCD_Setup_Get_DeInterlace_Type(void)
{
	return Setup_Data.DeInterlace_Type;
}

/**
 * Routine to return whether CCD_Setup_Startup and CCD_Setup_Dimensions completed successfully,
 * and the controller is in a state suitable to do an exposure. This is determined by examining the
 * Completion flags in Setup_Data.
 * @return Returns TRUE if setup was completed, FALSE otherwise.
 * @see #Setup_Data
 * @see #CCD_Setup_Startup
 * @see #CCD_Setup_Dimensions
 */
int CCD_Setup_Get_Setup_Complete(void)
{
	return (Setup_Data.Power_Complete&&Setup_Data.Timing_Complete&&Setup_Data.Utility_Complete&&
		Setup_Data.Dimension_Complete);
}

/**
 * Routine to return whether a call to CCD_Setup_Startup or CCD_Setup_Dimensions is in progress. This is done
 * by examining Setup_In_Progress in Setup_Data.
 * @return Returns TRUE if the SDSU CCD Controller is in the process of being setup, FALSE otherwise.
 * @see #Setup_Data
 * @see #CCD_Setup_Startup
 * @see #CCD_Setup_Dimensions
 */
int CCD_Setup_Get_Setup_In_Progress(void)
{
	return Setup_Data.Setup_In_Progress;
}

/**
 * Get the current value of ccd_setup's error number.
 * @return The current value of ccd_setup's error number.
 */
int CCD_Setup_Get_Error_Number(void)
{
	return Setup_Error_Number;
}

/**
 * The error routine that reports any errors occuring in ccd_setup in a standard way.
 */
void CCD_Setup_Error(void)
{
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Setup_Error_Number == 0)
		sprintf(Setup_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"CCD_Setup:Error(%d) : %s\n",Setup_Error_Number,Setup_Error_String);
	Setup_Error_Number = 0;
}

/**
 * The error routine that reports any errors occuring in ccd_setup in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 */
void CCD_Setup_Error_String(char *error_string)
{
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Setup_Error_Number == 0)
		sprintf(Setup_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"CCD_Setup:Error(%d) : %s\n",Setup_Error_Number,Setup_Error_String);
	Setup_Error_Number = 0;
}

/**
 * The warning routine that reports any warnings occuring in ccd_setup in a standard way.
 */
void CCD_Setup_Warning(void)
{
	/* if the error number is zero an warning message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an warning to display */
	if(Setup_Error_Number == 0)
		sprintf(Setup_Error_String,"Logic Error:No Warning defined");
	fprintf(stderr,"CCD_Setup:Warning(%d) : %s\n",Setup_Error_Number,Setup_Error_String);
	Setup_Error_Number = 0;
}

/* ------------------------------------------------------------------
**	Internal Functions
** ------------------------------------------------------------------ */
/**
 * Routine to reset the SDSU controller. A CCD_DSP_Command_Reset command is issued, which returns
 * <a href="ccd_dsp.html#CCD_DSP_SYR">SYR</a> on success. This is non-standard.
 * @return Returns TRUE if reset timing was successfully completed, 
 * 	FALSE if it failed in some way.
 * @see ccd_dsp.html#CCD_DSP_Command_Reset
 */
static int Setup_Reset_Controller(void)
{
	if(CCD_DSP_Command_Reset() != CCD_DSP_SYR)
	{
		Setup_Error_Number = 33;
		sprintf(Setup_Error_String,"Reset Controller Failed.");
		return FALSE;
	}
	return TRUE;
}

/**
 * Internal routine that performs a hardware test on the timing and utility boards. It does this by doing 
 * sending TDL commands to the boards and testing the results. Currently
 * only a short hardware test is implemented, where each board is tested. This routine is called from
 * CCD_Setup_Startup.
 * The test is performed <a href="#SETUP_SHORT_TEST_COUNT">SETUP_SHORT_TEST_COUNT</a> times.
 * @return If all the TDL commands fail to one of the boards it returns FALSE, otherwise
 *	it returns TRUE. If some commands fail a warning is given.
 * @see ccd_dsp.html#CCD_DSP_Command_TDL
 * @see #CCD_Setup_Startup
 */
static int Setup_Hardware_Test(void)
/* this function implements a SHORT hardware test only */
{
	int i;				/* loop number */
	int value;			/* value sent to tdl */
	int value_increment;		/* amount to increment value for each pass through the loop */
	int retval;			/* return value from dsp_command */
	int tim_errno,util_errno;	/* num of test encountered, per board */

	value_increment = TDL_MAX_VALUE/SETUP_SHORT_TEST_COUNT;

	/* test the timimg board SETUP_SHORT_TEST_COUNT times */
	tim_errno = 0;
	value = 0;
	for(i=1; i<=SETUP_SHORT_TEST_COUNT; i++)
	{
		retval = CCD_DSP_Command_TDL(CCD_DSP_TIM_BOARD_ID,value);
		if(retval != value)
			tim_errno++;
		value += value_increment;
	}

	/* test the utility board SETUP_SHORT_TEST_COUNT times */
	util_errno = 0;
	value = 0;	
	for(i=1; i<=SETUP_SHORT_TEST_COUNT; i++)
	{
		retval = CCD_DSP_Command_TDL(CCD_DSP_UTIL_BOARD_ID,value);
		if(retval != value)
			util_errno++;
		value += value_increment;
	}
	/* if some timing errors occured, setup an error message and determine whether it was fatal or not */
	if(tim_errno > 0)
	{
		Setup_Error_Number = 4;
		sprintf(Setup_Error_String,"Timing Board Hardware Test:Failed %d of %d times",
			tim_errno,SETUP_SHORT_TEST_COUNT);
		if(tim_errno < SETUP_SHORT_TEST_COUNT)
			CCD_Setup_Warning();
		else
			return FALSE;
	}
	/* if some utility errors occured, setup an error message and determine whether it was fatal or not */
	if(util_errno > 0)
	{
		Setup_Error_Number = 5;
		sprintf(Setup_Error_String,"Utility Board Hardware Test:Failed %d of %d times",
			util_errno,SETUP_SHORT_TEST_COUNT);
		if(util_errno < SETUP_SHORT_TEST_COUNT)
			CCD_Setup_Warning();
		else
			return FALSE;
	}
	return TRUE;
}

/**
 * Internal routine that loads a timing board DSP application onto the SDSU CCD Controller. The
 * application can come from either (EEP)ROM or a file on disc. This routine is called from
 * CCD_Setup_Startup. If the timing board is in idle mode and load_type is
 * CCD_SETUP_LOAD_FILENAME the board is stopped whilst the application is loaded.
 * @return The routine returns TRUE if the operation succeeded, or FALSE if it fails.
 * @param load_type Where to load the application from, one of<a href="#CCD_SETUP_LOAD_TYPE">CCD_SETUP_LOAD_TYPE</a>:
 *	CCD_SETUP_LOAD_APPLICATION (load from (EEP)ROM) or
 *	CCD_SETUP_LOAD_FILENAME (load from a disc file).
 * @param load_application_number If load_type is CCD_SETUP_LOAD_APPLICATION,
 * 	this specifies which application number to load.
 * @param filename If load_type is CCD_SETUP_LOAD_FILENAME,
 * 	this specifies a filename to load the application from.
 * @see ccd_dsp.html#CCD_DSP_Command_LDA
 * @see ccd_dsp.html#CCD_DSP_Command_IDL
 * @see ccd_dsp.html#CCD_DSP_Command_STP
 * @see ccd_dsp.html#CCD_DSP_Download
 * @see #CCD_Setup_Startup
 */
static int Setup_Timing_Board(enum CCD_SETUP_LOAD_TYPE load_type,int load_application_number,char *filename)
{
	int value,bit_value;

	if(!CCD_SETUP_IS_LOAD_TYPE(load_type))
	{
		Setup_Error_Number = 29;
		sprintf(Setup_Error_String,"Setup_Timing_Board:Timing board has illegal load type(%d)",
			load_type);
		return FALSE;
	}
	if(load_type == CCD_SETUP_LOAD_APPLICATION)
	{
		if(CCD_DSP_Command_LDA(CCD_DSP_TIM_BOARD_ID,load_application_number)!=CCD_DSP_DON)
		{
			Setup_Error_Number = 6;
			sprintf(Setup_Error_String,"Timing Board:Failed to load application %d",
				load_application_number);
			return FALSE;
		}
	}
	else if(load_type == CCD_SETUP_LOAD_FILENAME)
	{
	/* diddly It is not clear from the documentation whether stopping Idling is
	** still necessary */
		/* if we are currently in IDL mode - come out of this mode */
		value = CCD_DSP_Command_RDM(CCD_DSP_TIM_BOARD_ID,CCD_DSP_MEM_SPACE_X,0);
		bit_value = value & 0x01;
		if(bit_value)
		{
			if(CCD_DSP_Command_STP()!= CCD_DSP_DON)
			{
				Setup_Error_Number = 7;
				sprintf(Setup_Error_String,"Timing Board:Failed to load filename '%s':STP failed",
					filename);
				return FALSE;
			}
		}
		/* download the program from the file */
		if(!CCD_DSP_Download(CCD_DSP_TIM_BOARD_ID,filename))
		{
			Setup_Error_Number = 8;
			sprintf(Setup_Error_String,"Timing Board:Failed to download filename '%s'",
				filename);
			return FALSE;
		}
		/* if neccessary restart IDL mode */
		if(bit_value)
		{
			if(CCD_DSP_Command_IDL()!=CCD_DSP_DON)
			{
				Setup_Error_Number = 9;
				sprintf(Setup_Error_String,"Timing Board:Failed to load filename '%s':IDL failed",
					filename);
				return FALSE;
			}
		}
	}
	return TRUE;
}

/**
 * Internal routine that loads a utility board DSP application onto the SDSU CCD Controller. The
 * application can come from either (EEP)ROM or a file on disc. This routine is called from
 * CCD_Setup_Startup.
 * @return The routine returns TRUE if the operation succeeded, or FALSE if it fails.
 * @param load_type Where to load the application from, one of<a href="#CCD_SETUP_LOAD_TYPE">CCD_SETUP_LOAD_TYPE</a>:
 *	CCD_SETUP_LOAD_APPLICATION (load from (EEP)ROM) or
 *	CCD_SETUP_LOAD_FILENAME (load from a disc file).
 * @param load_application_number If load_type is CCD_SETUP_LOAD_APPLICATION,
 * 	this specifies which application number to load.
 * @param filename If load_type is CCD_SETUP_LOAD_FILENAME,
 * 	this specifies a filename to load the application from.
 * @see ccd_dsp.html#CCD_DSP_Command_LDA
 * @see ccd_dsp.html#CCD_DSP_Download
 * @see #CCD_Setup_Startup
 */
static int Setup_Utility_Board(enum CCD_SETUP_LOAD_TYPE load_type,int load_application_number,char *filename)
{
	if(!CCD_SETUP_IS_LOAD_TYPE(load_type))
	{
		Setup_Error_Number = 30;
		sprintf(Setup_Error_String,"Setup_Utility_Board:Utility board has illegal load type(%d)",
			load_type);
		return FALSE;
	}
	if(load_type == CCD_SETUP_LOAD_APPLICATION)
	{
		if(CCD_DSP_Command_LDA(CCD_DSP_UTIL_BOARD_ID,load_application_number)!=CCD_DSP_DON)
		{
			Setup_Error_Number = 10;
			sprintf(Setup_Error_String,"Utility Board:Failed to load application %d",
				load_application_number);
			return FALSE;
		}
	}
	else if(load_type == CCD_SETUP_LOAD_FILENAME)
	{
		if(!CCD_DSP_Download(CCD_DSP_UTIL_BOARD_ID,filename))
		{
			Setup_Error_Number = 11;
			sprintf(Setup_Error_String,"Utility Board:Failed to download filename '%s'",
				filename);
			return FALSE;
		}
	}
	return TRUE;
}

/**
 * Internal ccd_setup routine that turns on the analog power to the SDSU CCD Controller using the DSP
 * PON command. The command is sent to the utility board using the
 * <a href="ccd_dsp.html#CCD_DSP_Command_PON">CCD_DSP_Command_PON</a> routine.This routine is called from
 * CCD_Setup_Startup.
 * @return Returns TRUE if the operation succeeded, FALSE if it failed.
 * @see #CCD_Setup_Startup
 * @see ccd_dsp.html#CCD_DSP_Command_PON
 */
static int Setup_Power_On(void)
{
	if(CCD_DSP_Command_PON()!=CCD_DSP_DON)
	{
		Setup_Error_Number = 12;
		sprintf(Setup_Error_String,"Power On failed");
		return FALSE;
	}
	return TRUE;
}

/**
 * Internal ccd_setup routine that turns off the analog power to the SDSU CCD Controller using the DSP
 * POF command. The command is sent to the utility board using the
 * <a href="ccd_dsp.html#CCD_DSP_Command_POF">CCD_DSP_Command_POF</a> routine.This routine is called from
 * CCD_Setup_Shutdown.
 * @return Returns TRUE if the operation succeeded, FALSE if it failed.
 * @see #CCD_Setup_Shutdown
 * @see ccd_dsp.html#CCD_DSP_Command_POF
 */
static int Setup_Power_Off(void)
{
	if(CCD_DSP_Command_POF()!=CCD_DSP_DON)
	{
		Setup_Error_Number = 3;
		sprintf(Setup_Error_String,"Power Off failed");
		return FALSE;
	}
	return TRUE;
}

/**
 * Internal routine to setup the gain and speed of the SDSU CCD Controller. This routine is called from
 * CCD_Setup_Startup.
 * @return returns TRUE if the operation succeeded, FALSE if it failed.
 * @param gain The gain to set the video controllers on the SDSU CCD Controller to. One of 
 * 	<a href="ccd_dsp.html#CCD_DSP_GAIN">CCD_DSP_GAIN</a>:
 * 	CCD_DSP_GAIN_ONE, CCD_DSP_GAIN_TWO,
 * 	CCD_DSP_GAIN_FOUR and CCD_DSP_GAIN_NINE.
 * @param speed The speed to set the video integrators to. If TRUE they are 'fast', if FALSE they are 'slow'.
 * @see #CCD_Setup_Startup
 * @see ccd_dsp.html#CCD_DSP_Command_SGN
 */
static int Setup_Gain(enum CCD_DSP_GAIN gain,int speed)
{
	int ret_val;

	if(!CCD_GLOBAL_IS_BOOLEAN(speed))
	{
		Setup_Error_Number = 31;
		sprintf(Setup_Error_String,"Setting Gain to %d(speed:%d) failed",gain,speed);
		return FALSE;
	}
	if(!CCD_DSP_IS_GAIN(gain))
	{
		Setup_Error_Number = 34;
		sprintf(Setup_Error_String,"Setting Gain to %d(speed:%d) failed",gain,speed);
		return FALSE;
	}
	ret_val = CCD_DSP_Command_SGN(gain,speed);
	if(ret_val!=CCD_DSP_DON)
	{
		Setup_Error_Number = 13;
		sprintf(Setup_Error_String,"Setting Gain to %d(speed:%d) failed",gain,speed);
	}
	return (ret_val==CCD_DSP_DON);
}

/**
 * Internal routine to set whether the SDSU CCD Controller timing board should
 * idle when not executing commands, using the IDL or STP DSP commands. 
 * This routine is called from CCD_Setup_Startup.
 * @param idle TRUE if the timing board is to idle when not executing commands, FALSE if it isn't to idle.
 * @return returns TRUE if the operation succeeded, FALSE if it failed.
 * @see #CCD_Setup_Startup
 * @see ccd_dsp.html#CCD_DSP_Command_IDL
 * @see ccd_dsp.html#CCD_DSP_Command_STP
 */
static int Setup_Idle(int idle)
{
	if(!CCD_GLOBAL_IS_BOOLEAN(idle))
	{
		Setup_Error_Number = 32;
		sprintf(Setup_Error_String,"Setting Idle failed:Illegal idle value '%d'",idle);
		return FALSE;
	}
	if(idle)
	{
		if(CCD_DSP_Command_IDL()!=CCD_DSP_DON)
		{
			Setup_Error_Number = 14;
			sprintf(Setup_Error_String,"Setting Idle failed");
			return FALSE;
		}
	}
	else
	{
		if(CCD_DSP_Command_STP()!=CCD_DSP_DON)
		{
			Setup_Error_Number = 15;
			sprintf(Setup_Error_String,"Setting Stop Idle failed");
			return FALSE;
		}
	}
	return TRUE;
}

/**
 * Internal routine to set up the binning configuration for the SDSU CCD Controller. This routines writes the
 * binning values to the controller boards, and re-calculates the stored columns and rows values to allow for
 * binning e.g. NCols = NCols/NSBin. This routine is called from CCD_Setup_Dimensions.
 * @return Returns TRUE if the operation succeeds, FALSE if it fails.
 * @see #CCD_Setup_Dimensions
 * @see ccd_dsp.html#CCD_DSP_Command_WRM
 */
static int Setup_Binning(void)
{
	/* will be sending the FINAL image size to the boards, so calculate them now */
	Setup_Data.NCols = Setup_Data.NCols/Setup_Data.NSBin;
	Setup_Data.NRows = Setup_Data.NRows/Setup_Data.NPBin;

/* diddly These locations should be set by the PCI memory writes, but I am not convinced they work */
	if(CCD_DSP_Command_WRM(CCD_DSP_TIM_BOARD_ID,CCD_DSP_MEM_SPACE_Y,0005,Setup_Data.NSBin)!=CCD_DSP_DON)
	{
		Setup_Error_Number = 16;
		sprintf(Setup_Error_String,"Setting Column Binning failed(%d)",Setup_Data.NSBin);
		return FALSE;
	}
	if(CCD_DSP_Command_WRM(CCD_DSP_TIM_BOARD_ID,CCD_DSP_MEM_SPACE_Y,0006,Setup_Data.NPBin)!= CCD_DSP_DON)
	{
		Setup_Error_Number = 17;
		sprintf(Setup_Error_String,"Setting Row Binning failed(%d)",Setup_Data.NPBin);
		return FALSE;
	}
	return TRUE;
}

/**
 * Internal routine to set up the deinterlace setting for the SDSU CCD Controller. 
 * This routine re-calculates the stored columns and rows values to allow for the deinterlace type. Some deinterlace
 * types require an even number of rows and/or columns. The routine prints a warning if the rows or columns are
 * changed. This routine is called from CCD_Setup_Dimensions.
 * @return Returns TRUE if the operation succeeds, FALSE if it fails.
 * @see #CCD_Setup_Dimensions
 */
static int Setup_DeInterlace(void)
{
	switch(Setup_Data.DeInterlace_Type)
	{
		case CCD_DSP_DEINTERLACE_SINGLE:		/* Single readout */
			break;
		case CCD_DSP_DEINTERLACE_SPLIT_PARALLEL:	/* Split Parallel readout */
			if((float)Setup_Data.NRows/2 != (int)Setup_Data.NRows/2)
			{
				Setup_Error_Number = 18;
				sprintf(Setup_Error_String,"DeInterlace:Split Parallel needs even rows"
					"(%d,%d)",Setup_Data.NRows,Setup_Data.NCols);
				CCD_Setup_Warning();
				Setup_Data.NRows--;
			}
			break;
		case CCD_DSP_DEINTERLACE_SPLIT_SERIAL:	/* Split Serial readout */
			if((float)Setup_Data.NCols/2 != (int)Setup_Data.NCols/2)
			{
				Setup_Error_Number = 19;
				sprintf(Setup_Error_String,"DeInterlace:Split Serial needs even columns"
					"(%d,%d)",Setup_Data.NRows,Setup_Data.NCols);
				CCD_Setup_Warning();
				Setup_Data.NCols--;
			}
			break;
		case CCD_DSP_DEINTERLACE_SPLIT_QUAD:	/* Split Quad */
			if(((float)Setup_Data.NCols/2 != (int)Setup_Data.NCols/2)||
	      			((float)Setup_Data.NRows/2 != (int)Setup_Data.NRows/2))
			{
				Setup_Error_Number = 20;
				sprintf(Setup_Error_String,"DeInterlace:Split Quad needs even columns and rows"
					"(%d,%d)",Setup_Data.NCols,Setup_Data.NRows);
				CCD_Setup_Warning();
				if((float)Setup_Data.NCols/2 != (int)Setup_Data.NCols/2)
					Setup_Data.NCols--;
				if((float)Setup_Data.NRows/2 != (int)Setup_Data.NRows/2)
					Setup_Data.NRows--;
			}
			break;
		default:
			Setup_Error_Number = 21;
			sprintf(Setup_Error_String,"Setting DeInterlace:Illegal Setting(%d)",
				Setup_Data.DeInterlace_Type);
			return FALSE;
	} /* end switch */
	return TRUE;
}

/**
 * Internal routine to set up the CCD dimensions for the SDSU CCD Controller. This routines writes the
 * dimension values to the controller boards using WRM.  This routine is called from CCD_Setup_Dimensions.
 * @return Returns TRUE if the operation succeeds, FALSE if it fails.
 * @see #CCD_Setup_Dimensions
 * @see ccd_dsp.html#CCD_DSP_Command_WRM
 * @see ccd_dsp.html#CCD_DSP_WRM
 */
static int Setup_Dimensions(void)
{
	if(CCD_DSP_Command_Set_NCols(Setup_Data.NCols)!=CCD_DSP_DON)
	{
		Setup_Error_Number = 22;
		sprintf(Setup_Error_String,"Setting Dimensions:Column Setup failed(%d)",Setup_Data.NCols);
		return FALSE;
	}
	if(CCD_DSP_Command_Set_NRows(Setup_Data.NRows)!=CCD_DSP_DON)
	{
		Setup_Error_Number = 23;
		sprintf(Setup_Error_String,"Setting Dimensions:Row Setup failed(%d)",Setup_Data.NRows);
		return FALSE;
	}
	return TRUE;
}

/*
** $Log: not supported by cvs2svn $
** Revision 0.2  2000/02/02 15:55:14  cjm
** Binning and windowing addded.
**
** Revision 0.1  2000/01/25 14:57:27  cjm
** initial revision (PCI version).
**
*/





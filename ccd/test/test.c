/* test.c
** $Header: /home/cjm/cvs/frodospec/ccd/test/test.c,v 1.14 2003-03-26 15:51:55 cjm Exp $
*/
#include <stdio.h>
#include <time.h>
#include "ccd_setup.h"
#include "ccd_exposure.h"
#include "ccd_dsp.h"
#include "ccd_interface.h"
#include "ccd_text.h"
#include "ccd_temperature.h"
#include "ccd_setup.h"
#include "fitsio.h"

/**
 * This program is a basic test of the SDSU CCD controller. It is invoked with 1 argument:
 * <pre>
 * test <device>: device is either [pci|text]
 * </pre>
 * It sets up the controller and performs a basic exposure.
 * Note the setup is performed by downloading two DSP .lod files, tim.lod and util.lod,
 * which must be present in the bin directory otherwise an error is returned.
 * @author $Author: cjm $
 * @version $Revision: 1.14 $
 */

/* hash defines */
/**
 * Filename for the timing board DSP code .lod file.
 */
#define TIMING_FILENAME "tim.lod"
/**
 * Filename for the utility board DSP code .lod file.
 */
#define UTILITY_FILENAME "util.lod"
/**
 * The size of the image array in the X direction.
 */
#define CCD_X_SIZE	2150
/**
 * The size of the image array in the Y direction.
 */
#define CCD_Y_SIZE	2048
/**
 * The amount of array binning in the X direction.
 */
#define CCD_XBIN_SIZE	1
/**
 * The amount of array binning in the Y direction.
 */
#define CCD_YBIN_SIZE	1

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id: test.c,v 1.14 2003-03-26 15:51:55 cjm Exp $";

/* internal functions */
static int Test_Save_Fits_Headers(int exposure_time,int ncols,int nrows,char *filename);
static void Test_Fits_Header_Error(int status);

/**
 * Main program.
 * <ul>
 * <li>Initialises the library.
 * <li>The interface is opened.
 * <li>The controller is setup.
 * <li>The current temperature is retrieved.
 * <li>The controller dimensions are setup.
 * <li>The filter wheels are reset.
 * <li>Some fits headers are saved to disc.
 * <li>An exposure is taken.
 * <li>The interface to the controller is closed.
 * </ul>
 * @param argc The number of command line parameters.
 * @param argv The list of parameter strings.
 * @return This program returns 0 on success, and a positive number for failure.
 * @see ccd_global.html#CCD_Global_Initialise
 * @see ccd_interface.html#CCD_Interface_Open
 * @see ccd_temperature.html#CCD_Temperature_Get
 * @see ccd_setup.html#CCD_Setup_Startup
 * @see ccd_setup.html#CCD_Setup_Dimensions
 * @see ccd_filter_wheel.html#CCD_Filter_Wheel_Reset
 * @see #Test_Save_Fits_Headers
 * @see ccd_exposure.html#CCD_Exposure_Expose
 * @see ccd_setup.html#CCD_Setup_Shutdown
 * @see ccd_interface.html#CCD_Interface_Close
 */
int main(int argc, char *argv[])
{
	struct CCD_Setup_Window_Struct window_list[CCD_SETUP_WINDOW_COUNT];
	char *filename_list[1];
	struct timespec start_time;
	int retval;
	char *exposure_data = NULL;
	double temperature = 0.0;

	if(argc != 2)
	{
		fprintf(stdout,"test <device>: device is either [pci|text].\n");
		exit(1);
	}

	fprintf(stdout,"Test ...\n");
	if(strcmp(argv[1],"pci")==0)
		CCD_Global_Initialise(CCD_INTERFACE_DEVICE_PCI);
	else if(strcmp(argv[1],"text")==0)
		CCD_Global_Initialise(CCD_INTERFACE_DEVICE_TEXT);
	else
	{
		fprintf(stdout,"test <device>: device is either [pci|text].\n");
		exit(2);
	}

	CCD_Text_Set_Print_Level(CCD_TEXT_PRINT_LEVEL_COMMANDS);

	fprintf(stdout,"Test:CCD_Interface_Open\n");
	if(!CCD_Interface_Open())
	{
		CCD_Global_Error();
		exit(3);
	}
	fprintf(stdout,"Test:CCD_Setup_Startup\n");
	if(!CCD_Setup_Startup(CCD_SETUP_LOAD_ROM,NULL,
		CCD_SETUP_LOAD_FILENAME,0,TIMING_FILENAME,
		CCD_SETUP_LOAD_FILENAME,1,UTILITY_FILENAME,-107.0,
		CCD_DSP_GAIN_FOUR,TRUE,TRUE))
	{
		CCD_Global_Error();
		exit(4);
	}
	fprintf(stdout,"Test:CCD_Setup_Startup completed\n");

	fprintf(stdout,"Test:CCD_Temperature_Get\n");
	if(!CCD_Temperature_Get(&temperature))
	{
		CCD_Global_Error();
		exit(5);
	}
	fprintf(stdout,"Test:CCD_Temperature_Get returned %.2f\n",temperature);

	fprintf(stdout,"Test:CCD_Setup_Dimensions\n");
	if(!CCD_Setup_Dimensions(CCD_X_SIZE,CCD_Y_SIZE,CCD_XBIN_SIZE,CCD_YBIN_SIZE,
		CCD_DSP_AMPLIFIER_LEFT,CCD_DSP_DEINTERLACE_SINGLE,0,window_list))
	{
		CCD_Global_Error();
		exit(6);
	}
	fprintf(stdout,"Test:CCD_Setup_Dimensions completed\n");

	fprintf(stdout,"Test:CCD_Setup_Filter_Wheel\n");
	if(!CCD_Filter_Wheel_Reset(0))
	{
		CCD_Global_Error();
		exit(7);
	}
	if(!CCD_Filter_Wheel_Reset(1))
	{
		CCD_Global_Error();
		exit(8);
	}
	fprintf(stdout,"Test:CCD_Setup_Filter_Wheel completed\n");

	if(!Test_Save_Fits_Headers(10000,CCD_X_SIZE/CCD_XBIN_SIZE,CCD_Y_SIZE/CCD_YBIN_SIZE,"test.fits"))
	{
		fprintf(stdout,"Test:Saving FITS headers failed.\n");
		exit(9);
	}
	fprintf(stdout,"Test:Saving FITS headers completed.\n");

	start_time.tv_sec = 0;
	start_time.tv_nsec = 0;
	filename_list[0] = "test.fits";
	if(!CCD_Exposure_Expose(TRUE,TRUE,start_time,10000,filename_list,1))
	{
		CCD_Global_Error();
		exit(10);
	}
	fprintf(stdout,"Test:CCD_Exposure_Expose finished\n");

	if(!CCD_Setup_Shutdown())
	{
		CCD_Global_Error();
		exit(11);
	}
	fprintf(stdout,"Test:CCD_Setup_Shutdown completed\n");

	if(!CCD_Interface_Close())
	{
		CCD_Global_Error();
		exit(12);
	}
	fprintf(stdout,"Test:CCD_Interface_Close\n");

	fprintf(stdout,"Test:Finished Test ...\n");
	return 0;
}

/**
 * Internal routine that saves some basic FITS headers to the relevant filename.
 * This is needed as newer CCD_Exposure_Expose routines need saved FITS headers to
 * not give an error.
 * @param exposure_time The amount of time, in milliseconds, of the exposure.
 * @param ncols The number of columns in the FITS file.
 * @param nrows The number of rows in the FITS file.
 * @param filename The filename to save the FITS headers in.
 */
static int Test_Save_Fits_Headers(int exposure_time,int ncols,int nrows,char *filename)
{
	static fitsfile *fits_fp = NULL;
	int status = 0,retval,ivalue;
	double dvalue;

/* open file */
	if(fits_create_file(&fits_fp,filename,&status))
	{
		Test_Fits_Header_Error(status);
		return FALSE;
	}
/* SIMPLE keyword */
	ivalue = TRUE;
	retval = fits_update_key(fits_fp,TLOGICAL,(char*)"SIMPLE",&ivalue,NULL,&status);
	if(retval != 0)
	{
		Test_Fits_Header_Error(status);
		fits_close_file(fits_fp,&status);
		return FALSE;
	}
/* BITPIX keyword */
	ivalue = 16;
	retval = fits_update_key(fits_fp,TINT,(char*)"BITPIX",&ivalue,NULL,&status);
	if(retval != 0)
	{
		Test_Fits_Header_Error(status);
		fits_close_file(fits_fp,&status);
		return FALSE;
	}
/* NAXIS keyword */
	ivalue = 2;
	retval = fits_update_key(fits_fp,TINT,(char*)"NAXIS",&ivalue,NULL,&status);
	if(retval != 0)
	{
		Test_Fits_Header_Error(status);
		fits_close_file(fits_fp,&status);
		return FALSE;
	}
/* NAXIS1 keyword */
	ivalue = ncols;
	retval = fits_update_key(fits_fp,TINT,(char*)"NAXIS1",&ivalue,NULL,&status);
	if(retval != 0)
	{
		Test_Fits_Header_Error(status);
		fits_close_file(fits_fp,&status);
		return FALSE;
	}
/* NAXIS2 keyword */
	ivalue = nrows;
	retval = fits_update_key(fits_fp,TINT,(char*)"NAXIS2",&ivalue,NULL,&status);
	if(retval != 0)
	{
		Test_Fits_Header_Error(status);
		fits_close_file(fits_fp,&status);
		return FALSE;
	}
/* BZERO keyword */
	dvalue = 32768.0;
	retval = fits_update_key_fixdbl(fits_fp,(char*)"BZERO",dvalue,6,
		(char*)"Number to offset data values by",&status);
	if(retval != 0)
	{
		Test_Fits_Header_Error(status);
		fits_close_file(fits_fp,&status);
		return FALSE;
	}
/* BSCALE keyword */
	dvalue = 1.0;
	retval = fits_update_key_fixdbl(fits_fp,(char*)"BSCALE",dvalue,6,
		(char*)"Number to multiply data values by",&status);
	if(retval != 0)
	{
		Test_Fits_Header_Error(status);
		fits_close_file(fits_fp,&status);
		return FALSE;
	}
/* close file */
	if(fits_close_file(fits_fp,&status))
	{
		Test_Fits_Header_Error(status);
		return FALSE;
	}
	return TRUE;
}

/**
 * Internal routine to write the complete CFITSIO error stack to stderr.
 * @param status The status returned by CFITSIO.
 */
static void Test_Fits_Header_Error(int status)
{
	/* report the whole CFITSIO error message stack to stderr. */
	fits_report_error(stderr, status);
}
/*
** $Log: not supported by cvs2svn $
** Revision 1.13  2002/11/28 17:57:51  cjm
** Added rcsid.
**
** Revision 1.12  2002/11/07 19:18:22  cjm
** Changes to make library work with SDSU version 1.7 DSP code.
**
** Revision 1.11  2001/02/01 10:22:27  cjm
** Tidied program and made interface a parameter.
**
** Revision 1.10  2001/01/30 17:51:48  cjm
** Added comments.
**
** Revision 1.9  2001/01/30 12:35:47  cjm
** Added BZERO and BSCALE keywords so FITS images save successfully.
**
*/

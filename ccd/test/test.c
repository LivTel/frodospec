/* test.c  -*- mode: Fundamental;-*-
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

#define CCD_X_SIZE	2048
#define CCD_Y_SIZE	2048
#define CCD_XBIN_SIZE	1
#define CCD_YBIN_SIZE	1
#define BYTES_PER_PIXEL	2

static int Test_Save_Fits_Headers(int exposure_time,int ncols,int nrows,char *filename);
static void Test_Fits_Header_Error(int status);

int main(int argc, char *argv[])
{
	struct CCD_Setup_Window_Struct window_list[CCD_SETUP_WINDOW_COUNT];
	struct timespec start_time;
	int retval;
	char *exposure_data = NULL;
	double temperature = 0.0;

	fprintf(stdout,"Test ...\n");
	CCD_Global_Initialise(CCD_INTERFACE_DEVICE_TEXT);

	CCD_Text_Set_Print_Level(CCD_TEXT_PRINT_LEVEL_COMMANDS);

	fprintf(stdout,"Test:CCD_Interface_Open\n");
	CCD_Interface_Open();

	fprintf(stdout,"Test:CCD_Temperature_Get\n");
	if(CCD_Temperature_Get(&temperature))
		fprintf(stdout,"Test:CCD_Temperature_Get returned %.2f\n",temperature);
	else
		CCD_Global_Error();

	fprintf(stdout,"Test:CCD_Setup_Startup\n");
	if(!CCD_Setup_Startup(CCD_SETUP_LOAD_ROM,NULL,
		CCD_SETUP_LOAD_APPLICATION,0,NULL,
		CCD_SETUP_LOAD_APPLICATION,1,NULL,-107.0,
		CCD_DSP_GAIN_FOUR,TRUE,TRUE))
		CCD_Global_Error();
	fprintf(stdout,"Test:CCD_Setup_Startup completed\n");

	fprintf(stdout,"Test:CCD_Setup_Dimensions\n");
	if(!CCD_Setup_Dimensions(CCD_X_SIZE,CCD_Y_SIZE,CCD_XBIN_SIZE,CCD_YBIN_SIZE,
		CCD_DSP_AMPLIFIER_LEFT,CCD_DSP_DEINTERLACE_SINGLE,0,window_list))
		CCD_Global_Error();
	fprintf(stdout,"Test:CCD_Setup_Dimensions completed\n");

	fprintf(stdout,"Test:CCD_Setup_Filter_Wheel\n");
	if(!CCD_Filter_Wheel_Reset(0))
		CCD_Global_Error();
	if(!CCD_Filter_Wheel_Reset(1))
		CCD_Global_Error();
	fprintf(stdout,"Test:CCD_Setup_Filter_Wheel completed\n");

	if(!Test_Save_Fits_Headers(10000,CCD_X_SIZE/CCD_XBIN_SIZE,CCD_Y_SIZE/CCD_YBIN_SIZE,"test.fits"))
		fprintf(stdout,"Test:Saving FITS headers failed.\n");
	fprintf(stdout,"Test:Saving FITS headers completed.\n");

	start_time.tv_sec = 0;
	start_time.tv_nsec = 0;
	if(!CCD_Exposure_Expose(TRUE,start_time,10000,"test.fits"))
		CCD_Global_Error();
	fprintf(stdout,"Test:CCD_Exposure_Expose finished\n");

	if(!CCD_Setup_Shutdown())
		CCD_Global_Error();
	fprintf(stdout,"Test:CCD_Setup_Shutdown completed\n");

	CCD_Interface_Close();
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

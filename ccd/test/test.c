/* test.c  -*- mode: Fundamental;-*-
*/
#include <stdio.h>
#include "ccd_setup.h"
#include "ccd_exposure.h"
#include "ccd_dsp.h"
#include "ccd_interface.h"
#include "ccd_text.h"
#include "ccd_temperature.h"
#include "ccd_setup.h"

#define CCD_X_SIZE	16
#define CCD_Y_SIZE	10
#define CCD_XBIN_SIZE	1
#define CCD_YBIN_SIZE	1
#define BYTES_PER_PIXEL	2

int main(int argc, char *argv[])
{
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

	fprintf(stdout,"Test:CCD_Setup_Setup\n");
	if(!CCD_Setup_Setup_CCD(CCD_SETUP_FLAG_ALL,CCD_SETUP_LOAD_APPLICATION,0,NULL,
		CCD_SETUP_LOAD_APPLICATION,1,NULL,-123.0,
		CCD_SETUP_GAIN_FOUR,TRUE,TRUE,CCD_X_SIZE,CCD_Y_SIZE,CCD_XBIN_SIZE,CCD_YBIN_SIZE,
		CCD_SETUP_DEINTERLACE_SINGLE))
		CCD_Global_Error();
	fprintf(stdout,"Test:CCD_Setup_Setup completed\n");

	if(!CCD_Exposure_Expose(TRUE,TRUE,10000,"test.fits"))
		CCD_Global_Error();

	fprintf(stdout,"Test:CCD_Exposure_Expose finished\n");

	CCD_Interface_Close();
	fprintf(stdout,"Test:CCD_Interface_Close\n");

	fprintf(stdout,"Test:Finished Test ...\n");
	return 0;
}







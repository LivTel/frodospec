/* ccd_temperature.c
** low level ccd library
** $Header: /home/cjm/cvs/frodospec/ccd/c/ccd_temperature.c,v 0.10 2004-03-03 15:43:49 cjm Exp $
*/

/**
 * ccd_temperature holds the routines for calulating the current CCD temperature and setting the CCDs
 * temperature.
 * The CCD_Temperature_Get function calculates the temperature of the the ccd using the
 * Chebychev series and the coefficients given for the CY7 Series
 * Silicon Diodes Standard Curve #10 manufactured by Omega Engineering
 * For this diode, there are eleven (11) coefficients.  Also, the adu-
 * to-voltage conversion factor is needed to use the formula given by
 * Omega Engineering.
 * @author SDSU, Chris Mottram
 * @version $Revision: 0.10 $
 */
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
#define _POSIX_SOURCE 1
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "ccd_global.h"
#include "ccd_dsp.h"
#include "ccd_temperature.h"

/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id: ccd_temperature.c,v 0.10 2004-03-03 15:43:49 cjm Exp $";

/**
 * The number of coefficients used to calculate the temperature.
 */
#define TEMPERATURE_COEFF_COUNT			11

/**
 * The number of coefficients used to calculate the temperature.
 */
#define TEMPERATURE_DEFAULT_NTEMP		TEMPERATURE_COEFF_COUNT

/**
 * Temperature Coefficient 0.
 */
#define TEMPERATURE_DEFAULT_COEFF_0		287.756797
/**
 * Temperature Coefficient 1.
 */
#define TEMPERATURE_DEFAULT_COEFF_1		-194.144823
/**
 * Temperature Coefficient 2.
 */
#define TEMPERATURE_DEFAULT_COEFF_2		-3.837903
/**
 * Temperature Coefficient 3.
 */
#define TEMPERATURE_DEFAULT_COEFF_3		-1.318325
/**
 * Temperature Coefficient 4.
 */
#define TEMPERATURE_DEFAULT_COEFF_4		-0.109120
/**
 * Temperature Coefficient 5.
 */
#define TEMPERATURE_DEFAULT_COEFF_5		-0.393265
/**
 * Temperature Coefficient 6.
 */
#define TEMPERATURE_DEFAULT_COEFF_6		0.146911
/**
 * Temperature Coefficient 7.
 */
#define TEMPERATURE_DEFAULT_COEFF_7		-0.111192
/**
 * Temperature Coefficient 8.
 */
#define TEMPERATURE_DEFAULT_COEFF_8		0.028877
/**
 * Temperature Coefficient 9.
 */
#define TEMPERATURE_DEFAULT_COEFF_9		-0.029286
/**
 * Temperature Coefficient 10.
 */
#define TEMPERATURE_DEFAULT_COEFF_10		0.015619

/**
 * The default upper voltage used to calculate CCD temperature.
 */
#define TEMPERATURE_DEFAULT_VU			0.999614
/**
 * The default lower voltage used to calculate CCD temperature.
 */
#define TEMPERATURE_DEFAULT_VL			0.079767

/**
 * The default adu per volt used to calculate CCD temperature.
 */
#define TEMPERATURE_DEFAULT_ADU_PER_VOLT	1366.98
/**
 * The default adu offset used to calculate CCD temperature.
 */
#define TEMPERATURE_DEFAULT_ADU_OFFSET		2045

/**
 * The number of times to read the SDSU CCD Controller to determine the temperature.
 * @see #CCD_Temperature_Get
 */
#define TEMPERATURE_MAX_CHECKS  		30

/**
 * How close the calculated temperature has to be to the target temperature when calculating an ADU count
 * for a given temperature.
 * @see #Temperature_Calc_Temp_ADU
 */
#define TEMPERATURE_TOLERANCE			0.5
/**
 * The number of times to try to improve the accuracy of the ADU count when trying to work out an ADU
 * count for a given temperature. This stops the algorithm going into an infinite loop trying to get
 * an ADU count thats within <a href="#TEMPERATURE_TOLERANCE">TEMPERATURE_TOLERANCE</a> of the target temperature.
 * @see #Temperature_Calc_Temp_ADU
 */
#define TEMPERATURE_MAX_TOLERANCE_TRIALS	30
/**
 * Definition of the memory address where the Temperature regulation ADU counts are held,
 * in utility board Y memory space. This is the same value as is in the DSP code (DAC0).
 * @see#CCD_Temperature_Get_Heater_ADU
 */
#define TEMPERATURE_HEATER_ADDRESS		(0x2)
/**
 * This is the address on the SDSU utility board, Y memory space, to read the current ADU count of the thermistor
 * mounted on the utility board. 
 * This ADU count is read from the temperature monitoring device.
 */
#define TEMPERATURE_UTILITY_BOARD_ADU_ADDRESS	(0x7)
/**
 * This is the address on the SDSU utility board, Y memory space, to read the current ADU count of the thermistor
 * mounted in the dewar. 
 * This ADU count is read from the temperature monitoring device.
 */
#define TEMPERATURE_CURRENT_ADU_ADDRESS		(0xc)
/**
 * This is the address on the SDSU utility board to write the required ADU count. This ADU count
 * is derived from a temperature, and allows the utility board to control the CCD temperature.
 */
#define TEMPERATURE_REQUIRED_ADU_ADDRESS	(0x1c)
/**
 * How long we sleep for, in milliseconds, between successive calls to read
 * the current temperature ADU count from the utility board. The ADU count is
 * only calculated every 3ms.
 */
#define TEMPERATURE_GET_SLEEP_MS	(2)

/* data types */
/**
 * Data type holding local data to ccd_temperature. This data is all the numerical constants neccessary to
 * calculate temperatures for a particular temperature sensor connected to the system.
 */
typedef struct Temperature_Struct
{
	int	Temp_Coeff_Count;
	float	V_Upper;
	float	V_Lower;
	float	Adu_Per_Volt;
	int	Adu_Offset;
	float	Temp_Coeff[TEMPERATURE_COEFF_COUNT];
} TEMPERATURE_ATTR;

/* external variables */

/* internal variables */
/**
 * Variable holding error code of last operation performed by ccd_temperature.
 */
static int Temperature_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 */
static char Temperature_Error_String[CCD_GLOBAL_ERROR_STRING_LENGTH] = "";
/**
 * Data holding the current diode configuration values for a particular temperature sensor.
 */
static TEMPERATURE_ATTR Temperature_Data = 
{
	TEMPERATURE_DEFAULT_NTEMP,
	TEMPERATURE_DEFAULT_VU,TEMPERATURE_DEFAULT_VL,
	TEMPERATURE_DEFAULT_ADU_PER_VOLT,TEMPERATURE_DEFAULT_ADU_OFFSET,
	{TEMPERATURE_DEFAULT_COEFF_0,TEMPERATURE_DEFAULT_COEFF_1,TEMPERATURE_DEFAULT_COEFF_2,
	TEMPERATURE_DEFAULT_COEFF_3,TEMPERATURE_DEFAULT_COEFF_4,
	TEMPERATURE_DEFAULT_COEFF_5,TEMPERATURE_DEFAULT_COEFF_6,TEMPERATURE_DEFAULT_COEFF_7,
	TEMPERATURE_DEFAULT_COEFF_8,TEMPERATURE_DEFAULT_COEFF_9,
	TEMPERATURE_DEFAULT_COEFF_10}
};

/* internal function definitions */
static float Temperature_Temperature(float temp_coeff[],int n,float vu,float vl,float adu_per_volt,int adu_offset,
	float adu);
static int Temperature_Calc_Temp_ADU(float temp_coeff[],int n,float vu,float vl,float adu_per_volt,int adu_offset,
	float temperature);

/* external functions */
/**
 * This routine gets the current temperature of the CCD in the dewar using the SDSU CCD Controller utility board.
 * It reads the utility board using CCD_DSP_Command_RDM to read memory which has the digital counts 
 * of the voltage from the temperature sensor in it. This is done TEMPERATURE_MAX_CHECKS times.
 * The temperature is calculated from the adu by calling Temperature_Temperature. 
 * If the voltage is out of range an error is returned.
 * @param temperature The address of a variable to hold the calculated temperature to be returned.
 * 	The returned temperature is in degrees centigrade.
 * @return TRUE if the operation was successfull and the temperature returned was sensible, FALSE
 * 	if a failure occured or the temperature returned was not sensible.
 * @see #TEMPERATURE_MAX_CHECKS
 * @see #TEMPERATURE_CURRENT_ADU_ADDRESS
 * @see #TEMPERATURE_GET_SLEEP_MS
 * @see #Temperature_Temperature
 * @see ccd_dsp.html#CCD_DSP_Command_RDM
 */
int CCD_Temperature_Get(double *temperature)
{
	struct timespec sleep_time;
	int adu,retval;
	int i;
	float voltage;

	Temperature_Error_Number = 0;
#if LOGGING > 0
	CCD_Global_Log(CCD_GLOBAL_LOG_BIT_TEMPERATURE,"CCD_Temperature_Get() started.");
#endif
	if(temperature == NULL)
	{
		Temperature_Error_Number = 7;
		sprintf(Temperature_Error_String,"CCD_Temperature_Get:temperature pointer was NULL.");
		return FALSE;
	}
	CCD_DSP_Set_Abort(FALSE);
	adu = 0;
	for (i = 0; i < TEMPERATURE_MAX_CHECKS; i++)
	{
		retval = CCD_DSP_Command_RDM(CCD_DSP_UTIL_BOARD_ID,CCD_DSP_MEM_SPACE_Y,
			TEMPERATURE_CURRENT_ADU_ADDRESS);
		if((retval == 0)&&(CCD_DSP_Get_Error_Number() != 0))
		{
			CCD_DSP_Set_Abort(FALSE);
			Temperature_Error_Number = 3;
			sprintf(Temperature_Error_String,"CCD_Temperature_Get:Read temperature (%d) failed",i);
			return FALSE;
		}
#if LOGGING > 9
		CCD_Global_Log_Format(CCD_GLOBAL_LOG_BIT_TEMPERATURE,"CCD_Temperature_Get():RDM returned %d.",
			retval);
#endif
	/* ensure returned adu in range */
		retval = retval & 0xFFF;
		adu += retval;
	/* Sleep for 2 milliseconds. The controller only updates the temperature adu value every 3 ms. */
		sleep_time.tv_sec = 0;
		sleep_time.tv_nsec = TEMPERATURE_GET_SLEEP_MS*CCD_GLOBAL_ONE_MILLISECOND_NS;
		nanosleep(&sleep_time,NULL);
	}

#if LOGGING > 9
	CCD_Global_Log_Format(CCD_GLOBAL_LOG_BIT_TEMPERATURE,"CCD_Temperature_Get():%d RDMs returned %d.",
			TEMPERATURE_MAX_CHECKS,adu);
#endif

	/* Average the adu counts */
	adu = adu / TEMPERATURE_MAX_CHECKS;

#if LOGGING > 9
	CCD_Global_Log_Format(CCD_GLOBAL_LOG_BIT_TEMPERATURE,"CCD_Temperature_Get():Average adu:%d.",adu);
#endif

	/* Calculate the temperature */
	(*temperature) = (double)Temperature_Temperature(Temperature_Data.Temp_Coeff,Temperature_Data.Temp_Coeff_Count,
		Temperature_Data.V_Upper,
		Temperature_Data.V_Lower,Temperature_Data.Adu_Per_Volt,Temperature_Data.Adu_Offset,(float)adu);

#if LOGGING > 9
	CCD_Global_Log_Format(CCD_GLOBAL_LOG_BIT_TEMPERATURE,"CCD_Temperature_Get():Temperature:%.2f.",(*temperature));
#endif

	voltage = (adu - Temperature_Data.Adu_Offset) / Temperature_Data.Adu_Per_Volt; 

#if LOGGING > 9
	CCD_Global_Log_Format(CCD_GLOBAL_LOG_BIT_TEMPERATURE,"CCD_Temperature_Get():Voltage:%.2f.",voltage);
#endif

	/* is the voltage in range? */
	if ((voltage > Temperature_Data.V_Lower) && (voltage < Temperature_Data.V_Upper))
	{
#if LOGGING > 0
		CCD_Global_Log_Format(CCD_GLOBAL_LOG_BIT_TEMPERATURE,"CCD_Temperature_Get() returned %.2f.",
			(*temperature));
#endif
		return TRUE;
	}
	else
	{
		Temperature_Error_Number = 1;
		sprintf(Temperature_Error_String,"CCD Temperature Out of range: adu = %d T = %4.1f",
			adu,(*temperature));
		return FALSE;
	}
}

/**
 * This routine gets the current ADU of the utility board temperature sensor.
 * It reads the utility board using CCD_DSP_Command_RDM to read memory which has the digital counts 
 * of the voltage from the utility board temperature sensor in it.
 * @param adu The address of a variable to hold the Analogue Digital Units to be returned.
 * @return TRUE if the operation was successfull, FALSE if a failure occured.
 * @see #TEMPERATURE_UTILITY_BOARD_ADU_ADDRESS
 * @see ccd_dsp.html#CCD_DSP_Command_RDM
 */
int CCD_Temperature_Get_Utility_Board_ADU(int *adu)
{
	int retval;

	Temperature_Error_Number = 0;
#if LOGGING > 0
	CCD_Global_Log(CCD_GLOBAL_LOG_BIT_TEMPERATURE,"CCD_Temperature_Get_Utility_Board() started.");
#endif
	if(adu == NULL)
	{
		Temperature_Error_Number = 8;
		sprintf(Temperature_Error_String,"CCD_Temperature_Get:adu pointer was NULL.");
		return FALSE;
	}
	CCD_DSP_Set_Abort(FALSE);
	retval = CCD_DSP_Command_RDM(CCD_DSP_UTIL_BOARD_ID,CCD_DSP_MEM_SPACE_Y,TEMPERATURE_UTILITY_BOARD_ADU_ADDRESS);
	if((retval == 0)&&(CCD_DSP_Get_Error_Number() != 0))
	{
		CCD_DSP_Set_Abort(FALSE);
		Temperature_Error_Number = 6;
		sprintf(Temperature_Error_String,"CCD_Temperature_Get_Utility_Board:"
			"Read temperature failed");
		return FALSE;
	}
	(*adu) = retval;
#if LOGGING > 9
	CCD_Global_Log_Format(CCD_GLOBAL_LOG_BIT_TEMPERATURE,"CCD_Temperature_Get_Utility_Board():returned %d.",
			      (*adu));
#endif
	return TRUE;
}

/**
 * Routine to set the target temperature the SDSU CCD Controller will try to keep the CCD at during
 * operation of the camera. First <a href="#Temperature_Calc_Temp_ADU">Temperature_Calc_Temp_ADU</a> 
 * is called to get an ADU value for the
 * target_temperature and this is then written to the utility board using a 
 * write memory command using CCD_DSP_Command_WRM.
 * @param target_temperature The temperature we want the CCD cooled to, in degrees centigrade.
 * @return TRUE if the target temperature was set, FALSE if an error occured.
 * @see #TEMPERATURE_REQUIRED_ADU_ADDRESS
 * @see ccd_dsp.html#CCD_DSP_Command_WRM
 */
int CCD_Temperature_Set(double target_temperature)
{
	int adu = 0;

	Temperature_Error_Number = 0;
#if LOGGING > 0
	CCD_Global_Log_Format(CCD_GLOBAL_LOG_BIT_TEMPERATURE,"CCD_Temperature_Set(temperature=%.2f) started.",
		target_temperature);
#endif
	CCD_DSP_Set_Abort(FALSE);
	/* get the target adu count from target_temperature using the setup data */
	adu = Temperature_Calc_Temp_ADU(Temperature_Data.Temp_Coeff,Temperature_Data.Temp_Coeff_Count,
		Temperature_Data.V_Upper,Temperature_Data.V_Lower, 
		Temperature_Data.Adu_Per_Volt,Temperature_Data.Adu_Offset,target_temperature);
	/* write the target to memory */
	if(CCD_DSP_Command_WRM(CCD_DSP_UTIL_BOARD_ID,CCD_DSP_MEM_SPACE_Y,
		TEMPERATURE_REQUIRED_ADU_ADDRESS,adu) != CCD_DSP_DON)
	{
		CCD_DSP_Set_Abort(FALSE);
		Temperature_Error_Number = 2;
		sprintf(Temperature_Error_String,"Setting CCD Temperature failed.");
		return FALSE;
	}
#if LOGGING > 0
	CCD_Global_Log(CCD_GLOBAL_LOG_BIT_TEMPERATURE,"CCD_Temperature_Set() returned TRUE.");
#endif
	return TRUE;
}

/**
 * This routine gets the current ADU count of the voltage fed into the heater that regulates
 * the CCD temperature.
 * CCD_DSP_Command_RDM is used to read the relevant DSP memory location.
 * @param heater_adu The address of an integer to hold the current heater ADU.
 * @return TRUE if the operation was successfull and the temperature returned was sensible, FALSE
 * 	if a failure occured or the temperature returned was not sensible.
 * @see ccd_dsp.html#CCD_DSP_Command_RDM
 * @see #TEMPERATURE_HEATER_ADDRESS
 */
int CCD_Temperature_Get_Heater_ADU(int *heater_adu)
{
	int retval;

	Temperature_Error_Number = 0;
#if LOGGING > 0
	CCD_Global_Log(CCD_GLOBAL_LOG_BIT_TEMPERATURE,"CCD_Temperature_Get_Heater_ADU() started.");
#endif
	CCD_DSP_Set_Abort(FALSE);
	if(heater_adu == NULL)
	{
		Temperature_Error_Number = 4;
		sprintf(Temperature_Error_String,"CCD_Temperature_Get_Heater_ADU:adu was NULL.");
		return FALSE;
	}
	retval = CCD_DSP_Command_RDM(CCD_DSP_UTIL_BOARD_ID,CCD_DSP_MEM_SPACE_Y,TEMPERATURE_HEATER_ADDRESS);
	if((retval == 0)&&(CCD_DSP_Get_Error_Number() != 0))
	{
		CCD_DSP_Set_Abort(FALSE);
		Temperature_Error_Number = 5;
		sprintf(Temperature_Error_String,"CCD_Temperature_Get_Heater_ADU:Read memory failed.");
		return FALSE;
	}
	(*heater_adu) = retval;
#if LOGGING > 0
	CCD_Global_Log_Format(CCD_GLOBAL_LOG_BIT_TEMPERATURE,"CCD_Temperature_Get_Heater_ADU() returned %#x.",
		(*heater_adu));
#endif
	return TRUE;
}

/**
 * Get the current value of the error number.
 * @return The current value of the error number.
 */
int CCD_Temperature_Get_Error_Number(void)
{
	return Temperature_Error_Number;
}

/**
 * The error routine that reports any errors occuring in ccd_temperature in a standard way.
 * @see ccd_global.html#CCD_Global_Get_Current_Time_String
 */
void CCD_Temperature_Error(void)
{
	char time_string[32];

	CCD_Global_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Temperature_Error_Number == 0)
		sprintf(Temperature_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s CCD_Temperature:Error(%d) : %s\n",time_string,
		Temperature_Error_Number,Temperature_Error_String);
}

/**
 * The error routine that reports any errors occuring in ccd_temperature in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 * @see ccd_global.html#CCD_Global_Get_Current_Time_String
 */
void CCD_Temperature_Error_String(char *error_string)
{
	char time_string[32];

	CCD_Global_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Temperature_Error_Number == 0)
		sprintf(Temperature_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s CCD_Temperature:Error(%d) : %s\n",time_string,
		Temperature_Error_Number,Temperature_Error_String);
}

/* -----------------------------------------------------------------------------
** 	internal functions 
** ----------------------------------------------------------------------------- */
/**
 * Calculates the temperature from the adu, using the parameters passed in for a particular diode.
 * @param temp_coeff A set of temperature coefficients for the particular diode used for measuring the
 * 	temperature.
 * @param n The number of temperature coefficients passed in the temp_coeff array.
 * @param vu The upper voltage limit.
 * @param vl The lower voltage limit.
 * @param adu_per_volt The number of ADU's per volt.
 * @param adu_offset The offset to add to the ADU.
 * @param adu The adu we want the temperature for.
 * @return The temperature in degrees centigrade.
 */
static float Temperature_Temperature(float temp_coeff[], int n,float vu, float vl,float adu_per_volt,int adu_offset,
	float adu)
{
	int i;

	float temperature = -273.15,voltage = 0.0,tc[TEMPERATURE_COEFF_COUNT],x;

/* Convert adu's to voltage */
	voltage = (adu - (float) adu_offset) / adu_per_volt;

/* Calculate dimensionless variable for the Chebychev series */
	x = ((voltage - vl) - (vu - voltage)) / (vu - vl);

	tc[0] = 1;
	tc[1] = x;

	if (n <=2 )
	{
		temperature += temp_coeff[0] + (temp_coeff[1] * x);  /* changed voltage to x */ 
	}
	else
	{
		temperature += temp_coeff[0] + (temp_coeff[1] * x);
		for (i = 2; i < n; i++)
		{
			tc[i] = 2.0 * x * tc[i-1] - tc[i-2];
			temperature += temp_coeff[i] * tc[i];
		}
	}
	return temperature;
}

/**
 * Determine an adu value for a given temperature. The algorithm is numerical. A start adu is calculated midway 
 * between 
 * the upper and lower voltage limits. <a href="#Temperature_Temperature">Temperature_Temperature</a> is called
 * to get temperature based on this guess, and then the guess adu is refined to be midway between
 * the midpoint and one of the limits. The process is repeated for up to 
 * <a href="#TEMPERATURE_MAX_TOLERANCE_TRIALS">TEMPERATURE_MAX_TOLERANCE_TRIALS</a> times or until 
 * the calculated temperature
 * is within <a href="#TEMPERATURE_TOLERANCE">TEMPERATURE_TOLERANCE</a> of the required temperature.
 * @return Returns the adu for the temperature.
 * @param temp_coeff A set of temperature coefficients for the particular diode used for measuring the
 * 	temperature.
 * @param n The number of temperature coefficients passed in the temp_coeff array.
 * @param vu The upper voltage limit.
 * @param vl The lower voltage limit.
 * @param adu_per_volt The number of ADU's per volt.
 * @param adu_offset The offset to add to the ADU.
 * @param temperature The temperature we want the adu for.
 */
static int Temperature_Calc_Temp_ADU(float temp_coeff[], int n,float vu, float vl,float adu_per_volt,int adu_offset,
	float temperature)
{
	int tolerance = 0, trials = 0;
	float adu = 0.0,vmid,lower,upper,target_temp=-273.15;
	int iadu;

	lower = vl;
	upper = vu;

	vmid = (lower + upper) * 0.5;

/* Numerically determine v for a given temperature   */
/* Choose an initial adu in the middle of vu and vl  */
	adu = vmid * adu_per_volt + (float) adu_offset;

	tolerance = (fabs(target_temp - temperature) > TEMPERATURE_TOLERANCE);

	while (tolerance && (trials < TEMPERATURE_MAX_TOLERANCE_TRIALS))
	{
		target_temp = Temperature_Temperature(temp_coeff,n,vu,vl,adu_per_volt,adu_offset,adu);
		tolerance = (fabs(target_temp - temperature) > TEMPERATURE_TOLERANCE);
		if (tolerance)
		{
			if (target_temp < temperature)
			{
				upper = vmid;
				vmid = (lower + upper) * 0.5;
				adu = vmid * adu_per_volt + (float) adu_offset;
			}
			else if (target_temp > temperature)
			{
				lower = vmid;
				vmid = (lower + upper) * 0.5;
				adu = vmid * adu_per_volt + (float) adu_offset;
			}
		}
		trials++;
	}
/* ensure adu's are in range. */
	iadu = (int)adu;
	iadu = iadu & 0xfff;
/* return adu */
	return iadu;
}

/*
** $Log: not supported by cvs2svn $
** Revision 0.9  2002/12/16 16:49:36  cjm
** Removed Error routines resetting error number to zero.
**
** Revision 0.8  2002/11/07 19:13:39  cjm
** Changes to make library work with SDSU version 1.7 DSP code.
**
** Revision 0.7  2001/07/13 09:48:48  cjm
** Added CCD_Temperature_Get_Heater_ADU.
**
** Revision 0.6  2001/06/04 14:42:44  cjm
** Added LOGGING code.
**
** Revision 0.5  2000/09/25 09:51:28  cjm
** Changes to use with v1.4 SDSU DSP code.
**
** Revision 0.4  2000/04/13 13:09:30  cjm
** Added current time to error routines.
**
** Revision 0.4  2000/04/13 13:03:14  cjm
** Changed error routine to print current time.
**
** Revision 0.3  2000/03/01 15:44:41  cjm
** Backup.
**
** Revision 0.2  2000/02/22 16:07:55  cjm
** Added calls to CCD_DSP_Set_Abort.
**
** Revision 0.1  2000/01/25 14:57:27  cjm
** initial revision (PCI version).
**
*/

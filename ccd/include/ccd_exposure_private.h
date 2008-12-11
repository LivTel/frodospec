/* ccd_exposure_private.h
** $Header: /home/cjm/cvs/frodospec/ccd/include/ccd_exposure_private.h,v 1.1 2008-12-11 14:19:59 cjm Exp $
*/

#ifndef CCD_EXPOSURE_PRIVATE_H
#define CCD_EXPOSURE_PRIVATE_H

#include "ccd_exposure.h" /* enum CCD_EXPOSURE_STATUS declaration */

/**
 * Structure used to hold local data to ccd_exposure.
 * <dl>
 * <dt>Exposure_Status</dt> <dd>Whether an operation is being performed to CLEAR, EXPOSE or READOUT the CCD.</dd>
 * <dt>Start_Exposure_Clear_Time</dt> <dd>The amount of time before we are due to start an exposure, 
 * 	that a CLEAR_ARRAY command should be sent to the controller. This time is in seconds, 
 * 	and must be greater than the time the CLEAR_ARRAY command takes to clock all accumulated charge off the CCD 
 * 	(approx 5 seconds for a 2kx2k EEV42-40).</dd>
 * <dt>Start_Exposure_Offset_Time</dt> <dd>The amount of time, in milliseconds, before the desired start of 
 * 	exposure that we should send the START_EXPOSURE command, to allow for transmission delay.</dd>
 * <dt>Readout_Remaining_Time</dt> <dd>Amount of time, in milleseconds,
 * 	remaining for an exposure when we change status to READOUT, to stop RDM/TDL/WRMs affecting the readout.</dd>
 * <dt>Exposure_Length</dt> <dd>The last exposure length to be set.</dd>
 * <dt>Exposure_Start_Time</dt> <dd>The time stamp when the START_EXPOSURE command was sent to the controller.</dd>
 * </dl>
 * @see ccd_exposure.html#CCD_EXPOSURE_STATUS
 */
struct CCD_Exposure_Struct
{
	enum CCD_EXPOSURE_STATUS Exposure_Status;
	int Start_Exposure_Clear_Time;
	int Start_Exposure_Offset_Time;
	int Readout_Remaining_Time;
	int Exposure_Length;
	struct timespec Exposure_Start_Time;
};


/*
** $Log: not supported by cvs2svn $
*/
#endif

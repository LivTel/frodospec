// ExposureThread.java -*- mode: Fundamental;-*-
// $Header: /home/cjm/cvs/frodospec/ccd/java/ExposureThread.java,v 0.7 1999-09-17 16:50:19 cjm Exp $
import java.lang.*;
import java.io.*;

import ngat.ccd.*;

/**
 * This class extends thread to support the exposure of a CCD camera using the SDSU CCD Controller/libccd/CCDLibrary
 * in a separate thread, so that it may be aborted by the main program whilst it is underway.
 * @author Chris Mottram
 * @version $Revision: 0.7 $
 */
class ExposureThread extends Thread
{
	/**
	 * Revision Control System id string, showing the version of the Class
	 */
	public final static String RCSID = new String("$Id: ExposureThread.java,v 0.7 1999-09-17 16:50:19 cjm Exp $");
	/**
	 * CCDLibrary object, the library object used to interface with the SDSU CCD Controller
	 */
	private CCDLibrary libccd = null;
	/**
	 * Private copy of variable to be passed into 
	 * CCDExposureExpose.
	 */
	private boolean open_shutter;
	/**
	 * Private copy of variable to be passed into 
	 * CCDExposureExpose.
	 */
	private boolean readout_ccd;
	/**
	 * Private copy of variable to be passed into 
	 * CCDExposureExpose.
	 */
	private int msecs;
	/**
	 * Private copy of variable to be passed into 
	 * CCDExposureExpose.
	 */
	private String filename = null;
	/**
	 * We want to know if this exposure was aborted at any point. The aborted variable keeps track of
	 * this and is set to true in the <a href="#abort">abort</a>routine.
	 * @see #abort
	 */
	private boolean aborted = false;
	/**
	 * If we abort an exposure at any point,we need to save the exposure status at that point
	 * to determine which operations we can do to recover the situation. Specifcally, if we abort
	 * during exposure(rather than readout) we can subsequently readout any data that accumulated during exposure.
	 * @see #abort
	 */
	private int abortExposureStatus = libccd.CCD_DSP_EXPOSURE_STATUS_NONE;
	/**
	 * Private copy of any exception returned by CCDExposureExpose. This will be null for successful
	 * completion of the method.
	 */
	private CCDLibraryNativeException exposeException = null;

	/**
	 * Constructor of the thread. Copys all the parameters, ready to pass them into
	 * CCDExposureExpose when the thread is run.
	 */
	public ExposureThread(CCDLibrary libccd,boolean open_shutter,boolean readout_ccd,int msecs,String filename)
	{
		this.libccd = libccd;
		this.open_shutter = open_shutter;
		this.readout_ccd = readout_ccd;
		this.msecs = msecs;
		if(filename != null)
			this.filename = new String(filename);
		else
			this.filename = null;
	}

	/**
	 * Run method of the thread. Calls
	 * CCDExposureExpose with the parameters passed into the
	 * constructor. This causes the CCD to expose. The success or failure of the
	 * operation is stored in <a href="#exposeException">exposeException</a>, which can be reteived using the 
	 * <a href="#getExposeException">getExposeException</a> method. Exposure can be aborted using the
	 * <a href="#abort">abort</a> method.
	 * @see #getReturnValue
	 * @see #abort
	 */
	public void run()
	{
		exposeException = null;
		try
		{
			libccd.CCDExposureExpose(open_shutter,readout_ccd,msecs,filename);
		}
		catch(CCDLibraryNativeException e)
		{
			exposeException = e;
		}
	}

	/**
	 * This method will terminate a partly completed Exposure. If libccd is currently exposing
	 * CCDExposureAbort is called which stops the exposure.
	 * If libccd is currently reading out
	 * CCDExposureAbortReadout is called which stops the CCD.
	 * reading out. 
	 * CCDDSPGetExposureStatus is used to determine
	 * the current state of the exposure. In either case libccd will cause
	 * CCDExposureExpose to stop what it is doing. This causes
	 * the <a href="#run">run</a> method to finish executing, and the 
	 * <a href="#exposeException">exposeException</a>
	 * will be non-null.
	 * @see #getExposeException
	 * @see #run
	 */
	public void abort()
	{
		aborted = true;
		abortExposureStatus = libccd.CCDDSPGetExposureStatus();
		switch(abortExposureStatus)
		{
			case libccd.CCD_DSP_EXPOSURE_STATUS_EXPOSE:
				libccd.CCDExposureAbort();
				break;
			case libccd.CCD_DSP_EXPOSURE_STATUS_READOUT:
				libccd.CCDExposureAbortReadout();
				break;
			default:
				stop();
				break;
		}
	}

	/**
	 * This returns any exception generated by CCDExposureExpose
	 * in the <a href="#run">run</a> method. If the thread hasn't been run yet it returns null. If the exposure 
	 * was successfully completed it returns null, otherwise it returns the created exception.
	 * @return The exception generated by CCDExposureExpose or null.
	 * @see #run
	 */
	public CCDLibraryNativeException getExposeException()
	{
		return exposeException;
	}

	/**
	 * This returns whether the exposure was aborted or not.
	 * @return Returns true if the exposure was aborted, false if it was not aborted.
	 * @see #abort
	 */
	public boolean getAbortStatus()
	{
		return aborted;
	}
	/**
	 * This returns the status of the exposure when the exposure was aborted. This variable
	 * is set when an exposure is aborted using <a href="#abort">abort</a>.
	 * @return If the exposure was not aborted 
	 * CCD_DSP_EXPOSURE_STATUS_NONE is returned.
	 * If the exposure was waiting for the exposure time to complete
	 * CCD_DSP_EXPOSURE_STATUS_EXPOSE is returned.
	 * If the exposure was reading out from the CCD
	 * CCD_DSP_EXPOSURE_STATUS_READOUT is returned.
	 */
	public int getAbortExposureStatus()
	{
		return abortExposureStatus;
	}
}
 
//
// $Log: not supported by cvs2svn $
// Revision 0.6  1999/09/10 15:27:11  cjm
// Changed due to CCDLibrary moving to ngat.ccd. package.
//
// Revision 0.5  1999/09/08 10:52:40  cjm
// Trying to fix file permissions of these files.
//
// Revision 0.4  1999/05/28 09:54:18  dev
// "Name
//
// Revision 0.3  1999/03/05 14:42:02  dev
// Backup
//
// Revision 0.2  1999/02/23 11:08:00  dev
// backup/transfer to ltccd1.
//
// Revision 0.1  1999/01/22 09:55:51  dev
// initial revision
//
//

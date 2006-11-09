// ExposureThread.java
// $Header: /home/cjm/cvs/frodospec/ccd/java/ExposureThread.java,v 0.14 2006-11-09 10:26:43 eng Exp $
import java.lang.*;
import java.io.*;

import ngat.ccd.*;
import ngat.fits.*;

/**
 * This class extends thread to support the exposure of a CCD camera using the SDSU CCD Controller/libccd/CCDLibrary
 * in a separate thread, so that it may be aborted by the main program whilst it is underway.
 * @author Chris Mottram
 * @version $Revision: 0.14 $
 */
class ExposureThread extends Thread
{
	/**
	 * Revision Control System id string, showing the version of the Class
	 */
	public final static String RCSID = new String("$Id: ExposureThread.java,v 0.14 2006-11-09 10:26:43 eng Exp $");
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
	 * X size of image.
	 */
	private int xSize;
	/**
	 * Y size of image.
	 */
	private int ySize;
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
	private int abortExposureStatus = libccd.CCD_EXPOSURE_STATUS_NONE;
	/**
	 * Private copy of any exception returned by the exposure thread. This will be null for successful
	 * completion of the method.
	 */
	private Exception exposeException = null;

	/**
	 * Constructor of the thread. Copys all the parameters, ready to pass them into
	 * CCDExposureExpose when the thread is run.
	 */
	public ExposureThread(CCDLibrary libccd,boolean open_shutter,boolean readout_ccd,
		int xs,int ys,int msecs,String filename)
	{
		this.libccd = libccd;
		this.open_shutter = open_shutter;
		this.readout_ccd = readout_ccd;
		this.xSize = xs;
		this.ySize = ys;
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
	 * @see #getExposeException
	 * @see #saveHeaders
	 * @see #abort
	 */
	public void run()
	{
		exposeException = null;
		try
		{
			saveHeaders();
			libccd.CCDExposureExpose(open_shutter,-1L,msecs,filename);
		}
		catch(CCDLibraryNativeException e)
		{
			exposeException = e;
		}
		catch(FitsHeaderException e)
		{
			exposeException = e;
		}
	}

	/**
	 * Routine to save some FITS headers to disk. Saves them to filename.
	 * @exception FitsHeaderException Thrown if an error occurs whilst writing to disc.
	 * @see #filename
	 * @see #xSize
	 * @see #ySize
	 */
	public void saveHeaders() throws FitsHeaderException
	{
		FitsHeader fits = null;

		fits = new FitsHeader();
		fits.add("SIMPLE",new Boolean(true),"A valid FITS file",null,0);
		fits.add("BITPIX",new Integer(16),"Bits per pixel","bits",1);
		fits.add("NAXIS",new Integer(2),"Number of axes","bits",2);
		fits.add("NAXIS1",new Integer(xSize),"X axes","pixels",3);
		fits.add("NAXIS2",new Integer(ySize),"Y axes","pixels",4);
		fits.add("BZERO",new Double(32768.0),"Number to offset data values by","counts",5);
		fits.add("BSCALE",new Double(1.0),"Counts scaling factor",null,6);
		fits.writeFitsHeader(filename);
	}

	/**
	 * This method will terminate a partly completed Exposure. If libccd is currently exposing
	 * CCDExposureAbort is called which stops the exposure.
	 * If libccd is currently reading out CCDExposureAbortReadout is called which stops the CCD
	 * reading out. 
	 * CCDDSPGetExposureStatus is used to determine the current state of the exposure. 
	 * In either case libccd will cause CCDExposureExpose to stop what it is doing. This causes
	 * the <a href="#run">run</a> method to finish executing, and the 
	 * <a href="#exposeException">exposeException</a> will be non-null.
	 * @see #getExposeException
	 * @see #run
	 */
	public void abort() throws CCDLibraryNativeException
	{
		aborted = true;
		abortExposureStatus = libccd.CCDExposureGetExposureStatus();
		if(abortExposureStatus != libccd.CCD_EXPOSURE_STATUS_NONE)
			libccd.CCDExposureAbort();
	}

	/**
	 * This returns any exception generated by CCDExposureExpose ot saveFitsHeaders
	 * in the <a href="#run">run</a> method. If the thread hasn't been run yet it returns null. If the exposure 
	 * was successfully completed it returns null, otherwise it returns the created exception.
	 * @return The exception generated by CCDExposureExpose or saveFitsHeaders, or null.
	 * @see #run
	 */
	public Exception getExposeException()
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
// Revision 0.13  2003/03/26 15:52:25  cjm
// Changed for windowing API change.
//
// Revision 0.12  2001/01/31 17:04:14  cjm
// Added saveHeaders method.
//
// Revision 0.11  2000/07/14 16:11:07  cjm
// Updated CCDExposureExpose call.
//
// Revision 0.10  2000/03/01 16:12:37  cjm
// Changed to reflect change in CCDExposureExpose API.
//
// Revision 0.9  2000/01/24 16:33:10  cjm
// Changed so that the deprecated stop method was not called.
//
// Revision 0.8  1999/09/17 17:21:59  cjm
// Fixed Javadoc comments.
//
// Revision 0.7  1999/09/17 16:50:19  cjm
// Changed due to CCDExposureExpose returning an exception on error rather than a boolean.
//
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

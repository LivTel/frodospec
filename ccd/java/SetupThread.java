// SetupThread.java -*- mode: Fundamental;-*-
// $Header: /home/cjm/cvs/frodospec/ccd/java/SetupThread.java,v 0.5 1999-09-20 10:27:40 cjm Exp $
import java.io.*;
import java.lang.*;

import ngat.ccd.*;

/**
 * This class extends thread to support the setup of a CCD camera using the SDSU CCD Controller/libccd/CCDLibrary
 * in a separate thread, so that it may be aborted by the main program whilst it is underway..
 * @author Chris Mottram
 * @version $Revision: 0.5 $
 */
class SetupThread extends Thread
{
	/**
	 * Revision Control System id string, showing the version of the Class
	 */
	public final static String RCSID = new String("$Id: SetupThread.java,v 0.5 1999-09-20 10:27:40 cjm Exp $");
	/**
	 * CCDLibrary object, the library object used to interface with the SDSU CCD Controller
	 */
	private CCDLibrary libccd 		= null;
	/**
	 * Private copy of variable to be passed into 
	 * CCDSetupSetupCCD.
	 */
	private int setup_flags 		= 0;
	/**
	 * Private copy of variable to be passed into 
	 * CCDSetupSetupCCD.
	 */
	private int timing_load_type 		= 0;
	/**
	 * Private copy of variable to be passed into 
	 * CCDSetupSetupCCD.
	 */
	private int timing_application_number 	= 0;
	/**
	 * Private copy of variable to be passed into 
	 * CCDSetupSetupCCD.
	 */
	private String timing_filename 		= null;
	/**
	 * Private copy of variable to be passed into 
	 * CCDSetupSetupCCD.
	 */
	private int utility_load_type 		= 0;
	/**
	 * Private copy of variable to be passed into 
	 * CCDSetupSetupCCD.
	 */
	private int utility_application_number 	= 0;
	/**
	 * Private copy of variable to be passed into 
	 * CCDSetupSetupCCD.
	 */
	private String utility_filename 	= null;
	/**
	 * Private copy of variable to be passed into 
	 * CCDSetupSetupCCD.
	 */
	private double target_temperature 	= 0.0;
	/**
	 * Private copy of variable to be passed into 
	 * CCDSetupSetupCCD.
	 */
	private int gain 			= 0;
	/**
	 * Private copy of variable to be passed into 
	 * CCDSetupSetupCCD.
	 */
	private boolean gain_speed 		= true;
	/**
	 * Private copy of variable to be passed into 
	 * CCDSetupSetupCCD.
	 */
	private boolean idle 			= false;
	/**
	 * Private copy of variable to be passed into 
	 * CCDSetupSetupCCD.
	 */
	private int ncols 			= 0;
	/**
	 * Private copy of variable to be passed into 
	 * CCDSetupSetupCCD.
	 */
	private int nrows 			= 0;
	/**
	 * Private copy of variable to be passed into 
	 * CCDSetupSetupCCD.
	 */
	private int nsbin 			= 0;
	/**
	 * Private copy of variable to be passed into 
	 * CCDSetupSetupCCD.
	 */
	private int npbin 			= 0;
	/**
	 * Private copy of variable to be passed into 
	 * CCDSetupSetupCCD.
	 */
	private int deinterlace_type 		= 0;
	/**
	 * Private copy of any exception returned by CCDSetupSetupCCD. This will be null for successful
	 * completion of the method.
	 */
	private CCDLibraryNativeException setupException = null;

	/**
	 * Constructor of the thread. Copys all the parameters, ready to pass them into
	 * CCDSetupSetupCCD when the thread is run.
	 */
	public SetupThread(CCDLibrary libccd,int setup_flags,
		int timing_load_type,int timing_application_number,String timing_filename,
		int utility_load_type,int utility_application_number,String utility_filename,
		double target_temperature,int gain,boolean gain_speed,boolean idle,
		int ncols,int nrows,int nsbin,int npbin,int deinterlace_type)
	{
		this.libccd = libccd;
		this.setup_flags = setup_flags;
		this.timing_load_type = timing_load_type;
		this.timing_application_number = timing_application_number;
		if(this.timing_filename != null)
			this.timing_filename = new String(timing_filename);
		else
			this.timing_filename = null;
		this.utility_load_type = utility_load_type;
		this.utility_application_number = utility_application_number;
		if(this.utility_filename != null)
			this.utility_filename = new String(utility_filename);
		else
			this.utility_filename = null;
		this.target_temperature = target_temperature;
		this.gain = gain;
		this.gain_speed = gain_speed;
		this.idle = idle;
		this.ncols = ncols;
		this.nrows = nrows;
		this.nsbin = nsbin;
		this.npbin = npbin;
		this.deinterlace_type = deinterlace_type;
	}

	/**
	 * Run method of the thread. Calls
	 * CCDSetupSetupCCD with the parameters passed into the
	 * constructor. This causes the CCD to be setup for an exposure. The success or failure of the
	 * operation is stored in <a href="#setupException">setupException</a>, which can be retieved using the 
	 * <a href="#getSetupException">getSetupException</a> method. Setup can be aborted using the
	 * <a href="#abort">abort</a> method.
	 * @see #getSetupException
	 * @see #abort
	 */
	public void run()
	{
		setupException = null;
		try
		{
			libccd.CCDSetupSetupCCD(setup_flags,timing_load_type,timing_application_number,
				timing_filename,utility_load_type,utility_application_number,utility_filename,
				target_temperature,gain,gain_speed,idle,
				ncols,nrows,nsbin,npbin,deinterlace_type);
		}
		catch(CCDLibraryNativeException e)
		{
			setupException = e;
		}
	}

	/**
	 * This method will terminate a partly completed Setup. It calls 
	 * CCDSetupAbort which at the libccd level tells
	 * CCDSetupSetupCCD to stop what it is doing. This causes
	 * the <a href="#run">run</a> method to finish halting the thread, the 
	 * <a href="#setupException">setupException</a>
	 * will be non-null.
	 * @see #getSetupException
	 * @see #run
	 */
	public void abort()
	{
		libccd.CCDSetupAbort();
	}

	/**
	 * This returns the return value generated by CCDSetupSetupCCD
	 * in the <a href="#run">run</a> method. If the thread hasn't been run yet it returns false. If the setup was
	 * successfully completed it returns true, otherwise it returns false.
	 * @see #run
	 */
	public CCDLibraryNativeException getSetupException()
	{
		return setupException;
	}
}
 
//
// $Log: not supported by cvs2svn $
// Revision 0.4  1999/09/10 15:55:11  cjm
// Changed due to CCDLibrary moving to ngat.ccd. package.
//
// Revision 0.3  1999/09/08 10:52:40  cjm
// Trying to fix file permissions of these files.
//
// Revision 0.2  1999/02/23 11:08:00  dev
// backup/transfer to ltccd1.
//
// Revision 0.1  1999/01/27 10:53:03  dev
// initial revision
//
//
//



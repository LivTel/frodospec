// SetupThread.java -*- mode: Fundamental;-*-
// $Header: /home/cjm/cvs/frodospec/ccd/java/SetupThread.java,v 0.2 1999-02-23 11:08:00 dev Exp $
import java.io.*;
import java.lang.*;

/**
 * This class extends thread to support the setup of a CCD camera using the SDSU CCD Controller/libccd/CCDLibrary
 * in a separate thread, so that it may be aborted by the main program whilst it is underway..
 * @author Chris Mottram
 * @version $Revision: 0.2 $
 */
class SetupThread extends Thread
{
	/**
	 * Revision Control System id string, showing the version of the Class
	 */
	public final static String RCSID = new String("$Id: SetupThread.java,v 0.2 1999-02-23 11:08:00 dev Exp $");
	/**
	 * CCDLibrary object, the library object used to interface with the SDSU CCD Controller
	 * @see CCDLibrary
	 */
	private CCDLibrary libccd 		= null;
	/**
	 * Private copy of variable to be passed into 
	 * <a href="CCDLibrary.html#CCDSetupSetupCCD">CCDSetupSetupCCD</a>.
	 * @see CCDLibrary#CCDSetupSetupCCD
	 */
	private int setup_flags 		= 0;
	/**
	 * Private copy of variable to be passed into 
	 * <a href="CCDLibrary.html#CCDSetupSetupCCD">CCDSetupSetupCCD</a>.
	 * @see CCDLibrary#CCDSetupSetupCCD
	 */
	private int timing_load_type 		= 0;
	/**
	 * Private copy of variable to be passed into 
	 * <a href="CCDLibrary.html#CCDSetupSetupCCD">CCDSetupSetupCCD</a>.
	 * @see CCDLibrary#CCDSetupSetupCCD
	 */
	private int timing_application_number 	= 0;
	/**
	 * Private copy of variable to be passed into 
	 * <a href="CCDLibrary.html#CCDSetupSetupCCD">CCDSetupSetupCCD</a>.
	 * @see CCDLibrary#CCDSetupSetupCCD
	 */
	private String timing_filename 		= null;
	/**
	 * Private copy of variable to be passed into 
	 * <a href="CCDLibrary.html#CCDSetupSetupCCD">CCDSetupSetupCCD</a>.
	 * @see CCDLibrary#CCDSetupSetupCCD
	 */
	private int utility_load_type 		= 0;
	/**
	 * Private copy of variable to be passed into 
	 * <a href="CCDLibrary.html#CCDSetupSetupCCD">CCDSetupSetupCCD</a>.
	 * @see CCDLibrary#CCDSetupSetupCCD
	 */
	private int utility_application_number 	= 0;
	/**
	 * Private copy of variable to be passed into 
	 * <a href="CCDLibrary.html#CCDSetupSetupCCD">CCDSetupSetupCCD</a>.
	 * @see CCDLibrary#CCDSetupSetupCCD
	 */
	private String utility_filename 	= null;
	/**
	 * Private copy of variable to be passed into 
	 * <a href="CCDLibrary.html#CCDSetupSetupCCD">CCDSetupSetupCCD</a>.
	 * @see CCDLibrary#CCDSetupSetupCCD
	 */
	private double target_temperature 	= 0.0;
	/**
	 * Private copy of variable to be passed into 
	 * <a href="CCDLibrary.html#CCDSetupSetupCCD">CCDSetupSetupCCD</a>.
	 * @see CCDLibrary#CCDSetupSetupCCD
	 */
	private int gain 			= 0;
	/**
	 * Private copy of variable to be passed into 
	 * <a href="CCDLibrary.html#CCDSetupSetupCCD">CCDSetupSetupCCD</a>.
	 * @see CCDLibrary#CCDSetupSetupCCD
	 */
	private boolean gain_speed 		= true;
	/**
	 * Private copy of variable to be passed into 
	 * <a href="CCDLibrary.html#CCDSetupSetupCCD">CCDSetupSetupCCD</a>.
	 * @see CCDLibrary#CCDSetupSetupCCD
	 */
	private boolean idle 			= false;
	/**
	 * Private copy of variable to be passed into 
	 * <a href="CCDLibrary.html#CCDSetupSetupCCD">CCDSetupSetupCCD</a>.
	 * @see CCDLibrary#CCDSetupSetupCCD
	 */
	private int ncols 			= 0;
	/**
	 * Private copy of variable to be passed into 
	 * <a href="CCDLibrary.html#CCDSetupSetupCCD">CCDSetupSetupCCD</a>.
	 * @see CCDLibrary#CCDSetupSetupCCD
	 */
	private int nrows 			= 0;
	/**
	 * Private copy of variable to be passed into 
	 * <a href="CCDLibrary.html#CCDSetupSetupCCD">CCDSetupSetupCCD</a>.
	 * @see CCDLibrary#CCDSetupSetupCCD
	 */
	private int nsbin 			= 0;
	/**
	 * Private copy of variable to be passed into 
	 * <a href="CCDLibrary.html#CCDSetupSetupCCD">CCDSetupSetupCCD</a>.
	 * @see CCDLibrary#CCDSetupSetupCCD
	 */
	private int npbin 			= 0;
	/**
	 * Private copy of variable to be passed into 
	 * <a href="CCDLibrary.html#CCDSetupSetupCCD">CCDSetupSetupCCD</a>.
	 * @see CCDLibrary#CCDSetupSetupCCD
	 */
	private int deinterlace_type 		= 0;
	/**
	 * Private copy of the Return value of 
	 * <a href="CCDLibrary.html#CCDSetupSetupCCD">CCDSetupSetupCCD</a>.
	 * @see CCDLibrary#CCDSetupSetupCCD
	 */
	private boolean returnValue 		= false;

	/**
	 * Constructor of the thread. Copys all the parameters, ready to pass them into
	 * <a href="CCDLibrary.html#CCDSetupSetupCCD">CCDSetupSetupCCD</a> when the thread is run.
	 * @see CCDLibrary#CCDSetupSetupCCD
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
	 * <a href="CCDLibrary.html#CCDSetupSetupCCD">CCDSetupSetupCCD</a> with the parameters passed into the
	 * constructor. This causes the CCD to be setup for an exposure. The success or failure of the
	 * operation is stored in <a href="#returnValue">returnValue</a>, which can be retieved using the 
	 * <a href="#getReturnValue">getReturnValue</a> method. Setup can be aborted using the
	 * <a href="#abort">abort</a> method.
	 * @see CCDLibrary#CCDSetupSetupCCD
	 * @see #getReturnValue
	 * @see #abort
	 */
	public void run()
	{
		returnValue = libccd.CCDSetupSetupCCD(setup_flags,timing_load_type,timing_application_number,
			timing_filename,utility_load_type,utility_application_number,utility_filename,
			target_temperature,gain,gain_speed,idle,
			ncols,nrows,nsbin,npbin,deinterlace_type);
	}

	/**
	 * This method will terminate a partly completed Setup. It calls 
	 * <a href="CCDLibrary.html#CCDSetupAbort">CCDSetupAbort</a> which at the libccd level tells
	 * <a href="CCDLibrary.html#CCDSetupSetupCCD">CCDSetupSetupCCD</a> to stop what it is doing. This causes
	 * the <a href="#run">run</a> method to finish halting the thread, the <a href="#returnValue">returnValue</a>
	 * will be false.
	 * @see CCDLibrary#CCDSetupAbort
	 * @see CCDLibrary#CCDSetupSetupCCD
	 * @see #getReturnValue
	 * @see #run
	 */
	public void abort()
	{
		returnValue = false;
		libccd.CCDSetupAbort();
	}

	/**
	 * This returns the return value generated by <a href="CCDLibrary.html#CCDSetupSetupCCD">CCDSetupSetupCCD</a>\
	 * in the <a href="#run">run</a> method. If the thread hasn't been run yet it returns false. If the setup was
	 * successfully completed it returns true, otherwise it returns false.
	 * @see CCDLibrary#CCDSetupSetupCCD
	 * @see #run
	 */
	public boolean getReturnValue()
	{
		return returnValue;
	}
}

//
// $Log: not supported by cvs2svn $
// Revision 0.1  1999/01/27 10:53:03  dev
// initial revision
//
//
//



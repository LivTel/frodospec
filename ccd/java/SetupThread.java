// SetupThread.java -*- mode: Fundamental;-*-
// $Header: /home/cjm/cvs/frodospec/ccd/java/SetupThread.java,v 0.1 1999-01-27 10:53:03 dev Exp $
import java.io.*;
import java.lang.*;

class SetupThread extends Thread
{
	public final static String RCSID = new String("$Id: SetupThread.java,v 0.1 1999-01-27 10:53:03 dev Exp $");
	private CCDLibrary libccd 		= null;
	private int setup_flags 		= 0;
	private int timing_load_type 		= 0;
	private int timing_application_number 	= 0;
	private String timing_filename 		= null;
	private int utility_load_type 		= 0;
	private int utility_application_number 	= 0;
	private String utility_filename 	= null;
	private double target_temperature 	= 0.0;
	private int gain 			= 0;
	private boolean gain_speed 		= true;
	private boolean idle 			= false;
	private int nrows 			= 0;
	private int ncols 			= 0;
	private int nsbin 			= 0;
	private int npbin 			= 0;
	private int deinterlace_setting 	= 0;
	private boolean returnValue 		= false;

	public SetupThread(CCDLibrary libccd,int setup_flags,
		int timing_load_type,int timing_application_number,String timing_filename,
		int utility_load_type,int utility_application_number,String utility_filename,
		double target_temperature,int gain,boolean gain_speed,boolean idle,
		int nrows,int ncols,int nsbin,int npbin,int deinterlace_setting)
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
		this.deinterlace_setting = deinterlace_setting;
	}

	public void run()
	{
		returnValue = libccd.CCDSetupSetup(setup_flags,timing_load_type,timing_application_number,
			timing_filename,utility_load_type,utility_application_number,utility_filename,
			target_temperature,gain,gain_speed,idle,
			ncols,nrows,nsbin,npbin,deinterlace_setting);
	}

	public void abort()
	{
		returnValue = false;
		libccd.CCDSetupAbort();
	}

	public boolean getReturnValue()
	{
		return returnValue;
	}
}

//
// $Log: not supported by cvs2svn $
//
//



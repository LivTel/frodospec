// Test.java -*- mode: Fundamental;-*-
// $Header: /home/cjm/cvs/frodospec/ccd/java/Test.java,v 0.10 2000-02-03 16:59:21 cjm Exp $
import java.lang.*;
import java.io.*;

import ngat.ccd.*;

/**
 * This is the main test program.
 * @author Chris Mottram
 * @version $Revision: 0.10 $
 */
class Test
{
	/**
	 * A CCD X size to pass into the library.
	 */
	private final static int CCD_X_SIZE 	= 2048;
	/**
	 * A CCD Y size to pass into the library.
	 */
	private final static int CCD_Y_SIZE 	= 2048;
	/**
	 * A CCD X bin size to pass into the library.
	 */
	private final static int CCD_XBIN_SIZE 	= 1;
	/**
	 * A CCD Y bin size to pass into the library.
	 */
	private final static int CCD_YBIN_SIZE 	= 1;
	/**
	 * The CCDLibrary object that handles communication with the low level SDSU libccd library.
	 */
	private CCDLibrary libccd = null;
	/**
	 * A Thread to manage the setup of the CCD.
	 */
	private SetupThread setupThread = null;
	/**
	 * A Thread to manage the exposure of the CCD.
	 */
	private ExposureThread exposureThread = null;
	/**
	 * A Thread to manage the readout of the CCD.
	 */
	private ReadOutThread readOutThread = null;
	/**
	 * A boolean determining whether to do an exposure or not.
	 */
	private boolean doExpose = false;
	/**
	 * A boolean determining whether to get the ccd's current temperature.
	 */
	private boolean doTemperature = false;
	/**
	 * A boolean determining whether the exposure thread was aborted.
	 */
	private boolean exposureAborted = false;

	/**
	 * Initialisation routine. Sets up the libccd library interface.
	 * @exception CCDLibraryNativeException Thrown if CCDInterfaceOpen fails.
	 * @see #libccd
	 */
	public void init() throws CCDLibraryNativeException
	{
		libccd = new CCDLibrary();

		libccd.CCDInitialise(libccd.CCD_INTERFACE_DEVICE_TEXT);
		libccd.CCDTextSetPrintLevel(libccd.CCD_TEXT_PRINT_LEVEL_COMMANDS);//libccd.CCD_TEXT_PRINT_LEVEL_ALL
		libccd.CCDInterfaceOpen();
	}

	/**
	 * Routine to start a setup thread to setup the ccd camera.
	 * @see #setupThread
	 */
	public boolean setup()
	{
		CCDLibraryNativeException setupException = null;
		AbortThread abortThread = null;
		boolean retval = false;
		int errorNumber = 0;

		abortThread = new AbortThread(this);
		abortThread.setPriority(Thread.NORM_PRIORITY-1);
		setupThread = new SetupThread(libccd,libccd.CCD_SETUP_LOAD_APPLICATION,1,null,
			libccd.CCD_SETUP_LOAD_APPLICATION,2,null,-107.0,libccd.CCD_DSP_GAIN_FOUR,true,true,
			CCD_X_SIZE,CCD_Y_SIZE,CCD_XBIN_SIZE,CCD_YBIN_SIZE,libccd.CCD_DSP_DEINTERLACE_SPLIT_QUAD);
		setupThread.setPriority(Thread.NORM_PRIORITY-1);
		setupThread.start();
		abortThread.start();
		while(setupThread.isAlive())
		{
			try
			{
				Thread.sleep(1000);
			}
			catch(InterruptedException e)
			{
				System.err.println(this.getClass().getName()+e);
			}
		}
		abortThread.quit();
		setupException = setupThread.getSetupException();
		if(setupException == null)
		{
			System.out.println("Test:Setup Completed.");
			retval = true;
		}
		else
		{
			System.err.println("Test:Setup returned:"+setupException);
			retval = false;
		}
		return retval;
	}

	/**
	 * Routine to get the temperature from the ccd.
	 */
	public void getTemperature()
	{
		CCDLibraryDouble temperature = null;

		temperature = new CCDLibraryDouble();
		try
		{
			libccd.CCDTemperatureGet(temperature);
			System.out.println("Test:CCDTemperatureGet:"+temperature.getValue());
		}
		catch(CCDLibraryNativeException e)
		{
			System.err.println(e.toString());
		}
	}

	/**
	 * Routine to start a thread to perform an exposure.
	 * @see #exposureThread
	 */
	public void expose()
	{
		AbortThread abortThread = null;
		CCDLibraryNativeException exposeException = null;
		int errorNumber = 0;

		abortThread = new AbortThread(this);
		abortThread.setPriority(Thread.NORM_PRIORITY-1);
		exposureThread = new ExposureThread(libccd,true,true,10000,new String("test.fits"));
		exposureThread.setPriority(Thread.NORM_PRIORITY-1);
		exposureThread.start();
		abortThread.start();
		exposureAborted = false;
		while(exposureThread.isAlive())
		{
			try
			{
				Thread.sleep(1000);
			}
			catch(InterruptedException e)
			{
				System.err.println(this.getClass().getName()+e);
			}
		}
		abortThread.quit();
		exposeException = exposureThread.getExposeException();
		exposureAborted = exposureThread.getAbortStatus();
		if(exposeException == null)
			System.out.println("Test:CCDExposureExpose Completed.");
		else
		{
			if(exposureAborted)
			{
				System.out.println("Test:CCDExposureExpose was Aborted");
				System.err.println("Test:CCDExposureExpose returned:"+exposeException);
			}
			else
				System.err.println("Test:CCDExposureExpose returned:"+exposeException);
		}
	}

	/**
	 * Routine to start a thread to read out the ccd camera.
	 * @see #readOutThread
	 */
	public void readout()
	{
		AbortThread abortThread = null;
		CCDLibraryNativeException readOutException = null;
		int errorNumber = 0;

		abortThread = new AbortThread(this);
		abortThread.setPriority(Thread.NORM_PRIORITY-1);
		readOutThread = new ReadOutThread(libccd,new String("test.fits"));
		readOutThread.setPriority(Thread.NORM_PRIORITY-1);
		readOutThread.start();
		abortThread.start();
		while(readOutThread.isAlive())
		{
			try
			{
				Thread.sleep(1000);
			}
			catch(InterruptedException e)
			{
				System.err.println(this.getClass().getName()+e);
			}
		}
		abortThread.quit();
		readOutException = readOutThread.getReadOutException();
		if(readOutException == null)
			System.out.println("Test:CCDExposureReadOutCCD Completed.");
		else
			System.err.println("Test:readout:"+readOutException);
	}

	/**
	 * Routine called from the abort thread, to abort the operation currently taking place.
	 * @see SetupThread#abort
	 * @see ExposureThread#abort
	 * @see ReadOutThread#abort
	 */
	public void abort()
	{
		if(setupThread != null)
		{
			if(setupThread.isAlive())
				setupThread.abort();
		}
		if(exposureThread != null)
		{
			if(exposureThread.isAlive())
				exposureThread.abort();
		}
		if(readOutThread != null)
		{
			if(readOutThread.isAlive())
				readOutThread.abort();
		}
	}

	/**
	 * Routine to try and recover from an aborted exposure.
	 * @see #exposureThread
	 */
	public void exposeRecover()
	{
		int reply;

		if(exposureThread.getAbortExposureStatus() == libccd.CCD_DSP_EXPOSURE_STATUS_EXPOSE)
		{
			System.out.println(this.getClass().getName()+":Exposure Aborted:"+
				"Try to readout data? (y/n)");
			try
			{
				reply = System.in.read();
				/* soak up return character before next AbortThread gets it */
				while(System.in.available()>0)
					System.in.read();
			}
			catch(IOException e)
			{
				System.err.println(this.getClass().getName()+e);
				reply = 'n';
			}						
			if((reply == 'y')||(reply == 'Y'))
			{
				System.out.println(this.getClass().getName()+
					":Trying to readout data");
				readout();
			}
			else
				System.out.println(this.getClass().getName()+
					":Not going to readout data");
		}
		else
			System.out.println(this.getClass().getName()+":Exposure Aborted:"+
				"Cannot readout data");
	}

	/**
	 * Routine to close the libccd interface.
	 */
	public void close()
	{
		try
		{
			libccd.CCDInterfaceClose();
		}
		catch(CCDLibraryNativeException e)
		{
			System.err.println("Test:close:"+e);
		}
	}

	/**
	 * Routine to parse arguments.
	 * @see #doTemperature
	 * @see #doExpose
	 */
	public void parseArgs(String []args)
	{
		for(int i = 0; i < args.length;i++)
		{
			if(args[i].equals("-temperature")||args[i].equals("-t"))
			{
				doTemperature = true;
			}
			else if(args[i].equals("-expose")||args[i].equals("-e"))
			{
				doExpose = true;
			}
			else
			{
				System.out.println(this.getClass().getName()+" Help:");
				System.out.println(this.getClass().getName()+" test the SDSU CCD Controller library");
				System.out.println("jre -cp <pathname of classes> "+this.getClass().getName()+
					" [Options]");
				System.out.println("Options are:");
				System.out.println("\t-t[emperature] - Get current CCD temperature.");
				System.out.println("\t-e[xpose] - Do an exposure.");
				System.out.println("\t-h[elp] - display this help.");
				System.exit(0);
			}
		}
	}

	/**
	 * Main routine of program. Creates an instance of test,initialises it and parses arguments,
	 * calls the camera setup, and optionally does an exposure/takes the ccd temperature.
	 * @see #init
	 * @see #parseArgs
	 * @see #setup
	 * @see #getTemperature
	 * @see #expose
	 * @see #close
	 */
	public static void main(String[] args)
	{
		Test test = new Test();

		try
		{
			test.init();
		}
		catch(CCDLibraryNativeException e)
		{
			System.err.println("Test:init failed:"+e);
			System.exit(1);
		}
		test.parseArgs(args);
		if(!test.setup())
		{
			test.close();
			System.exit(1);
		}
		if(test.doTemperature)
			test.getTemperature();
		if(test.doExpose)
		{
			test.expose();
			if(test.exposureAborted)
				test.exposeRecover();
		}
		test.close();

		System.out.println("Finished Test ...");
		System.exit(0);
	}
}
//
// $Log: not supported by cvs2svn $
// Revision 0.9  1999/09/20 14:39:48  cjm
// Changed due to libccd native routines throwing CCDLibraryNativeException when errors occur.
//
// Revision 0.8  1999/09/10 15:55:30  cjm
// Changed due to CCDLibrary moving to ngat.ccd. package.
//
// Revision 0.7  1999/09/08 10:52:40  cjm
// Trying to fix file permissions of these files.
//
//

// Test.java -*- mode: Fundamental;-*-
// $Header: /home/cjm/cvs/frodospec/ccd/java/Test.java,v 0.4 1999-03-08 12:20:40 dev Exp $
import java.lang.*;
import java.io.*;

class Test
{
	private final static int CCD_X_SIZE 	= 2048;
	private final static int CCD_Y_SIZE 	= 2048;
	private final static int CCD_XBIN_SIZE 	= 1;
	private final static int CCD_YBIN_SIZE 	= 1;
	private CCDLibrary libccd = null;
	private SetupThread setupThread = null;
	private ExposureThread exposureThread = null;
	private ReadOutThread readOutThread = null;
	private boolean doExpose = false;
	private boolean doTemperature = false;
	private boolean exposureAborted = false;

	public void init()
	{
		libccd = new CCDLibrary();

		libccd.CCDInitialise(libccd.INTERFACE_DEVICE_TEXT);
		libccd.CCDTextSetPrintLevel(libccd.TEXT_PRINT_LEVEL_COMMANDS);//libccd.TEXT_PRINT_LEVEL_ALL
		libccd.CCDInterfaceOpen();
	}

	public boolean setup()
	{
		AbortThread abortThread = null;
		boolean retval = false;
		int errorNumber = 0;

		abortThread = new AbortThread(this);
		abortThread.setPriority(Thread.NORM_PRIORITY-1);
		setupThread = new SetupThread(libccd,libccd.SETUP_FLAG_ALL,libccd.SETUP_LOAD_APPLICATION,1,null,
			libccd.SETUP_LOAD_APPLICATION,2,null,-123.0,libccd.SETUP_GAIN_FOUR,true,true,
			CCD_X_SIZE,CCD_Y_SIZE,CCD_XBIN_SIZE,CCD_YBIN_SIZE,libccd.SETUP_DEINTERLACE_SPLIT_QUAD);
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
		abortThread.stop();
		retval = setupThread.getReturnValue();
		if(retval)
			System.out.println("Test:CCDSetupSetup Completed.");
		else
		{
			errorNumber = libccd.CCDDSPGetErrorNumber();
			if((errorNumber == 4)||(errorNumber == 8)||(errorNumber == 12)||(errorNumber == 13))
				System.out.println("Test:CCDSetupSetup was Aborted:Error Message follows.");
			libccd.CCDError();
		}
		return retval;
	}

	public void getTemperature()
	{
		CCDLibraryDouble temperature = null;
		boolean retval;

		temperature = new CCDLibraryDouble();
		retval = libccd.CCDTemperatureGet(temperature);
		if(retval)
			System.out.println("Test:CCDTemperatureGet:"+temperature.getValue());
		else
			libccd.CCDError();
	}

	public void expose()
	{
		AbortThread abortThread = null;
		boolean retval = false;
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
		abortThread.stop();
		retval = exposureThread.getReturnValue();
		exposureAborted = exposureThread.getAbortStatus();
		if(retval)
			System.out.println("Test:CCDExposureExpose Completed.");
		else
		{
			if(exposureAborted)
			{
				System.out.println("Test:CCDExposureExpose was Aborted");
				errorNumber = libccd.CCDDSPGetErrorNumber();
				if((errorNumber == 4)||(errorNumber == 8)||(errorNumber == 12)||(errorNumber == 13))
					libccd.CCDError();
				else
					System.out.println("Test:CCDExposureExpose was Aborted without an error "+
						"occuring");
			}
			else
				libccd.CCDError();
		}
	}

	public void readout()
	{
		AbortThread abortThread = null;
		boolean retval = false;
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
		abortThread.stop();
		retval = readOutThread.getReturnValue();
		if(retval)
			System.out.println("Test:CCDExposureReadOutCCD Completed.");
		else
		{
			errorNumber = libccd.CCDDSPGetErrorNumber();
			if((errorNumber == 4)||(errorNumber == 8)||(errorNumber == 12)||(errorNumber == 13))
				System.out.println("Test:CCDExposureReadOutCCD was Aborted:Error Message follows.");
			libccd.CCDError();
		}
	}

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

	public void exposeRecover()
	{
		int reply;

		if(exposureThread.getAbortExposureStatus() == libccd.DSP_EXPOSURE_STATUS_EXPOSE)
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

	public void close()
	{
		libccd.CCDInterfaceClose();
	}

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

	public static void main(String[] args)
	{
		Test test = new Test();

		test.init();
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







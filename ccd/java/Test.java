// Test.java -*- mode: Fundamental;-*-
// $Header: /home/cjm/cvs/frodospec/ccd/java/Test.java,v 0.1 1999-01-27 10:55:24 dev Exp $
import java.lang.*;
import java.io.*;

class Test
{
	private final static int CCD_X_SIZE 	= 16;
	private final static int CCD_Y_SIZE 	= 10;
	private final static int CCD_XBIN_SIZE 	= 1;
	private final static int CCD_YBIN_SIZE 	= 1;
	private final static int BYTES_PER_PIXEL = 2;
	private CCDLibrary libccd = null;
	private ExposureThread exposureThread = null;
	private SetupThread setupThread = null;
	private byte[] exposureData = null;

	public void init()
	{
		libccd = new CCDLibrary();

		libccd.CCDSetup(libccd.INTERFACE_DEVICE_TEXT);
		libccd.CCDTextSetPrintLevel(libccd.TEXT_PRINT_LEVEL_COMMANDS);
		libccd.CCDInterfaceOpen();
	}

	public boolean setup()
	{
		AbortThread abortThread = null;
		boolean retval = false;

		abortThread = new AbortThread(this);
		abortThread.setPriority(Thread.NORM_PRIORITY-1);
		setupThread = new SetupThread(libccd,libccd.SETUP_FLAG_ALL,libccd.SETUP_LOAD_APPLICATION,0,null,
			libccd.SETUP_LOAD_APPLICATION,1,null,-123.0,libccd.SETUP_GAIN_FOUR,true,true,
			CCD_X_SIZE,CCD_Y_SIZE,CCD_XBIN_SIZE,CCD_YBIN_SIZE,0);
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
			libccd.CCDError();
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

		abortThread = new AbortThread(this);
		abortThread.setPriority(Thread.NORM_PRIORITY-1);
		exposureThread = new ExposureThread(libccd,true,true,CCD_X_SIZE,CCD_Y_SIZE,10000,0);
		exposureThread.setPriority(Thread.NORM_PRIORITY-1);
		exposureThread.start();
		abortThread.start();
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
	}

	public void readout()
	{
		int x,y;
		char ch;

		exposureData = exposureThread.getData();
		System.out.println("Test:Got Exposure Data:is null = "+(exposureData == null));
		if(exposureData == null)
			libccd.CCDError();
		else
		{
			for(y=0;y<(CCD_Y_SIZE/CCD_YBIN_SIZE);y++)
			{
				for(x=0;x<(CCD_X_SIZE/CCD_XBIN_SIZE)*BYTES_PER_PIXEL;x++)
				{
					ch = (char)exposureData[(y*((CCD_X_SIZE/CCD_XBIN_SIZE)*BYTES_PER_PIXEL))+x];
					System.out.print(ch);
				}
				System.out.print("\n");
			}
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
	}

	public void close()
	{
		libccd.CCDInterfaceClose();
	}

	public static void main(String[] args)
	{
		Test test = new Test();

		test.init();

		if(!test.setup())
		{
			test.close();
			System.exit(1);
		}
		test.getTemperature();

		test.expose();
		test.readout();

		test.close();

		System.out.println("Finished Test ...");
		System.exit(0);
	}
}







// TestLampController.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/test/TestLampController.java,v 1.1 2009-08-20 11:22:47 cjm Exp $
package ngat.frodospec.test;

import java.lang.*;
import java.text.*;
import ngat.frodospec.*;
import ngat.phase2.*;
import ngat.util.logging.*;

/**
 * This class tests the FrodoSpec LampController class.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class TestLampController
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: TestLampController.java,v 1.1 2009-08-20 11:22:47 cjm Exp $");
	/**
	 * The logger.
	 */
	protected Logger logger = null;
	/**
	 * The log level.
	 */
	protected int logLevel = Logging.VERBOSITY_VERY_VERBOSE;
	/**
	 * The lamp controller under test.
	 */
	protected LampController lampController = null;

	/**
	 * This is the initialisation routine.
	 */
	private void init() throws Exception
	{
		System.out.println(this.getClass().getName()+":init:Initialising loggers.");
		initLoggers();
		lampController = new LampController();
		lampController.setLampUnit(null);
	}

	/**
	 * Start two threads, one per arm.
	 * @see TestLampControllerArmThread
	 */
	private void run() throws Exception
	{
		Thread redThread = new TestLampControllerArmThread(this,FrodoSpecConfig.RED_ARM);
		redThread.start();
		Thread blueThread = new TestLampControllerArmThread(this,FrodoSpecConfig.BLUE_ARM);
		blueThread.start();
		redThread.join();
		blueThread.join();
	}

	/**
	 * Method to initialise the logger.
	 * @see #logger
	 * @see #logFilter
	 */
	protected void initLoggers()
	{
		LogHandler handler = null;
		BogstanLogFormatter blf = null;
		String loggerList[] = {"log"};

		// setup log formatter
		blf = new BogstanLogFormatter();
		blf.setDateFormat(new SimpleDateFormat("yyyy-MM-dd 'at' HH:mm:ss.SSS z"));
		// setup log handler
		handler = new ConsoleLogHandler(blf);
		handler.setLogLevel(Logging.ALL);
		// setup log filter
		//logFilter = new BitFieldLogFilter(Logging.ALL);
		// Apply handler and filter to each logger in the list
		for(int i=0;i < loggerList.length;i++)
		{
			System.out.println(this.getClass().getName()+":initLoggers:Setting up logger:"+loggerList[i]);
			logger = LogManager.getLogger(loggerList[i]);
			logger.addHandler(handler);
			//logger.setLogLevel(Logging.ALL);
			logger.setLogLevel(logLevel);
			logger.setFilter(null);
		}
	}

	/**
	 * Get the lamp controller.
	 * @return The lamp controller.
	 * @see #lampController
	 */
	protected LampController getLampController()
	{
		return lampController;
	}

	/**
	 * This routine parses arguments passed into the program.
	 * @param args Command line arguments to parse.
	 */
	private void parseArgs(String[] args)
	{
		for(int i = 0; i < args.length;i++)
		{
		}
	}

	/**
	 * Help message routine.
	 */
	private void help()
	{
		System.out.println(this.getClass().getName()+" Help:");
		System.out.println("Options are:");
	}

	/**
	 * The main routine, called when this program is executed. 
	 * @see #parseArgs
	 * @see #init
	 * @see #run
	 */
	public static void main(String[] args)
	{
		TestLampController test = new TestLampController();

		test.parseArgs(args);
		try
		{
			test.init();
			test.run();
		}
		catch (Exception e)
		{
			System.err.println("test failed:"+e);
			e.printStackTrace(System.err);
			System.exit(1);
		}
		System.exit(0);
	}

	protected class TestLampControllerArmThread extends Thread
	{
		int arm;
		TestLampController testLampController;

		public TestLampControllerArmThread(TestLampController tlc,int a)
		{
			super();
			testLampController = tlc;
			arm = a;
		}

		public void run()
		{
			try
			{
				System.out.println(this.getClass().getName()+":run:arm "+
						   FrodoSpecConstants.ARM_STRING_LIST[arm]+":Started.");
				if(arm == FrodoSpecConfig.RED_ARM)
				{
					// Ne ARC
					System.out.println(this.getClass().getName()+":run:arm "+
							   FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ":Ne ARC:setLampLock.");
					testLampController.getLampController().setLampLock(arm,"Ne",null);
					System.out.println(this.getClass().getName()+":run:arm "+
							   FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ":Ne ARC:10s exposure (sleep).");
					Thread.sleep(10);
					System.out.println(this.getClass().getName()+":run:arm "+
							   FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ":Ne ARC:clearLampLock.");
					testLampController.getLampController().clearLampLock(arm);
					// MULTRUN
					System.out.println(this.getClass().getName()+":run:arm "+
							   FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ":MULTRUN:setNoLampLock.");
					testLampController.getLampController().setNoLampLock(arm,null);
					System.out.println(this.getClass().getName()+":run:arm "+
							   FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ":MULTRUN:10s exposure (sleep).");
					Thread.sleep(10);
					System.out.println(this.getClass().getName()+":run:arm "+
							   FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ":MULTRUN:clearLampLock.");
					testLampController.getLampController().clearLampLock(arm);
					// Ne ARC
					System.out.println(this.getClass().getName()+":run:arm "+
							   FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ":Ne ARC:setLampLock.");
					testLampController.getLampController().setLampLock(arm,"Ne",null);
					System.out.println(this.getClass().getName()+":run:arm "+
							   FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ":Ne ARC:10s exposure (sleep).");
					Thread.sleep(10);
					System.out.println(this.getClass().getName()+":run:arm "+
							   FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ":Ne ARC:clearLampLock.");
					testLampController.getLampController().clearLampLock(arm);
				}
				else if(arm == FrodoSpecConfig.BLUE_ARM)
				{
					// Xe ARC
					System.out.println(this.getClass().getName()+":run:arm "+
							   FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ":Xe ARC:setLampLock.");
					testLampController.getLampController().setLampLock(arm,"Xe",null);
					System.out.println(this.getClass().getName()+":run:arm "+
							   FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ":Xe ARC:10s exposure (sleep).");
					Thread.sleep(10);
					System.out.println(this.getClass().getName()+":run:arm "+
							   FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ":Xe ARC:clearLampLock.");
					testLampController.getLampController().clearLampLock(arm);
					// MULTRUN
					System.out.println(this.getClass().getName()+":run:arm "+
							   FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ":MULTRUN:setNoLampLock.");
					testLampController.getLampController().setNoLampLock(arm,null);
					System.out.println(this.getClass().getName()+":run:arm "+
							   FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ":MULTRUN:10s exposure (sleep).");
					Thread.sleep(10);
					System.out.println(this.getClass().getName()+":run:arm "+
							   FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ":MULTRUN:clearLampLock.");
					testLampController.getLampController().clearLampLock(arm);
					// Xe ARC
					System.out.println(this.getClass().getName()+":run:arm "+
							   FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ":Xe ARC:setLampLock.");
					testLampController.getLampController().setLampLock(arm,"Xe",null);
					System.out.println(this.getClass().getName()+":run:arm "+
							   FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ":Xe ARC:10s exposure (sleep).");
					Thread.sleep(10);
					System.out.println(this.getClass().getName()+":run:arm "+
							   FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ":Xe ARC:clearLampLock.");
					testLampController.getLampController().clearLampLock(arm);
				}
				System.out.println(this.getClass().getName()+":run:arm "+
						   FrodoSpecConstants.ARM_STRING_LIST[arm]+":Finished.");
			}
			catch(Exception e)
			{
				System.err.println(this.getClass().getName()+":run:arm "+
						   FrodoSpecConstants.ARM_STRING_LIST[arm]+":Exception in run:"+e);
				e.printStackTrace();
			}
		}
	}
}
//
// $Log: not supported by cvs2svn $
//

// TestPlc.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/test/TestPlc.java,v 1.2 2009-02-05 11:40:19 cjm Exp $
package ngat.frodospec.test;

import java.lang.*;
import java.io.*;
import java.net.*;
import java.text.*;
import java.util.*;

import ngat.eip.*;
import ngat.frodospec.*;
import ngat.frodospec.test.*;
import ngat.phase2.*;
import ngat.util.*;
import ngat.util.logging.*;

/**
 * This class tests the FrodoSpec Plc class.
 * @author Chris Mottram
 * @version $Revision: 1.2 $
 */
public class TestPlc
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: TestPlc.java,v 1.2 2009-02-05 11:40:19 cjm Exp $");
	/**
	 * The properties filename to configure the Plc.
	 */
	protected String configFilename = null;
	/**
	 * The properties filename to configure the Plc.
	 */
	protected String netConfigFilename = null;
	/**
	 * The logger.
	 */
	protected Logger logger = null;
	/**
	 * The filter used to filter messages sent to the logger.
	 * @see #logger
	 */
	protected BitFieldLogFilter logFilter = null;
	/**
	 * The log level.
	 */
	protected int logLevel = Logging.VERBOSITY_VERY_VERBOSE;
	/**
	 * The Frodospec Plc instance.
	 */
	protected Plc plc = null;
	/**
	 * The FrodoSpec status instance, used for loading properties/configuring the plc instance.
	 */
	protected FrodoSpecStatus status = null;
	/**
	 * Whether we should perform the specified operation.
	 */
	protected boolean doFaultReset = false;
	/**
	 * Whether we should perform the specified operation.
	 */
	protected boolean doGetAirPressure = false;
	/**
	 * Whether we should perform the specified operation.
	 */
	protected boolean doGetFaultStatus = false;
	/**
	 * Whether we should perform the specified operation.
	 */
	protected boolean doGetMechanismStatus = false;
	/**
	 * Whether we should perform the specified operation.
	 */
	protected boolean doGetHumidity = false;
	/**
	 * Whether we should perform the specified operation.
	 */
	protected boolean doGetTemperature = false;
	/**
	 * Whether we should perform the specified operation.
	 */
	protected boolean doSetGrating = false;
	/**
	 * Which temperature probe to query.
	 */
	protected int temperatureProbe = 0;
	/**
	 * Which arm to move.
	 */
	protected int arm = FrodoSpecConfig.NO_ARM;
	/**
	 * Should the arm be in the low or high resolution setting?
	 */
	protected int resolution = FrodoSpecConfig.RESOLUTION_UNKNOWN;

	/**
	 * This is the initialisation routine.
	 */
	private void init() throws Exception
	{
		System.out.println(this.getClass().getName()+":init:Initialising status.");
		status = new FrodoSpecStatus();
		System.out.println(this.getClass().getName()+":init:Loading status properties.");
		status.load(netConfigFilename,configFilename);
		System.out.println(this.getClass().getName()+":init:Initialising loggers.");
		initLoggers();
		//logger.setLogLevel(logLevel);
		logger.log(1,"init:Creating plc instance.");
		plc = new Plc();
		plc.setLogLevel(logLevel);
		logger.log(1,"init:Initialising plc.");
		plc.init(status);
		logger.log(1,"init:Finished.");
	}

	/**
	 * This is the run method.
	 * @see #plc
	 * @see #doFaultReset
	 * @see #doGetAirPressure
	 * @see #doGetFaultStatus
	 * @see #doGetMechanismStatus
	 * @see #doGetHumidity
	 * @see #doGetTemperature
	 * @see #doSetGrating
	 * @see #temperatureProbe
	 * @see #arm
	 * @see #resolution
	 * @see ngat.frodospec.Plc#faultReset
	 * @see ngat.frodospec.Plc#getAirPressure
	 * @see ngat.frodospec.Plc#getFaultStatus
	 * @see ngat.frodospec.Plc#getMechanismStatus
	 * @see ngat.frodospec.Plc#getHumidity
	 * @see ngat.frodospec.Plc#getTemperature
	 * @see ngat.frodospec.Plc#setGrating
	 */
	private void run() throws Exception
	{
		int ivalue;
		float fvalue;

		if(doFaultReset)
			plc.faultReset();
		if(doGetAirPressure)
		{
			fvalue = plc.getAirPressure();
			logger.log(1,this.getClass().getName()+":run:The air pressure is:"+fvalue+" bar.");
		}
		if(doGetFaultStatus)
		{
			ivalue = plc.getFaultStatus();
			logger.log(1,this.getClass().getName()+":run:The fault status is:"+
				   Plc.printBits(ivalue)+".");
		}
		if(doGetMechanismStatus)
		{
			ivalue = plc.getMechanismStatus();
			logger.log(1,this.getClass().getName()+":run:The mechanism status is:"+
				   Plc.printBits(ivalue)+".");
		}
		if(doGetHumidity)
		{
			fvalue = plc.getHumidity();	
			logger.log(1,this.getClass().getName()+":run:The relative humidity is:"+fvalue+" percent.");
		}
		if(doGetTemperature)
		{
			fvalue = plc.getTemperature(temperatureProbe);
			logger.log(1,this.getClass().getName()+":run:The temperature at probe "+temperatureProbe+
				   " is:"+fvalue+" degrees centigrade.");
		}
		if(doSetGrating)
		{
			plc.setGrating(arm,resolution);
		}
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
		String loggerList[] = {"log","ngat.eip.EIPPLC"};

		// setup log formatter
		blf = new BogstanLogFormatter();
		blf.setDateFormat(new SimpleDateFormat("yyyy-MM-dd 'at' HH:mm:ss.SSS z"));
		// setup log handler
		handler = new ConsoleLogHandler(blf);
		handler.setLogLevel(Logging.ALL);
		// setup log filter
		logFilter = new BitFieldLogFilter(Logging.ALL);
		// Apply handler and filter to each logger in the list
		for(int i=0;i < loggerList.length;i++)
		{
			System.out.println(this.getClass().getName()+":initLoggers:Setting up logger:"+loggerList[i]);
			logger = LogManager.getLogger(loggerList[i]);
			logger.addHandler(handler);
			//logger.setLogLevel(Logging.ALL);
			logger.setLogLevel(logLevel);
			logger.setFilter(logFilter);
		}
	}

	/**
	 * This routine parses arguments passed into the program.
	 * @param args Command line arguments to parse.
	 * @see #help
	 * @see #doFaultReset
	 * @see #doGetAirPressure
	 * @see #doGetFaultStatus
	 * @see #doGetMechanismStatus
	 * @see #doGetHumidity
	 * @see #doGetTemperature
	 * @see #doSetGrating
	 * @see #temperatureProbe
	 * @see #arm
	 * @see #resolution
	 */
	private void parseArgs(String[] args)
	{
		doFaultReset = false;
		doGetAirPressure = false;
		doGetAirPressure = false;
		doGetFaultStatus = false;
		doGetMechanismStatus = false;
		doGetHumidity = false;
		doGetTemperature = false;
		doSetGrating = false;
		for(int i = 0; i < args.length;i++)
		{
			if(args[i].equals("-co")||args[i].equals("-config_filename"))
			{
				if((i+1)< args.length)
				{
					configFilename = args[i+1];
					i++;
				}
				else
					System.err.println("-config_filename requires a filename");
			}
			else if(args[i].equals("-fr")||args[i].equals("-fault_reset"))
			{
				doFaultReset = true;
			}
			else if(args[i].equals("-gap")||args[i].equals("-get_air_pressure"))
			{
				doGetAirPressure = true;
			}
			else if(args[i].equals("-gfs")||args[i].equals("-get_fault_status"))
			{
				doGetFaultStatus = true;
			}
			else if(args[i].equals("-gms")||args[i].equals("-get_mech_status"))
			{
				doGetMechanismStatus = true;
			}
			else if(args[i].equals("-gh")||args[i].equals("-get_humidity"))
			{
				doGetHumidity = true;
			}
			else if(args[i].equals("-gt")||args[i].equals("-get_temperature"))
			{
				if((i+1)< args.length)
				{
					temperatureProbe = Integer.parseInt(args[i+1]);
					doGetTemperature = true;
					i++;
				}
				else
					System.err.println("-get_temperature requires a probe number 0..3");
			}
			else if(args[i].equals("-nco")||args[i].equals("-net_config_filename"))
			{
				if((i+1)< args.length)
				{
					netConfigFilename = args[i+1];
					i++;
				}
				else
					System.err.println("-config_filename requires a filename");
			}
			else if(args[i].equals("-set_grating")||args[i].equals("-sg"))
			{
				if((i+2)< args.length)
				{
					if(args[i+1].equalsIgnoreCase("red"))
						arm = FrodoSpecConfig.RED_ARM;
					else if(args[i+1].equalsIgnoreCase("blue"))
						arm = FrodoSpecConfig.BLUE_ARM;
					else
					{
						System.err.println(this.getClass().getName()+
								   ":parseArguments:Illegal arm:"+args[i]+
								   ": Should be [red|blue].");
						System.exit(1);
					}
					if(args[i+2].equalsIgnoreCase("low"))
						resolution = FrodoSpecConfig.RESOLUTION_LOW;
					else if(args[i+2].equalsIgnoreCase("high"))
						resolution = FrodoSpecConfig.RESOLUTION_HIGH;
					else
					{
						System.err.println(this.getClass().getName()+
								   ":parseArguments:Illegal resolution:"+args[i]+
								   ": Should be [low|high].");
						System.exit(1);
					}
					doSetGrating = true;
					i+= 2;
				}
				else
					System.err.println("-get_temperature requires a probe number 0..3");
			}
			else if(args[i].equals("-h")||args[i].equals("-help"))
			{
				help();
				System.exit(0);
			}
			else
			{
				System.err.println(this.getClass().getName()+":Option not supported:"+args[i]);
				System.exit(1);
			}
		}
	}

	/**
	 * Help message routine.
	 */
	private void help()
	{
		System.out.println(this.getClass().getName()+" Help:");
		System.out.println("Options are:");
		System.out.println("\t-co[nfig_filename] <filename> - Specify properties filename.");
		System.out.println("\t-nco|net_config_filename <filename> - Specify properties filename.");
		System.out.println("\t-fr|fault_reset - Reset faults.");
		System.out.println("\t-gap|-get_air_pressure - Get the air pressure from the plc.");
		System.out.println("\t-gfs|-get_fault_status - Get the fault status from the plc.");
		System.out.println("\t-gms|-get_mechanism_status - Get the mechanism status from the plc.");
		System.out.println("\t-gh|-get_humidity - Get the humidity from the plc.");
		System.out.println("\t-get_temperature <[0..3]> - Get the temperature (of the specified sensor) from the plc.");
		System.out.println("\t-sg|-set_grating <red|blue> <low|high> - Set one of the gratings to either low or high resolution.");
	}

	/**
	 * The main routine, called when this program is executed. 
	 * @see #parseArgs
	 * @see #init
	 * @see #run
	 */
	public static void main(String[] args)
	{
		TestPlc test = new TestPlc();

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
}
//
// $Log: not supported by cvs2svn $
// Revision 1.1  2008/11/20 11:34:41  cjm
// Initial revision
//
//

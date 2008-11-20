// SendConfigCommand.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/test/SendConfigCommand.java,v 1.1 2008-11-20 11:34:41 cjm Exp $
package ngat.frodospec.test;

import java.lang.*;
import java.io.*;
import java.net.*;
import java.util.*;

import ngat.frodospec.test.*;
import ngat.message.ISS_INST.*;
import ngat.phase2.*;
import ngat.util.*;

/**
 * This class send a FrodoSpec camera configuration to FrodoSpec. 
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class SendConfigCommand
{
	/**
	 * The default port number to send ISS commands to.
	 */
	static final int DEFAULT_FRODOSPEC_PORT_NUMBER = 7083;
	/**
	 * The default port number for the (fake ISS) server, to get commands from the FrodoSpec from.
	 */
	static final int DEFAULT_SERVER_PORT_NUMBER = 7383;
	/**
	 * The filename of a current filter wheel property file.
	 */
	private String filename = null;
	/**
	 * The ip address to send the messages read from file to, this should be the machine the FrodoSpec is on.
	 */
	private InetAddress address = null;
	/**
	 * The port number to send commands from the file to the FrodoSpec.
	 */
	private int frodospecPortNumber = DEFAULT_FRODOSPEC_PORT_NUMBER;
	/**
	 * The port number for the server, to recieve commands from the FrodoSpec.
	 */
	private int serverPortNumber = DEFAULT_SERVER_PORT_NUMBER;
	/**
	 * The server class that listens for connections from the FrodoSpec.
	 */
	private SicfTCPServer server = null;
	/**
	 * The stream to write error messages to - defaults to System.err.
	 */
	private PrintStream errorStream = System.err;
	/**
	 * X Binning of configuration. Defaults to 1.
	 */
	private int xBin = 1;
	/**
	 * Y Binning of configuration. Defaults to 1.
	 */
	private int yBin = 1;
	/**
	 * Whether exposures taken using this configuration, should do a calibration (dark) frame
	 * before the exposure.
	 */
	private boolean calibrateBefore = false;
	/**
	 * Whether exposures taken using this configuration, should do a calibration (dark) frame
	 * after the exposure.
	 */
	private boolean calibrateAfter = false;
	/**
	 * Which arm to configure.
	 */
	private int arm = FrodoSpecConfig.NO_ARM;
	/**
	 * Which resolution.
	 */
	private int resolution = FrodoSpecConfig.RESOLUTION_UNKNOWN;

	/**
	 * This is the initialisation routine. This starts the server thread.
	 */
	private void init()
	{
		server = new SicfTCPServer(this.getClass().getName(),serverPortNumber);
		server.setController(this);
		server.start();
	}

	/**
	 * This routine creates a CONFIG command. This object
	 * has a FrodoSpecConfig phase2 object with it, this is created and it's fields initialised.
	 * @return An instance of CONFIG.
	 * @see #arm
	 * @see #resolution
	 * @see #xBin
	 * @see #yBin
	 * @see #calibrateBefore
	 * @see #calibrateAfter
	 */
	private CONFIG createConfig()
	{
		String string = null;
		CONFIG configCommand = null;
		FrodoSpecConfig frodospecConfig = null;
		FrodoSpecDetector detector = null;

		configCommand = new CONFIG("Object Id");
		frodospecConfig = new FrodoSpecConfig("Object Id");
	// detector for config
		detector = new FrodoSpecDetector();
		detector.setXBin(xBin);
		detector.setYBin(yBin);
		frodospecConfig.setDetector(0,detector);
	// arm
		frodospecConfig.setArm(arm);
	// resolution
		frodospecConfig.setResolution(resolution);
	// InstrumentConfig fields.
		frodospecConfig.setCalibrateBefore(calibrateBefore);
		frodospecConfig.setCalibrateAfter(calibrateAfter);
	// CONFIG command fields
		configCommand.setConfig(frodospecConfig);
		return configCommand;
	}

	/**
	 * This is the run routine. It creates a CONFIG object and sends it to the using a 
	 * SicfTCPClientConnectionThread, and awaiting the thread termination to signify message
	 * completion. 
	 * @return The routine returns true if the command succeeded, false if it failed.
	 * @exception Exception Thrown if an exception occurs.
	 * @see #createConfig
	 * @see SicfTCPClientConnectionThread
	 * @see #getThreadResult
	 */
	private boolean run() throws Exception
	{
		ISS_TO_INST issCommand = null;
		SicfTCPClientConnectionThread thread = null;
		boolean retval;

		issCommand = (ISS_TO_INST)(createConfig());
		if(issCommand instanceof CONFIG)
		{
			CONFIG configCommand = (CONFIG)issCommand;
			FrodoSpecConfig frodospecConfig = (FrodoSpecConfig)(configCommand.getConfig());
			System.err.println("CONFIG:"+
				"arm:"+frodospecConfig.getArm()+":"+
				"resolution:"+frodospecConfig.getResolution()+":"+
				"calibrate before:"+frodospecConfig.getCalibrateBefore()+":"+
				"calibrate after:"+frodospecConfig.getCalibrateAfter()+":"+
				frodospecConfig.getDetector(0).getXBin()+":"+
				frodospecConfig.getDetector(0).getYBin()+".");
		}
		thread = new SicfTCPClientConnectionThread(address,frodospecPortNumber,issCommand);
		thread.start();
		while(thread.isAlive())
		{
			try
			{
				thread.join();
			}
			catch(InterruptedException e)
			{
				System.err.println("run:join interrupted:"+e);
			}
		}// end while isAlive
		retval = getThreadResult(thread);
		return retval;
	}

	/**
	 * Find out the completion status of the thread and print out the final status of some variables.
	 * @param thread The Thread to print some information for.
	 * @return The routine returns true if the thread completed successfully,
	 * 	false if some error occured.
	 */
	private boolean getThreadResult(SicfTCPClientConnectionThread thread)
	{
		boolean retval;

		if(thread.getAcknowledge() == null)
			System.err.println("Acknowledge was null");
		else
			System.err.println("Acknowledge with timeToComplete:"+
				thread.getAcknowledge().getTimeToComplete());
		if(thread.getDone() == null)
		{
			System.err.println("Done was null");
			retval = false;
		}
		else
		{
			if(thread.getDone().getSuccessful())
			{
				System.err.println("Done was successful");
				retval = true;
			}
			else
			{
				System.err.println("Done returned error("+thread.getDone().getErrorNum()+
					"): "+thread.getDone().getErrorString());
				retval = false;
			}
		}
		return retval;
	}

	/**
	 * This routine parses arguments passed into SendConfigCommand.
	 * @see #frodospecPortNumber
	 * @see #address
	 * @see #arm
	 * @see #resolution
	 * @see #xBin
	 * @see #yBin
	 * @see #calibrateBefore
	 * @see #calibrateAfter
	 * @see #help
	 */
	private void parseArgs(String[] args)
	{
		for(int i = 0; i < args.length;i++)
		{
			if(args[i].equals("-a")||args[i].equals("-arm"))
			{
				if((i+1)< args.length)
				{
					if(args[i+1].equals("red"))
						arm = FrodoSpecConfig.RED_ARM;
					else if(args[i+1].equals("blue"))
						arm = FrodoSpecConfig.BLUE_ARM;
					else
						errorStream.println("Illegal arm:"+args[i+1]);
					i++;
				}
				else
					errorStream.println("arm requires an argument: red|blue");
			}
			else if(args[i].equals("-ca")||args[i].equals("-calibrate_after"))
			{
				calibrateAfter = true;
			}
			else if(args[i].equals("-cb")||args[i].equals("-calibrate_before"))
			{
				calibrateBefore = true;
			}
			else if(args[i].equals("-fp")||args[i].equals("-frodospecport"))
			{
				if((i+1)< args.length)
				{
					frodospecPortNumber = Integer.parseInt(args[i+1]);
					i++;
				}
				else
					errorStream.println("-frodospecport requires a port number");
			}
			else if(args[i].equals("-h")||args[i].equals("-help"))
			{
				help();
				System.exit(0);
			}
			else if(args[i].equals("-ip")||args[i].equals("-address"))
			{
				if((i+1)< args.length)
				{
					try
					{
						address = InetAddress.getByName(args[i+1]);
					}
					catch(UnknownHostException e)
					{
						System.err.println(this.getClass().getName()+":illegal address:"+
							args[i+1]+":"+e);
						System.exit(1);
					}
					i++;
				}
				else
					errorStream.println("-address requires an address");
			}
			else if(args[i].equals("-r")||args[i].equals("-resolution"))
			{
				if((i+1)< args.length)
				{
					if(args[i+1].equals("low"))
						resolution = FrodoSpecConfig.RESOLUTION_LOW;
					else if(args[i+1].equals("high"))
						resolution = FrodoSpecConfig.RESOLUTION_HIGH;
					else
						errorStream.println("Illegal resolution:"+args[i+1]);
					i++;
				}
				else
					errorStream.println("-resolution should  have an argument low|high");
			}
			else if(args[i].equals("-s")||args[i].equals("-serverport"))
			{
				if((i+1)< args.length)
				{
					serverPortNumber = Integer.parseInt(args[i+1]);
					i++;
				}
				else
					errorStream.println("-serverport requires a port number");
			}
			else if(args[i].equals("-x")||args[i].equals("-xBin"))
			{
				if((i+1)< args.length)
				{
					xBin = Integer.parseInt(args[i+1]);
					i++;
				}
				else
					errorStream.println("-xBin requires a valid number.");
			}
			else if(args[i].equals("-y")||args[i].equals("-yBin"))
			{
				if((i+1)< args.length)
				{
					yBin = Integer.parseInt(args[i+1]);
					i++;
				}
				else
					errorStream.println("-yBin requires a valid number.");
			}
			else
			{
				System.out.println(this.getClass().getName()+":Option not supported:"+args[i]);
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
		System.out.println("\t-a[rm] <red|blue> - Specify arm.");
		System.out.println("\t-[ca|[calibrate_after] - Do a calibration after any exposures.");
		System.out.println("\t-[cb|[calibrate_before] - Do a calibration before any exposures.");
		System.out.println("\t-[fp|frodospecport] <port number> - Port to send commands to.");
		System.out.println("\t-[ip]|[address] <address> - Address to send commands to.");
		System.out.println("\t-r[esolution] <low|high> - Specify resolution.");
		System.out.println("\t-s[erverport] <port number> - Port for FrodoSpec to send commands back.");
		System.out.println("\t-x[Bin] <binning factor> - X readout binning factor the CCD.");
		System.out.println("\t-y[Bin] <binning factor> - Y readout binning factor the CCD.");
		System.out.println("The default server port is "+DEFAULT_SERVER_PORT_NUMBER+".");
		System.out.println("The default FrodoSpec port is "+DEFAULT_FRODOSPEC_PORT_NUMBER+".");
	}

	/**
	 * The main routine, called when SendConfigCommand is executed. This initialises the object, parses
	 * it's arguments, opens the filename, runs the run routine, and then closes the file.
	 * @see #parseArgs
	 * @see #init
	 * @see #run
	 */
	public static void main(String[] args)
	{
		boolean retval;
		SendConfigCommand sicf = new SendConfigCommand();

		sicf.parseArgs(args);
		sicf.init();
		if(sicf.address == null)
		{
			System.err.println("No FrodoSpec Address Specified.");
			sicf.help();
			System.exit(1);
		}
		try
		{
			retval = sicf.run();
		}
		catch (Exception e)
		{
			retval = false;
			System.err.println("run failed:"+e);

		}
		if(retval)
			System.exit(0);
		else
			System.exit(2);
	}
}
//
// $Log: not supported by cvs2svn $
//

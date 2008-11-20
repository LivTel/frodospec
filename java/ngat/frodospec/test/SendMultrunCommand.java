// SendMultrunCommand.java 
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/test/SendMultrunCommand.java,v 1.1 2008-11-20 11:34:41 cjm Exp $
package ngat.frodospec.test;

import java.lang.*;
import java.io.*;
import java.net.*;
import java.util.*;

import ngat.frodospec.*;
import ngat.message.ISS_INST.*;
import ngat.phase2.*;
import ngat.util.*;

/**
 * This class send a MULTRUN to FrodoSpec. 
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class SendMultrunCommand
{
	/**
	 * The default port number to send ISS commands to.
	 */
	static final int DEFAULT_FRODOSPEC_PORT_NUMBER = 7083;
	/**
	 * The default port number for the server, to get commands from the FRODOSPEC from.
	 */
	static final int DEFAULT_SERVER_PORT_NUMBER = 7383;
	/**
	 * The ip address to send the messages read from file to, this should be the machine the FRODOSPEC is on.
	 */
	private InetAddress address = null;
	/**
	 * The port number to send commands from the file to the FRODOSPEC.
	 */
	private int frodospecPortNumber = DEFAULT_FRODOSPEC_PORT_NUMBER;
	/**
	 * The port number for the server, to recieve commands from the FRODOSPEC.
	 */
	private int serverPortNumber = DEFAULT_SERVER_PORT_NUMBER;
	/**
	 * The server class that listens for connections from the FRODOSPEC.
	 */
	private SicfTCPServer server = null;
	/**
	 * The stream to write error messages to - defaults to System.err.
	 */
	private PrintStream errorStream = System.err;
	/**
	 * Exposure length. Defaults to zero, which should cause MULTRUN to return an error.
	 */
	private int exposureLength = 0;
	/**
	 * Number of exposures for the MULTRUN to take. 
	 * Defaults to zero, which should cause MULTRUN to return an error.
	 */
	private int exposureCount = 0;
	/**
	 * Whether this MULTRUN has standard flags set (is of a standard source). Defaults to false.
	 */
	private boolean standard = false;
	/**
	 * Whether to send the generated filenames to the DpRt. Defaults to false.
	 */
	private boolean pipelineProcess = false;
	/**
	 * Which arm to expose.
	 */
	private int arm = FrodoSpecConfig.NO_ARM;

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
	 * This routine creates a MULTRUN command. 
	 * @return An instance of MULTRUN.
	 * @see #arm
	 * @see #exposureLength
	 * @see #exposureCount
	 * @see #standard
	 * @see #pipelineProcess
	 */
	private FRODOSPEC_MULTRUN createMultrun()
	{
		String string = null;
		FRODOSPEC_MULTRUN multrunCommand = null;

		multrunCommand = new FRODOSPEC_MULTRUN("SendMultrunCommand");
		multrunCommand.setArm(arm);
		multrunCommand.setExposureTime(exposureLength);
		multrunCommand.setNumberExposures(exposureCount);
		multrunCommand.setStandard(standard);
		multrunCommand.setPipelineProcess(pipelineProcess);
		return multrunCommand;
	}

	/**
	 * This is the run routine. It creates a MULTRUN object and sends it to the using a 
	 * SicfTCPClientConnectionThread, and awaiting the thread termination to signify message
	 * completion. 
	 * @return The routine returns true if the command succeeded, false if it failed.
	 * @exception Exception Thrown if an exception occurs.
	 * @see #createMultrun
	 * @see SicfTCPClientConnectionThread
	 * @see #getThreadResult
	 */
	private boolean run() throws Exception
	{
		ISS_TO_INST issCommand = null;
		SicfTCPClientConnectionThread thread = null;
		boolean retval;

		issCommand = (ISS_TO_INST)(createMultrun());
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
			System.out.println("Done was null");
			retval = false;
		}
		else
		{
			if(thread.getDone().getSuccessful())
			{
				System.out.println("Done was successful");
				if(thread.getDone() instanceof EXPOSE_DONE)
				{
					System.out.println("\tFilename:"+
						((EXPOSE_DONE)(thread.getDone())).getFilename());
				}
				retval = true;
			}
			else
			{
				System.out.println("Done returned error("+thread.getDone().getErrorNum()+
					"): "+thread.getDone().getErrorString());
				retval = false;
			}
		}
		return retval;
	}

	/**
	 * This routine parses arguments passed into SendMultrunCommand.
	 * @see #arm
	 * @see #exposureLength
	 * @see #exposureCount
	 * @see #standard
	 * @see #pipelineProcess
	 * @see #frodospecPortNumber
	 * @see #address
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
						errorStream.println("-arm has illegal arm:"+args[i+1]);
					i++;
				}
				else
					errorStream.println("-arm requires a parameter:red|blue");
			}
			else if(args[i].equals("-f")||args[i].equals("-frodospecport"))
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
					}
					i++;
				}
				else
					errorStream.println("-address requires an address");
			}
			else if(args[i].equals("-l")||args[i].equals("-exposureLength"))
			{
				if((i+1)< args.length)
				{
					exposureLength = Integer.parseInt(args[i+1]);
					i++;
				}
				else
					errorStream.println("-exposureLength requires an argument.");
			}
			else if(args[i].equals("-n")||args[i].equals("-exposureCount"))
			{
				if((i+1)< args.length)
				{
					exposureCount = Integer.parseInt(args[i+1]);
					i++;
				}
				else
					errorStream.println("-exposureCount requires an argument.");
			}
			else if(args[i].equals("-p")||args[i].equals("-pipelineProcess"))
			{
					pipelineProcess = true;
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
			else if(args[i].equals("-t")||args[i].equals("-standard"))
			{
				standard = true;
			}
			else
				System.out.println(this.getClass().getName()+":Option not supported:"+args[i]);
		}
	}

	/**
	 * Help message routine.
	 */
	private void help()
	{
		System.out.println(this.getClass().getName()+" Help:");
		System.out.println("Options are:");
		System.out.println("\t-a[rm] <red|blue> - Which arm.");
		System.out.println("\t-f[rodospecport] <port number> - Port to send commands to.");
		System.out.println("\t-[ip]|[address] <address> - Address to send commands to.");
		System.out.println("\t-[l]|[exposureLength] <time in millis> - Specify exposure length.");
		System.out.println("\t-[n]|[exposureCount] <number> - Specify number of exposures.");
		System.out.println("\t-s[erverport] <port number> - Port for the FRODOSPEC to send commands back.");
		System.out.println("\t-p[ipelineProcess] - Send frames to pipeline process.");
		System.out.println("\t-[t]|[standard] - Set standard parameters.");
		System.out.println("The default server port is "+DEFAULT_SERVER_PORT_NUMBER+".");
		System.out.println("The default FRODOSPEC port is "+DEFAULT_FRODOSPEC_PORT_NUMBER+".");
	}

	/**
	 * The main routine, called when SendMultrunCommand is executed. This initialises the object, parses
	 * it's arguments, opens the filename, runs the run routine, and then closes the file.
	 * @see #parseArgs
	 * @see #init
	 * @see #run
	 */
	public static void main(String[] args)
	{
		boolean retval;
		SendMultrunCommand smc = new SendMultrunCommand();

		smc.parseArgs(args);
		smc.init();
		if(smc.address == null)
		{
			System.err.println("No Ccs Address Specified.");
			smc.help();
			System.exit(1);
		}
		try
		{
			retval = smc.run();
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

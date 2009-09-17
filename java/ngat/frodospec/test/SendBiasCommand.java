// SendBiasCommand.java 
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/test/SendBiasCommand.java,v 1.1 2009-09-17 09:47:52 cjm Exp $
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
public class SendBiasCommand
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
	 * This routine creates a FRODOSPEC_BIAS command. 
	 * @return An instance of FRODOSPEC_BIAS.
	 * @see #arm
	 */
	private FRODOSPEC_BIAS createBias()
	{
		String string = null;
		FRODOSPEC_BIAS biasCommand = null;

		biasCommand = new FRODOSPEC_BIAS("SendBiasCommand");
		biasCommand.setArm(arm);
		return biasCommand;
	}

	/**
	 * This is the run routine. It creates a FRODOSPEC_BIAS object and sends it to the using a 
	 * SicfTCPClientConnectionThread, and awaiting the thread termination to signify message
	 * completion. 
	 * @return The routine returns true if the command succeeded, false if it failed.
	 * @exception Exception Thrown if an exception occurs.
	 * @see #createBias
	 * @see SicfTCPClientConnectionThread
	 * @see #getThreadResult
	 */
	private boolean run() throws Exception
	{
		ISS_TO_INST issCommand = null;
		SicfTCPClientConnectionThread thread = null;
		boolean retval;

		issCommand = (ISS_TO_INST)(createBias());
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
				if(thread.getDone() instanceof FRODOSPEC_BIAS_DONE)
				{
					System.out.println("\tFilename:"+
						((FRODOSPEC_BIAS_DONE)(thread.getDone())).getFilename());
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
	 * This routine parses arguments passed into SendBiasCommand.
	 * @see #arm
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
		System.out.println("\t-s[erverport] <port number> - Port for the FRODOSPEC to send commands back.");
		System.out.println("The default server port is "+DEFAULT_SERVER_PORT_NUMBER+".");
		System.out.println("The default FRODOSPEC port is "+DEFAULT_FRODOSPEC_PORT_NUMBER+".");
	}

	/**
	 * The main routine, called when SendBiasCommand is executed. This initialises the object, parses
	 * it's arguments, opens the filename, runs the run routine, and then closes the file.
	 * @see #parseArgs
	 * @see #init
	 * @see #run
	 */
	public static void main(String[] args)
	{
		boolean retval;
		SendBiasCommand smc = new SendBiasCommand();

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

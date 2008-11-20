// Move.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/newmark/test/Move.java,v 1.1 2008-11-20 11:34:37 cjm Exp $
package ngat.frodospec.newmark.test;

import java.lang.*;
import java.io.*;
import java.net.*;
import java.util.*;

import ngat.serial.arcomess.*;
import ngat.frodospec.newmark.*;

/**
 * This class tests the Frodospec Newmark library, by homing the filter slide using a Newmark controller
 * via an ArcomESS connection.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class Move
{
	/**
	 * Revision Control System id string, showing the version of the Class
	 */
	public final static String RCSID = new String("$Id: Move.java,v 1.1 2008-11-20 11:34:37 cjm Exp $");
	/**
	 * The default log level.
	 * @see ngat.serial.arcomess.ArcomESS#LOG_BIT_SERIAL
	 * @see ngat.serial.arcomess.ArcomESS#LOG_BIT_SOCKET
	 * @see ngat.frodospec.newmark.Newmark#LOG_BIT_COMMAND
	 */
	public final static int DEFAULT_LOG_LEVEL = (ArcomESS.LOG_BIT_SERIAL|ArcomESS.LOG_BIT_SOCKET|
						     Newmark.LOG_BIT_COMMAND);
	/**
	 * Which type of device to try to connect to.
	 * @see ngat.serial.arcomess.ArcomESS#INTERFACE_DEVICE_NONE
	 */
	protected int deviceId = ArcomESS.INTERFACE_DEVICE_NONE;
	/**
	 * Name of the device to connect to.
	 * If deviceId is INTERFACE_DEVICE_SERIAL, the serial device i.e. /dev/ttyS0.
	 * If deviceId is INTERFACE_DEVICE_SOCKET, the socket hostname/IP address, 
	 * i.e. 150.204.240.115 / frodospecserialports.
	 */
	protected String deviceName = null;
	/**
	 * The port number of the device to connect to. Only used when deviceId is INTERFACE_DEVICE_SOCKET.
	 */
	protected int portNumber = 0;
	/**
	 * Log level to use for ArcomESS and Newmark logging.
	 * @see #DEFAULT_LOG_LEVEL
	 */
	protected int logLevel = DEFAULT_LOG_LEVEL;
	/**
	 * The new absolute position to move the mechanism controlled by the Newmark motion controller to,
	 * Usually in mm, for the FrodoSpec filter slide valid positions are around -25 to 175 mm.
	 */
	protected double position = 0;

	/**
	 * Run method.
	 * <ul>
	 * <li>Creates an ArcomESS instance.
	 * <li>Calls interfaceOpen with deviceId, deviceName, portNumber to connect to the Newmark controller
	 *     via the Arcom ESS.
	 * <li>Creates a Newmark instance.
	 * <li>Calls move to move the Newmark device.
	 * <li>Finally, calls ArcomESS's interfaceClose to close the connection.
	 * </ul>
	 * @exception ArcomESSNativeException Thrown if the Arcom ESS fails.
	 * @exception NewmarkNativeException Thrown if the move fails.
	 * @see #deviceName
	 * @see #deviceId
	 * @see #portNumber
	 * @see #logLevel
	 * @see #position
	 * @see ngat.serial.arcomess.ArcomESS
	 * @see ngat.frodospec.newmark.Newmark
	 * @see ngat.serial.arcomess.ArcomESS#interfaceOpen
	 * @see ngat.serial.arcomess.ArcomESS#interfaceClose
	 * @see ngat.frodospec.newmark.Newmark#move
	 */
	public void run() throws ArcomESSNativeException, NewmarkNativeException
	{
		ArcomESS arcomESS = null;
		Newmark newmark = null;

		arcomESS = new ArcomESS();
		arcomESS.setLogFilterLevel(logLevel);
		newmark = new Newmark(arcomESS);
		newmark.setLogFilterLevel(logLevel);
		arcomESS.interfaceOpen(deviceId,deviceName,portNumber);
		try
		{
			newmark.move(position);
		}
		finally
		{
			arcomESS.interfaceClose();
		}
	}

	/**
	 * Parse command line arguments.
	 * @param args The command line argument list.
	 * @see #deviceName
	 * @see #deviceId
	 * @see #portNumber
	 * @see #help
	 */
	private void parseArgs(String[] args)
	{
		for(int i = 0; i < args.length;i++)
		{

			if(args[i].equals("-h")||args[i].equals("-help"))
			{
				help();
				System.exit(0);
			}
			else if(args[i].equals("-log_level")||args[i].equals("-l"))
			{
				if((i+1)< args.length)
				{
					logLevel = Integer.parseInt(args[i+1]);

					i += 1;
				}
				else
				{
					System.err.println("-log_level should have an integer argument.");
					System.exit(1);
				}
			}
			else if(args[i].equals("-position")||args[i].equals("-p"))
			{
				if((i+1)< args.length)
				{
					position = Double.parseDouble(args[i+1]);
					i += 1;
				}
				else
				{
					System.err.println("-log_level should have an integer argument.");
					System.exit(1);
				}
			}
			else if(args[i].equals("-serial")||args[i].equals("-ser"))
			{
				if((i+1)< args.length)
				{
					deviceName = args[i+1];
					deviceId = ArcomESS.INTERFACE_DEVICE_SERIAL;
					i++;
				}
				else
				{
					System.err.println("-serial should  have a device argument.");
					System.exit(1);
				}
			}
			else if(args[i].equals("-socket")||args[i].equals("-sock"))
			{
				if((i+2)< args.length)
				{
					deviceName = args[i+1];
					portNumber = Integer.parseInt(args[i+2]);
					deviceId = ArcomESS.INTERFACE_DEVICE_SOCKET;
					i += 2;
				}
				else
				{
					System.err.println("-serial should have 2 arguments : <device> <port number>.");
					System.exit(1);
				}
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
		System.out.println("\t-help");
		System.out.println("\t-serial <device name>");
		System.out.println("\t-socket <hostname> <port number>");
		System.out.println("\t-log_level <n>");
		System.out.println("\t-position <mm> (-25 to 175 usually a good range).");
	}

	/**
	 * Main method - entry point for the test program.
	 * Example run: 
	 * <pre>java ngat.frodospec.newmark.test.Move -socket frodospec1serialports 3040 -position 100</pre>
	 * <ul>
	 * <li>Calls parseArgs to parse the command line arguments.
	 * <li>Calls run method to move the Newmark slide.
	 * </ul>
	 * @param args Command line arguments.
	 * @see #deviceName
	 * @see #deviceId
	 * @see #portNumber
	 * @see #position
	 * @see #parseArgs
	 * @see #run
	 */
	public static void main(String[] args)
	{
		Move m = new Move();

		m.parseArgs(args);
		try
		{
			m.run();
		}
		catch(Exception e)
		{
			System.err.println("Move run failed:"+e);
			e.printStackTrace();
			System.exit(1);
		}
	}
}
//
// $Log: not supported by cvs2svn $
//

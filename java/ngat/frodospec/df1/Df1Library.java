// Df1Library.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/df1/Df1Library.java,v 1.1 2023-03-21 14:41:53 cjm Exp $
package ngat.frodospec.df1;

import java.lang.*;
import ngat.util.logging.*;

/**
 * This class supports an interface to a Micrologix 1100 PLC, using thr DF1 protocol either over a 
 * serial link, or over a socket conenction via an Arcom ESS.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class Df1Library
{
	/**
	 * Revision Control System id string, showing the version of the Class
	 */
	public final static String RCSID = new String("$Id: Df1Library.java,v 1.1 2023-03-21 14:41:53 cjm Exp $");
// df1_general.h
	/* These constants should be the same as those in df1_general.h */
	/**
	 * Logging filter bit.
	 * @see #setLogFilterLevel
	 */
	public final static int LOG_BIT_SERIAL       = (1<<16);
	/**
	 * Logging filter bit.
	 * @see #setLogFilterLevel
	 */
	public final static int LOG_BIT_SOCKET       = (1<<17);
	/**
	 * Logging filter bit.
	 * @see #setLogFilterLevel
	 */
	public final static int LOG_BIT_DF1          = (1<<18);
	/**
	 * Logging filter bit.
	 * @see #setLogFilterLevel
	 */
	public final static int LOG_BIT_DF1_READ_WRITE = (1<<19);
// df1_interface.h
	/* These constants should be the same as those in df1_interface.h */
	/**
	 * Interface device number, showing that commands will currently be sent nowhere.
	 * @see #interfaceOpen
	 */
	public final static int INTERFACE_DEVICE_NONE = 		0;
	/**
	 * Interface device number, showing that commands will currently be sent over a serial connection.
	 * @see #interfaceOpen
	 */
	public final static int INTERFACE_DEVICE_SERIAL = 		1;
	/**
	 * Interface device number, showing that commands will currently be sent over a socket (Arcom ESS) connection.
	 * @see #interfaceOpen
	 */
	public final static int INTERFACE_DEVICE_SOCKET = 		2;

//internal C layer initialisation
	/**
	 * Native method that allows the JNI layer to store a reference to this Class's logger.
	 * @param logger The logger for this class.
	 */
	private native void initialiseLoggerReference(Logger logger);
	/**
	 * Native method that allows the JNI layer to release the global reference to this Class's logger.
	 */
	private native void finaliseLoggerReference();
// df1_general.h
	/**
	 * Native wrapper to libfrodospec_df1 routine that changes the log Filter Level.
	 */
	private native void Df1_Set_Log_Filter_Level(int level);
// df1_interface.h
	/**
	 * Native wrapper to libfrodospec_df1 routine that create a Df1 library handle and associates it with
	 * this instance of the Df1Library class.
	 * @exception Df1LibraryNativeException This method throws a Df1LibraryNativeException if it failed.
	 */
	private native void Df1_Interface_Handle_Create() throws Df1LibraryNativeException;
	/**
	 * Native wrapper to libfrodospec_df1 routine that destroys the Df1 library handle associated with
	 * this instance of the Df1Library class.
	 * @exception Df1LibraryNativeException This method throws a Df1LibraryNativeException if it failed.
	 */
	private native void Df1_Interface_Handle_Destroy() throws Df1LibraryNativeException;
	/**
	 * Native wrapper to libfrodospec_df1 routine that opens the selected interface device.
	 * @param interfaceDevice The interface device to use to communicate with the PLC.
	 * 	One of: INTERFACE_DEVICE_NONE, INTERFACE_DEVICE_SERIAL, INTERFACE_DEVICE_SOCKET.
	 * @param deviceName The name of the device. For devices of type SERIAL, this will be something like
	 *        "/dev/ttyS0". For devices of type SOCKET, this should be a valid IP address or hostname 
	 *        e.g. 150.204.240.115 or frodospec1serialports.
	 * @param portNumber The port number of the SOCKET device - not used for serial devices.
	 * @see #INTERFACE_DEVICE_NONE
	 * @see #INTERFACE_DEVICE_SERIAL
	 * @see #INTERFACE_DEVICE_DOCKET
	 * @exception Df1LibraryNativeException This method throws a Df1LibraryNativeException if it failed.
	 */
	private native void Df1_Interface_Open(int interfaceDevice,String deviceName,int portNumber)
		throws Df1LibraryNativeException;
	/**
	 * Native wrapper to libfrodospec_df1 routine that closes the selected interface device.
	 * @exception Df1LibraryNativeException This method throws a Df1LibraryNativeException if it failed.
	 */
	private native void Df1_Interface_Close() throws Df1LibraryNativeException;
// df1_read_write.h
	/**
	 * Native wrapper to libfrodospec_df1 routine that writes a boolean value to the PLC.
	 * @exception Df1LibraryNativeException This method throws a Df1LibraryNativeException if it failed.
	 */
	private native void Df1_Write_Boolean(String plcAddress,boolean value) throws Df1LibraryNativeException;
	/**
	 * Native wrapper to libfrodospec_df1 routine that reads a boolean value from the PLC.
	 * @exception Df1LibraryNativeException This method throws a Df1LibraryNativeException if it failed.
	 */
	private native boolean Df1_Read_Boolean(String plcAddress) throws Df1LibraryNativeException;
	/**
	 * Native wrapper to libfrodospec_df1 routine that writes an integer value to the PLC.
	 * @exception Df1LibraryNativeException This method throws a Df1LibraryNativeException if it failed.
	 */
	private native void Df1_Write_Integer(String plcAddress,short value) throws Df1LibraryNativeException;
	/**
	 * Native wrapper to libfrodospec_df1 routine that reads an integer value from the PLC.
	 * @exception Df1LibraryNativeException This method throws a Df1LibraryNativeException if it failed.
	 */
	private native short Df1_Read_Integer(String plcAddress) throws Df1LibraryNativeException;
	/**
	 * Native wrapper to libfrodospec_df1 routine that writes a float value to the PLC.
	 * @exception Df1LibraryNativeException This method throws a Df1LibraryNativeException if it failed.
	 */
	private native void Df1_Write_Float(String plcAddress,float value) throws Df1LibraryNativeException;
	/**
	 * Native wrapper to libfrodospec_df1 routine that reads a float value from the PLC.
	 * @exception Df1LibraryNativeException This method throws a Df1LibraryNativeException if it failed.
	 */
	private native float Df1_Read_Float(String plcAddress) throws Df1LibraryNativeException;

// per instance variables
	/**
	 * The logger to log messages to.
	 */
	protected Logger logger = null;

// static code block
	/**
	 * Static code to load libfrodospec_df1, the shared C library that implements an interface to the
	 * PLC.
	 */
	static
	{
		System.loadLibrary("frodospec_df1");
	}

// constructor
	/**
	 * Constructor. Constructs the logger, and sets the C layers reference to it.
	 * Calls Df1_Interface_Handle_Create to create a Df1 library handle and associate in the handle map
	 * with this new instance of Df1Library.
	 * @exception Df1LibraryNativeException Thrown if the handle creation / mapping fails.
	 * @see #logger
	 * @see #initialiseLoggerReference
	 * @see #Df1_Interface_Handle_Create
	 */
	public Df1Library() throws Df1LibraryNativeException
	{
		super();
		logger = LogManager.getLogger(this);
		initialiseLoggerReference(logger);
		Df1_Interface_Handle_Create();
	}

	/**
	 * Finalize method for this class, delete JNI global references.
	 * Calls Df1_Interface_Handle_Destroy to remove the Df1 Library handle from the handle map for this
	 * instance of the Df1Library class.
	 * @exception Df1LibraryNativeException Thrown if the handle destruction fails.
	 * @see #finaliseLoggerReference
	 * @see #Df1_Interface_Handle_Destroy
	 */
	protected void finalize() throws Throwable
	{
		super.finalize();
		Df1_Interface_Handle_Destroy();
		finaliseLoggerReference();
	}

// df1_general.h
	/**
	 * Routine that changes the libfrodospec_df1 logging filter level.
	 * @param level The logging filter level.
	 * @see #Df1_Set_Log_Filter_Level
	 * @see #LOG_BIT_SERIAL
	 * @see #LOG_BIT_SOCKET
	 * @see #LOG_BIT_DF1
	 * @see #LOG_BIT_DF1_READ_WRITE
	 */
	public void setLogFilterLevel(int level)
	{
		Df1_Set_Log_Filter_Level(level);
	}

// df1_interface.h
	/**
	 * Routine to open the interface. 
	 * @param interfaceDevice The interface device to use to communicate with the PLC.
	 * 	One of: INTERFACE_DEVICE_NONE, INTERFACE_DEVICE_SERIAL, INTERFACE_DEVICE_SOCKET.
	 * @param deviceName The name of the device. For devices of type SERIAL, this will be something like
	 *        "/dev/ttyS0". For devices of type SOCKET, this should be a valid IP adddress or hostname 
	 *        (e.g. 150.204.240.115 ot frodospec1serialports).
	 * @param portNumber The port number of the SOCKET device - not used for serial devices.
	 * @exception Df1LibraryNativeException This method throws a Df1LibraryNativeException if the device could
	 * 	not be opened.
	 * @see #INTERFACE_DEVICE_NONE
	 * @see #INTERFACE_DEVICE_SERIAL
	 * @see #INTERFACE_DEVICE_SOCKET
	 * @see #interfaceClose
	 * @see #Df1_Interface_Open
	 */
	public void interfaceOpen(int interfaceDevice,String deviceName,int portNumber) 
		throws Df1LibraryNativeException
	{
		Df1_Interface_Open(interfaceDevice,deviceName,portNumber);
	}

	/**
	 * Routine to close the interface opened with <b>interfaceOpen</b>.
	 * @exception Df1LibraryNativeException This method throws a Df1LibraryNativeException if the device could
	 * 	not be closed.
	 * @see #interfaceOpen
	 * @see #Df1_Interface_Close
	 */
	public void interfaceClose() throws Df1LibraryNativeException
	{
		Df1_Interface_Close();
	}

	/**
	 * Routine to get an interface device number from a string representation of the device. Used for
	 * getting a device number from a string in a properties file.
	 * @param s A string representing a device number, one of:
	 * 	<ul>
	 * 	<li>INTERFACE_DEVICE_NONE,
	 * 	<li>INTERFACE_DEVICE_SERIAL,
	 * 	<li>INTERFACE_DEVICE_SOCKET.
	 * 	</ul>.
	 * @return An interface device number, one of:
	 * 	<ul>
	 * 	<li>INTERFACE_DEVICE_NONE,
	 * 	<li>INTERFACE_DEVICE_SERIAL,
	 * 	<li>INTERFACE_DEVICE_SOCKET.
	 * 	</ul>. 
	 * @exception Df1LibraryFormatException If the string was not an accepted value an exception is thrown.
	 * @see #INTERFACE_DEVICE_NONE
	 * @see #INTERFACE_DEVICE_SERIAL
	 * @see #INTERFACE_DEVICE_SOCKET
	 */
	public static int interfaceDeviceFromString(String s) throws Df1LibraryFormatException
	{
		if(s.equals("INTERFACE_DEVICE_NONE"))
			return INTERFACE_DEVICE_NONE;
		if(s.equals("INTERFACE_DEVICE_SERIAL"))
			return INTERFACE_DEVICE_SERIAL;
		if(s.equals("INTERFACE_DEVICE_SOCKET"))
			return INTERFACE_DEVICE_SOCKET;
		throw new Df1LibraryFormatException("ngat.frodospec.df1.Df1Library","interfaceDeviceFromString",s);
	}

// df1_read_write.h
	/**
	 * Method to write a boolean value to a PLC address.
	 * @param plcAddress The memory location to write the boolean to, in the form: B3:0/5 , T4:0.dn. 
	 * @param value The boolean value (true or false).
	 * @exception Df1LibraryNativeException This method throws a Df1LibraryNativeException if the write failed.
	 * @see #Df1_Write_Boolean
	 */
	public void setBoolean(String plcAddress,boolean value) throws Df1LibraryNativeException
	{
		Df1_Write_Boolean(plcAddress,value);
	}

	/**
	 * Method to read a boolean value from a PLC address.
	 * @param plcAddress The memory location to read the boolean from, in the form: B3:0/5 , T4:0.dn.
	 * @return The boolean value at that PLC address (true or false).
	 * @exception Df1LibraryNativeException This method throws a Df1LibraryNativeException if the read failed.
	 * @see #Df1_Read_Boolean
	 */
	public boolean getBoolean(String plcAddress)  throws Df1LibraryNativeException
	{
		return Df1_Read_Boolean(plcAddress);
	}

	/**
	 * Method to write an integer value to a PLC address.
	 * @param plcAddress The memory location to write the integer to, in the form: N7:1 , T4:0.PRE. 
	 * @param value The integer value. This should be a positive short.
	 * @exception Df1LibraryNativeException This method throws a Df1LibraryNativeException if the write failed.
	 * @see #Df1_Write_Integer
	 */
	public void setInteger(String plcAddress,short value) throws Df1LibraryNativeException
	{
		Df1_Write_Integer(plcAddress,value);
	}

	/**
	 * Method to read an integer value from a PLC address.
	 * @param plcAddress The memory location to read the integer from, in the form: N7:1 , T4:0.PRE.
	 * @return The integer value at that PLC address.
	 * @exception Df1LibraryNativeException This method throws a Df1LibraryNativeException if the read failed.
	 * @see #Df1_Read_Integer
	 */
	public short getInteger(String plcAddress)  throws Df1LibraryNativeException
	{
		return Df1_Read_Integer(plcAddress);
	}

	/**
	 * Method to write a float value to a PLC address.
	 * @param plcAddress The memory location to write the float to, in the form: F8:5 , F8:2. 
	 * @param value The float value.
	 * @exception Df1LibraryNativeException This method throws a Df1LibraryNativeException if the write failed.
	 * @see #Df1_Write_Float
	 */
	public void setFloat(String plcAddress,float value) throws Df1LibraryNativeException
	{
		Df1_Write_Float(plcAddress,value);
	}

	/**
	 * Method to read a float value from a PLC address.
	 * @param plcAddress The memory location to read the integer from, in the form: F8:5 , F8:2.
	 * @return The float value at that PLC address.
	 * @exception Df1LibraryNativeException This method throws a Df1LibraryNativeException if the read failed.
	 * @see #Df1_Read_Float
	 */
	public float getFloat(String plcAddress)  throws Df1LibraryNativeException
	{
		return Df1_Read_Float(plcAddress);
	}
}
 
//
// $Log: not supported by cvs2svn $
//

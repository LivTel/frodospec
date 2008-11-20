// Newmark.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/newmark/Newmark.java,v 1.1 2008-11-20 11:34:35 cjm Exp $
package ngat.frodospec.newmark;

import java.lang.*;
import ngat.util.logging.*;
import ngat.serial.arcomess.*;

/**
 * An instance of this class respresents a Newmark Motion Controller. It supports an Arcom
 * ESS interface (via an instance of the ngat.serial.arcomess.ArcomESS class), which
 * allows communication with the motion controller's serial interface either directly or via
 * a Arcom ESS (Ethernet Serial Server).
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class Newmark
{
	/**
	 * Revision Control System id string, showing the version of the Class
	 */
	public final static String RCSID = new String("$Id: Newmark.java,v 1.1 2008-11-20 11:34:35 cjm Exp $");
// newmark_general.h
	/* These constants should be the same as those in newmark_general.h */
	/**
	 * Logging filter bit.
	 * @see #setLogFilterLevel
	 */
	public final static int LOG_BIT_COMMAND       = (1<<29);
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
	/**
	 * Native method that allows the JNI layer to store in a handle map, this reference, the asssociated
	 * ArcomESS reference and it's associated native Arcom ESS interface handle.
	 * @param handle The handle to initialise.
	 */
	private native void initialiseHandle(ArcomESS handle);
	/**
	 * Native method that allows the JNI layer to release this instance from the handle map.
	 */
	private native void finaliseHandle();
// newmark_general.h
	/**
	 * Native wrapper to libfrodospec_newmark routine that changes the log Filter Level.
	 */
	private native void Newmark_Set_Log_Filter_Level(int level);
// newmark_command.h
	/**
	 * Native wrapper to libfrodospec_newmark routine that homes the mechanism.
	 */
	private native void Newmark_Command_Home() throws NewmarkNativeException;
	/**
	 * Native wrapper to libfrodospec_newmark routine that moves the mechanism.
	 */
	private native void Newmark_Command_Move(double position) throws NewmarkNativeException;
	/**
	 * Native wrapper to libfrodospec_newmark routine that aborts movement.
	 */
	private native void Newmark_Command_Abort_Move() throws NewmarkNativeException;
	/**
	 * Native wrapper to libfrodospec_newmark routine that gets the current mechanism position.
	 */
	private native double Newmark_Command_Position_Get() throws NewmarkNativeException;
	/**
	 * Native wrapper to libfrodospec_newmark routine that sets the 'in-position' tolerance.
	 */
	private native void Newmark_Command_Position_Tolerance_Set(double mm) throws NewmarkNativeException;

// per instance variables
	/**
	 * The logger to log messages to.
	 */
	protected Logger logger = null;

// static code block
	/**
	 * Static code to load libfrodospec_newmark, the shared C library that implements an interface to the
	 * Newmark motion controller.
	 */
	static
	{
		System.loadLibrary("frodospec_newmark");
	}

// constructor
	/**
	 * Constructor. Constructs the logger, and sets the C layers reference to it.
	 * Calls initialiseHandle to associate in the handle map the ArcomESS instance
	 * with this new instance of Newmark, and the associated ArcomESS interface handle.
	 * @param handle An instance of ArcomESS describing the interface to the Newmark motion controller.
	 * @exception NewmarkNativeException Thrown if the handle creation / mapping fails.
	 * @see #logger
	 * @see #initialiseLoggerReference
	 * @see #initialiseHandle
	 */
	public Newmark(ArcomESS handle) throws NewmarkNativeException
	{
		super();
		logger = LogManager.getLogger(this);
		initialiseLoggerReference(logger);
		initialiseHandle(handle);
	}

	/**
	 * Finalize method for this class, delete JNI global references.
	 * Calls finaliseHandle to remove the handle from the handle map for this
	 * instance of the Newmark class.
	 * @exception Df1LibraryNativeException Thrown if the handle destruction fails.
	 * @see #finaliseLoggerReference
	 * @see #finaliseHandle
	 */
	protected void finalize() throws Throwable
	{
		super.finalize();
		finaliseHandle();
		finaliseLoggerReference();
	}

// newmark_general.h
	/**
	 * Routine that changes the libfrodospec_newmark logging filter level.
	 * @param level The logging filter level.
	 * @see #Newmark_Set_Log_Filter_Level
	 * @see #LOG_BIT_COMMAND
	 */
	public void setLogFilterLevel(int level)
	{
		Newmark_Set_Log_Filter_Level(level);
	}

// newmark_command.h
	/**
	 * Method to home the mechanism.
	 * @exception NewmarkNativeException This method throws a NewmarkNativeException if it failed.
	 * @see #Newmark_Command_Home
	 */
	public void home() throws NewmarkNativeException
	{
		Newmark_Command_Home();
	}

	/**
	 * Method to move the mechanism.
	 * @param position The new position of the mechanism in mm. Valid range around -25 to 175 for our mechanism.
	 * @exception NewmarkNativeException This method throws a NewmarkNativeException if it failed.
	 * @see #Newmark_Command_Move
	 */
	public void move(double position) throws NewmarkNativeException
	{
		Newmark_Command_Move(position);
	}

	/**
	 * Method to abort any ongoing movement.
	 * @exception NewmarkNativeException This method throws a NewmarkNativeException if it failed.
	 * @see #Newmark_Command_Abort_Move
	 */
	public void abort() throws NewmarkNativeException
	{
		Newmark_Command_Abort_Move();
	}

	/**
	 * Method to get the current position of the mechanism.
	 * @return The current position of the mechanism in mm. Valid range around -25 to 175 for our mechanism.
	 * @exception NewmarkNativeException This method throws a NewmarkNativeException if it failed.
	 * @see #Newmark_Command_Position_Get
	 */
	public double getPosition() throws NewmarkNativeException
	{
		return Newmark_Command_Position_Get();
	}

	/**
	 * Method to set how close the reported stage position has to be to the requested stage position
	 * for the stage to be 'in position'.
	 * @param mm The tolerance in mm. Should be between 0.0..1.0 mm.
	 * @exception NewmarkNativeException This method throws a NewmarkNativeException if it failed.
	 * @see #Newmark_Command_Position_Tolerance_Set
	 */
	public void setPositionTolerance(double mm) throws NewmarkNativeException
	{
		Newmark_Command_Position_Tolerance_Set(mm);
	}
}
 
//
// $Log: not supported by cvs2svn $
//

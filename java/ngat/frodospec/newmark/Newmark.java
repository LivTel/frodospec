// Newmark.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/newmark/Newmark.java,v 1.3 2011-01-05 14:13:02 cjm Exp $
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
 * @version $Revision: 1.3 $
 */
public class Newmark
{
	/**
	 * Revision Control System id string, showing the version of the Class
	 */
	public final static String RCSID = new String("$Id: Newmark.java,v 1.3 2011-01-05 14:13:02 cjm Exp $");
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
	 * @param clazz A string representing the class that generated log messages originiting
	 *        from this method call. 
	 * @param source A string representing the source that generated log messages originiting
	 *        from this method call. 
	 */
	private native void Newmark_Command_Home(String clazz,String source) throws NewmarkNativeException;
	/**
	 * Native wrapper to libfrodospec_newmark routine that moves the mechanism.
	 */
	private native void Newmark_Command_Move(String clazz,String source,double position) throws 
		NewmarkNativeException;
	/**
	 * Native wrapper to libfrodospec_newmark routine that aborts movement.
	 * @param clazz A string representing the class that generated log messages originiting
	 *        from this method call. 
	 * @param source A string representing the source that generated log messages originiting
	 *        from this method call. 
	 */
	private native void Newmark_Command_Abort_Move(String clazz,String source) throws NewmarkNativeException;
	/**
	 * Native wrapper to libfrodospec_newmark routine that gets the current mechanism position.
	 * @param clazz A string representing the class that generated log messages originiting
	 *        from this method call. 
	 * @param source A string representing the source that generated log messages originiting
	 *        from this method call. 
	 */
	private native double Newmark_Command_Position_Get(String clazz,String source) throws NewmarkNativeException;
	/**
	 * Native wrapper to libfrodospec_newmark routine that sets the 'in-position' tolerance.
	 * @param clazz A string representing the class that generated log messages originiting
	 *        from this method call. 
	 * @param source A string representing the source that generated log messages originiting
	 *        from this method call. 
	 */
	private native void Newmark_Command_Position_Tolerance_Set(String clazz,String source,double mm) throws 
		NewmarkNativeException;

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
		Newmark_Command_Home(this.getClass().getName(),null);
	}

	/**
	 * Method to home the mechanism.
	 * @param clazz A string representing the class to put in the log record for all logs generated
	 *              by this call.
	 * @param source A string representing the source to put in the log record for all logs generated
	 *              by this call.
	 * @exception NewmarkNativeException This method throws a NewmarkNativeException if it failed.
	 * @see #Newmark_Command_Home
	 */
	public void home(String clazz,String source) throws NewmarkNativeException
	{
		Newmark_Command_Home(clazz,source);
	}

	/**
	 * Method to move the mechanism.
	 * @param position The new position of the mechanism in mm. Valid range around -25 to 175 for our mechanism.
	 * @exception NewmarkNativeException This method throws a NewmarkNativeException if it failed.
	 * @see #Newmark_Command_Move
	 */
	public void move(double position) throws NewmarkNativeException
	{
		Newmark_Command_Move(this.getClass().getName(),null,position);
	}

	/**
	 * Method to move the mechanism.
	 * @param clazz A string representing the class to put in the log record for all logs generated
	 *              by this call.
	 * @param source A string representing the source to put in the log record for all logs generated
	 *              by this call.
	 * @param position The new position of the mechanism in mm. Valid range around -25 to 175 for our mechanism.
	 * @exception NewmarkNativeException This method throws a NewmarkNativeException if it failed.
	 * @see #Newmark_Command_Move
	 */
	public void move(String clazz,String source,double position) throws NewmarkNativeException
	{
		Newmark_Command_Move(clazz,source,position);
	}

	/**
	 * Method to abort any ongoing movement.
	 * @exception NewmarkNativeException This method throws a NewmarkNativeException if it failed.
	 * @see #Newmark_Command_Abort_Move
	 */
	public void abort() throws NewmarkNativeException
	{
		Newmark_Command_Abort_Move(this.getClass().getName(),null);
	}

	/**
	 * Method to abort any ongoing movement.
	 * @param clazz A string representing the class to put in the log record for all logs generated
	 *              by this call.
	 * @param source A string representing the source to put in the log record for all logs generated
	 *              by this call.
	 * @exception NewmarkNativeException This method throws a NewmarkNativeException if it failed.
	 * @see #Newmark_Command_Abort_Move
	 */
	public void abort(String clazz,String source) throws NewmarkNativeException
	{
		Newmark_Command_Abort_Move(clazz,source);
	}

	/**
	 * Method to get the current position of the mechanism.
	 * @return The current position of the mechanism in mm. Valid range around -25 to 175 for our mechanism.
	 * @exception NewmarkNativeException This method throws a NewmarkNativeException if it failed.
	 * @see #Newmark_Command_Position_Get
	 */
	public double getPosition() throws NewmarkNativeException
	{
		return Newmark_Command_Position_Get(this.getClass().getName(),null);
	}

	/**
	 * Method to get the current position of the mechanism.
	 * @param clazz A string representing the class to put in the log record for all logs generated
	 *              by this call.
	 * @param source A string representing the source to put in the log record for all logs generated
	 *              by this call.
	 * @return The current position of the mechanism in mm. Valid range around -25 to 175 for our mechanism.
	 * @exception NewmarkNativeException This method throws a NewmarkNativeException if it failed.
	 * @see #Newmark_Command_Position_Get
	 */
	public double getPosition(String clazz,String source) throws NewmarkNativeException
	{
		return Newmark_Command_Position_Get(clazz,source);
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
		Newmark_Command_Position_Tolerance_Set(this.getClass().getName(),null,mm);
	}

	/**
	 * Method to set how close the reported stage position has to be to the requested stage position
	 * for the stage to be 'in position'.
	 * @param clazz A string representing the class to put in the log record for all logs generated
	 *              by this call.
	 * @param source A string representing the source to put in the log record for all logs generated
	 *              by this call.
	 * @param mm The tolerance in mm. Should be between 0.0..1.0 mm.
	 * @exception NewmarkNativeException This method throws a NewmarkNativeException if it failed.
	 * @see #Newmark_Command_Position_Tolerance_Set
	 */
	public void setPositionTolerance(String clazz,String source,double mm) throws NewmarkNativeException
	{
		Newmark_Command_Position_Tolerance_Set(clazz,source,mm);
	}
}
 
//
// $Log: not supported by cvs2svn $
// Revision 1.2  2009/02/05 11:40:07  cjm
// Swapped Bitwise for Absolute logging levels.
//
// Revision 1.1  2008/11/20 11:34:35  cjm
// Initial revision
//
//

// FocusStage.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/FocusStage.java,v 1.1 2008-11-20 11:33:35 cjm Exp $
package ngat.frodospec;

import java.lang.*;

import ngat.phase2.FrodoSpecConfig;
import ngat.util.*;
import ngat.util.logging.*;
import ngat.serial.arcomess.*;
import ngat.frodospec.newmark.*;

/**
 * An instance of this class is used to control a focus stage.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class FocusStage
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: FocusStage.java,v 1.1 2008-11-20 11:33:35 cjm Exp $");
	/**
	 * FrodoSpec status object.
	 */
	protected FrodoSpecStatus status = null;
	/**
	 * An instance of the Arcom ESS, used to manage the connection to the serial controller.
	 */
	protected ArcomESS arcomESS = null;
	/**
	 * An instance of the Newmark serial controller, used to control this focus stage.
	 */
	protected Newmark newmark = null;
	/**
	 * Boolean determining whether the we should attempt communication with this focus stage.
	 */
	protected boolean enable = false;
	/**
	 * Boolean determining whether the we should attempt to move the focus stage, or just monitor it's position.
	 */
	protected boolean moveEnable = false;
	/**
	 * The initial position we should move the focus stage to.
	 */
	protected double moveValue = 0.0;
	/**
	 * The position tolerance in mm:- 
	 * how close the reported stage position has to be to the requested stage position
	 * for the stage to be 'in position'
	 */
	protected double positionTolerance = 0.002;
	/**
	 * How to communicate with the focus stage, one of INTERFACE_DEVICE_SERIAL, INTERFACE_DEVICE_SOCKET.
	 * @see ngat.serial.arcomess.ArcomESS#INTERFACE_DEVICE_NONE
	 * @see ngat.serial.arcomess.ArcomESS#INTERFACE_DEVICE_SERIAL
	 * @see ngat.serial.arcomess.ArcomESS#INTERFACE_DEVICE_SOCKET
	 */
	protected int deviceId = ArcomESS.INTERFACE_DEVICE_NONE;
	/**
	 * A string representing the connection device name. For SERIAL connection devices, this will be something like
	 * "/dev/ttyS0". 
	 * For SOCKET connection devices (via the Arcom ESS), this will be an IP address or hostname string.
	 */
	protected String deviceName = null;
	/**
	 * The port number of the connection for SOCKET connection devices (via the Arcom ESS).
	 */
	protected int devicePortNumber = 0;
	/**
	 * The logger to use.
	 */
	protected Logger logger = null;
	/**
	 * Which arm this focus stage belongs to.
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	protected int arm = FrodoSpecConfig.NO_ARM;
	/**
	 * A string representation of the arm, should be either "red" or "blue".
	 */
	protected String armString = null;

	/**
	 * Constructor. Construct hardware accessors and point the Newmark to use the ArcomESS.
	 * Don't open a connection yet, though.
	 * @see #arcomESS
	 * @see #newmark
	 * @exception ArcomESSNativeException Thrown if the ArcomESS instance cannot be constructed.
	 * @exception NewmarkNativeException Thrown if the Newmark instance cannot be constructed.
	 */
	public FocusStage() throws ArcomESSNativeException, NewmarkNativeException
	{
		super();
		// construct hardware accessors
		arcomESS = new ArcomESS();
		newmark = new Newmark(arcomESS);
	}

	/**
	 * Initialise the focus stage.
	 * @param status An instance of FrodoSpecStatus. Used to load config from.
	 * @param arm Which arm this stage is attached to. One of BLUE_ARM or RED_ARM in FrodoSpecConfig.
	 * @exception IllegalArgumentException Thrown if arm was not either RED_ARM or BLUE_ARM.
	 * @exception ArcomESSNativeException Thrown if open fails.
	 * @exception NewmarkNativeException Thrown if home fails.
	 * @see #status
	 * @see #loadConfig
	 * @see #setPositionTolerance
	 * @see #home
	 * @see #moveToSetPoint
	 * @see #logger
	 * @see #arm
	 * @see #armString
	 * @see #enable
	 * @see #moveEnable
	 * @see FrodoSpecStatus
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	public void init(FrodoSpecStatus status,int arm) throws IllegalArgumentException, ArcomESSNativeException, 
								NewmarkNativeException
	{
		// set status
		this.status = status;
		// set logger
		logger = LogManager.getLogger("log");
		logger.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,this.getClass().getName()+
			   ":init:Started.");
		// check arm and set armString
		if(arm == FrodoSpecConfig.RED_ARM)
			armString = "red";
		else if(arm == FrodoSpecConfig.BLUE_ARM)
			armString = "blue";
		else
			throw new IllegalArgumentException(this.getClass().getName()+":init:Illegal arm value:"+arm);
		// load config
		logger.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,this.getClass().getName()+
			   ":init:Loading configuration for "+armString+" arm.");
		loadConfig();
		// home and move focus stage for 
		if(enable)
		{
			setPositionTolerance();
			if(moveEnable)
			{
				home();
				moveToSetPoint();
			}
		}
		else
		{
			logger.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,this.getClass().getName()+
				   ":init:enable was false:Focus for arm "+armString+" not initialised.");
		}
		logger.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,this.getClass().getName()+":init:Finished.");
	}

	/**
	 * Method to close any open connections to the focus stage. Closes the arcomESS connection, if enabled.
	 * We synchronise on the arcomESS object whilst doing this in case another thread is accessing the focus stage.
	 * @exception ArcomESSNativeException Thrown if closing the connection failed.
	 * @see #enable
	 * @see #arcomESS
	 * @see ngat.serial.arcomess.ArcomESS#interfaceClose
	 */
	public void close() throws ArcomESSNativeException
	{
		if(enable)
		{
			synchronized(arcomESS)
			{
				arcomESS.interfaceClose();
			}
		}
		else
		{
			logger.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,this.getClass().getName()+
				   ":close:enable was false:Focus for arm "+armString+" not closed.");
		}
	}

	/**
	 * Method to get the current position of the focus stage, if enabled.
	 * We synchronise on the arcomESS object whilst doing this in case another thread is accessing the focus stage.
	 * @return The position of the focus stage, in mm (usually, by default). Usually in the range -25 to 175mm.
	 * @exception ArcomESSNativeException Thrown if the open/close operation failed.
	 * @exception NewmarkNativeException Thrown if getting the position failed.
	 * @see #enable
	 * @see #newmark
	 * @see #open
	 * @see #close
	 * @see ngat.frodospec.newmark.Newmark#getPosition
	 */
	public double getPosition() throws NewmarkNativeException, ArcomESSNativeException
	{
		double position;

		if(enable)
		{
			synchronized(arcomESS)
			{
				try
				{
					open();
					position = newmark.getPosition();
					logger.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,
						   this.getClass().getName()+
						   ":getPosition:Arm "+armString+" has focus position "+position+".");
				}
				finally
				{
					close();
				}
			}
			return position;
		}
		else
		{
			logger.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,this.getClass().getName()+
			     ":getPosition:enable was false:Focus for arm "+armString+" not read - faking result.");
			return 0.0;
		}
	}

	/**
	 * Set the log level. 
	 * @param level The log level.
	 * @see #arcomESS
	 * @see #newmark
	 * @see ngat.serial.arcomess.ArcomESS#setLogFilterLevel
	 * @see ngat.frodospec.newmark.Newmark#setLogFilterLevel
	 */
	public void setLogLevel(int level)
	{
		arcomESS.setLogFilterLevel(level);
		newmark.setLogFilterLevel(level);
	}

	/**
	 * Get whether this focus stage is enabled.
	 * This won't return a sensible answer until init/loadConfig has been called, which initialises the
	 * object's internal enable flag.
	 * @return A boolean, if true the focus stage is enabled.
	 * @see #enable
	 * @see #init
	 * @see #loadConfig
	 */
	public boolean getEnable()
	{
		return enable;
	}

	// protected methods
	/**
	 * Method to load configuration from the status object.
	 * @see #logger
	 * @see #enable
	 * @see #armString
	 * @see #deviceId
	 * @see #deviceName
	 * @see #devicePortNumber
	 * @see #moveEnable
	 * @see #moveValue
	 * @see #positionTolerance
	 */
	protected void loadConfig()
	{
		String deviceIdString = null;

		logger.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,this.getClass().getName()+
			   ":loadConfig:Started.");
		// load config
		enable = status.getPropertyBoolean("frodospec.focus."+armString+".enable");
		if(enable)
		{
			logger.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,this.getClass().getName()+
				   ":loadConfig:Focus enabled for arm "+armString+".");
			deviceIdString = status.getProperty("frodospec.focus."+armString+".device_id");
			deviceId = ArcomESS.interfaceDeviceFromString(deviceIdString);
			deviceName = status.getProperty("frodospec.focus."+armString+".device_name");
			if(deviceId == ArcomESS.INTERFACE_DEVICE_SERIAL)
			{
				devicePortNumber = 0;
			}
			else if(deviceId == ArcomESS.INTERFACE_DEVICE_SOCKET)
			{
				devicePortNumber = status.getPropertyInteger("frodospec.focus."+armString+
									     ".port_number");
			}
			moveEnable = status.getPropertyBoolean("frodospec.focus."+armString+".move");
			moveValue = status.getPropertyDouble("frodospec.focus."+armString+".value");
			// note this one value applies to both arms
			// we will set this value twice in FrodoSpec...
			positionTolerance = status.getPropertyDouble("frodospec.focus.position.tolerance");
		}
		else
		{
			logger.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,this.getClass().getName()+
				   ":loadConfig:enable was false:Focus for arm "+armString+" not initialised.");
		}
	}

	/**
	 * Method to set the Newmark librarys position tolerance.
	 * @exception NewmarkNativeException Thrown if the focus stage failed.
	 * @see #positionTolerance
	 * @see #newmark
	 * @see ngat.frodospec.newmark.Newmark#setPositionTolerance
	 */
	protected void setPositionTolerance() throws NewmarkNativeException
	{
		logger.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,this.getClass().getName()+
			   ":setPositionTolerance:Started.");
		if(enable)
		{
			logger.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,this.getClass().getName()+
				   ":setPositionTolerance:Setting to "+positionTolerance+".");
			newmark.setPositionTolerance(positionTolerance);
		}
		logger.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,this.getClass().getName()+
			   ":setPositionTolerance:Finished.");
	}

	/**
	 * Method to open the connection to the arcomESS, using details loaded from the config,
	 * if the device is enabled.
	 * We synchronise on the arcomESS object whilst doing this in case another thread is accessing the focus stage.
	 * @exception ArcomESSNativeException Thrown if initialising the connection failed.
	 * @see #enable
	 * @see #arcomESS
	 * @see #deviceId
	 * @see #deviceName
	 * @see #devicePortNumber
	 * @see ngat.serial.arcomess.ArcomESS#setLogFilterLevel
	 * @see ngat.serial.arcomess.ArcomESS#interfaceOpen
	 */
	protected void open() throws ArcomESSNativeException
	{
		logger.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,this.getClass().getName()+
			   ":open:Started for arm "+armString+".");
		if(enable)
		{
			synchronized(arcomESS)
			{
				arcomESS.interfaceOpen(deviceId,deviceName,devicePortNumber);
			}
		}
		else
		{
			logger.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,this.getClass().getName()+
				   ":open: "+armString+" arm not enabled.");
		}
		logger.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,this.getClass().getName()+
			   ":open:Finished for arm "+armString+".");
	}

	/**
	 * Method to home the focus stage, if the device is enabled and movement is enabled.
	 * We synchronise on the arcomESS object whilst doing this in case another thread is accessing the focus stage.
	 * @exception ArcomESSNativeException Thrown if the open/close operation failed.
	 * @exception NewmarkNativeException Thrown if the focus stage failed.
	 * @see #enable
	 * @see #moveEnable
	 * @see #newmark
	 * @see #open
	 * @see #close
	 * @see ngat.frodospec.newmark.Newmark#home
	 */
	protected void home() throws NewmarkNativeException,ArcomESSNativeException
	{
		logger.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,this.getClass().getName()+
			   ":home:Started for arm "+armString+".");
		if(enable)
		{
			if(moveEnable)
			{
				synchronized(arcomESS)
				{
					try
					{
						open();
						newmark.home();
					}
					finally
					{
						close();
					}
				}
			}
			else
			{
				logger.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,this.getClass().getName()+
					   ":home: "+armString+" arm not enabled for movement.");
			}
		}
		else
		{
			logger.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,this.getClass().getName()+
				   ":home: "+armString+" arm not enabled.");
		}
		logger.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,this.getClass().getName()+
			   ":home:Finished for arm "+armString+".");
	}

	/**
	 * Method to move the focus stage, if the device is enabled and movement is enabled.
	 * We synchronise on the arcomESS object whilst doing this in case another thread is accessing the focus stage.
	 * @exception ArcomESSNativeException Thrown if the open/close operation failed.
	 * @exception NewmarkNativeException Thrown if the focus stage failed.
	 * @see #enable
	 * @see #moveEnable
	 * @see #newmark
	 * @see #moveValue
	 * @see #open
	 * @see #close
	 * @see ngat.frodospec.newmark.Newmark#move
	 */
	protected void moveToSetPoint() throws NewmarkNativeException, ArcomESSNativeException
	{
		logger.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,this.getClass().getName()+
			   ":moveToSetPoint:Started for arm "+armString+".");
		if(enable)
		{
			if(moveEnable)
			{
				synchronized(arcomESS)
				{
					try
					{
						open();
						newmark.move(moveValue);
					}
					finally
					{
						close();
					}
				}
			}
			else
			{
				logger.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,this.getClass().getName()+
					   ":moveToSetPoint: "+armString+" arm not enabled for movement.");
			}
		}
		else
		{
			logger.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,this.getClass().getName()+
				   ":moveToSetPoint: "+armString+" arm not enabled.");
		}
		logger.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,this.getClass().getName()+
			   ":moveToSetPoint:Finished for arm "+armString+".");
	}
}
//
// $Log: not supported by cvs2svn $
//

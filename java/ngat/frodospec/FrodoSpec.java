// FrodoSpec.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/FrodoSpec.java,v 1.7 2009-02-09 15:08:20 cjm Exp $
package ngat.frodospec;


import java.lang.*;
import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;
import java.io.*;
import java.net.*;
import java.text.*;
import java.util.*;

import ngat.eip.*;
import ngat.lamp.*;
import ngat.net.*;
import ngat.util.*;
import ngat.util.logging.*;

import ngat.frodospec.ccd.*;
import ngat.frodospec.newmark.NewmarkNativeException;
import ngat.serial.arcomess.ArcomESSNativeException;
import ngat.fits.*;
import ngat.message.ISS_INST.*;
import ngat.message.INST_DP.*;
import ngat.phase2.*;

/**
 * This class is the start point for the FrodoSpec Control System.
 * @author Chris Mottram
 * @version $Revision: 1.7 $
 */
public class FrodoSpec
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: FrodoSpec.java,v 1.7 2009-02-09 15:08:20 cjm Exp $");
	/**
	 * Logger channel id.
	 */
	public final static String LOGGER_CHANNEL_ID = new String("FRODOSPEC");
	/**
	 * The minimum port number to listen for connections on.
	 */
	static final int MINIMUM_PORT_NUMBER = 1025;
	/**
	 * The maximum port number to send ISS commands on.
	 */
	static final int MAXIMUM_PORT_NUMBER = 65535;
	/**
	 * The server class that listens for connections.
	 */
	private FrodoSpecTCPServer server = null;
	/**
	 * The server class that listens for Telescope Image Transfer request connections.
	 */
	private TitServer titServer = null;
	/**
	 * The CCDLibrary class - used to interface with the SDSU CCD Controller.
	 */
	private CCDLibrary redCCD = null;
	/**
	 * The CCDLibrary class - used to interface with the SDSU CCD Controller.
	 */
	private CCDLibrary blueCCD = null;
	/**
	 * The Plc instance used for comms to the FrodoSpec PLC.
	 */
	private Plc plc = null;
	/**
	 * List of FrodoSpec focus stages - one per arm.
	 */
	private FocusStage focusStageList[] = {null,null,null};
	/**
	 * An instance of LTAGLampUnit to control/interface with the LT A&G Box calibration lamps.
	 * @see ngat.lamp.LTAGLampUnit
	 */
	private LTAGLampUnit lampUnit = null;
	/**
	 * An instance of LampController, to control /synchronize access to the LT A&G Box calibration lamps
	 * between both arms of FrodoSpec.
	 * @see #lampUnit
	 * @see ngat.frodospec.LampController
	 */
	private LampController lampController = null;
	/**
	 * FrodoSpec status object.
	 */
	private FrodoSpecStatus status = null;
	/**
	 * FrodoSpec FITS Filename objects to generate unique fits filenames according to ISS rules.
	 */
	private FitsFilename fitsFilenameList[] = {null,null,null};
	/**
	 * The only instance of the ngat.fits.FitsHeader class - used to interface with some Java JNI routines 
	 * to write FITS headers.
	 */
	private FitsHeader libngatfitsList[] = {null,null,null};
	/**
	 * The only instance of the ngat.fits.FitsHeaderDefaults class - used to specify some default
	 * values/comments/units/fits keyword ordering when writing FITS files.
	 */
	private FitsHeaderDefaults fitsHeaderDefaultsList[] = {null,null,null};
	/**
	 * This hashtable holds the map between COMMAND sub-class names and their implementations, which
	 * are stored as the Hashtable data values as class objects of sub-classes of CommandImplementation.
	 * When the FrodoSpec gets a COMMAND from a client it can query this Hashtable to find it's 
	 * implementation class.
	 */
	private Hashtable implementationList = null;
	/**
	 * Command line argument. The level of logging to perform in FrodoSpec.
	 */
	private int logLevel = 0;
	/**
	 * The port number to listen for connections from clients on.
	 */
	private int frodospecPortNumber = 0;
	/**
	 * The ip address of the machine the ISS is running on, to send ISS commands to.
	 */
	private InetAddress issAddress = null;
	/**
	 * The port number to send iss commands to.
	 */
	private int issPortNumber = 0;
	/**
	 * The ip address of the machine the DP(RT) is running on, to send Data Pipeline (Real Time) commands to.
	 */
	private InetAddress dprtAddress = null;
	/**
	 * The port number to send DP(RT) commands to.
	 */
	private int dprtPortNumber = 0;
	/**
	 * The port number to listen for Telescope Image Transfer requests.
	 */
	private int titPortNumber = 0;
	/**
	 * The logging logger.
	 */
	protected Logger logLogger = null;
	/**
	 * The error logger.
	 */
	protected Logger errorLogger = null;
	/**
	 * The thread monitor window.
	 */
	private ThreadMonitorFrame threadMonitorFrame = null;
	/**
	 * The network configuration file filename, as specified in the command line arguments.
	 */
	private String netConfigurationFilename = null;
	/**
	 * The main FrodoSpec configuration file filename, as specified in the command line arguments.
	 */
	private String configurationFilename = null;
	/**
	 * The filter database configuration filename, as specified in the command line arguments.
	 */
	private String filterConfigurationFilename = null;
	/**
	 * The current filter configuration filename, as specified in the command line arguments.
	 */
	private String currentFilterConfigurationFilename = null;
	/**
	 * The FITS header defaults filename, as specified in the command line arguments.
	 */
	private String fitsHeaderDefaultsFilename = null;
	/**
	 * Whether to start the thread monitor, as specified in the command line arguments.
	 */
	private boolean threadMonitor = false;

	/**
	 * This is the initialisation routine. This creates the status,
	 * fitsFilenameList, redCCD, blueCCD, plc, libngatfitsList and fitsHeaderDefaults objects. 
	 * The properties for the application are loaded into the status object.
	 * The error and log files are opened.<br>
	 * It calls the <a href="#initImplementationList">initImplementationList</a> method, which creates the
	 * <a href="#implementationList">implementationList</a>. This stores the mapping between messages sent
	 * to the server and classes to perform the tasks to complete the messages.
	 * See the reInit method, which is called when a REDATUM level reboot is performed,
	 * and re-initialises some of the above from new values in the configuration files.
	 * The reInit method must be kept up to date with respect to this method.
	 * @exception FileNotFoundException Thrown if the property file cannot be found.
	 * @exception IOException Thrown if the property file cannot be accessed and the properties cannot
	 * 	be loaded for some reason.
	 * @exception NumberFormatException Thrown if getting a port number from the configuration file that
	 * 	is not a valid integer.
	 * @exception ArcomESSNativeException Thrown by the FocusStage constructor.
	 * @exception NewmarkNativeException Thrown by the FocusStage constructor.
	 * @exception Exception Thrown from FitsFilename.initialise, if the directory listing failed.
	 * @see #status
	 * @see #fitsFilenameList
	 * @see #redCCD
	 * @see #blueCCD
	 * @see #plc
	 * @see #focusStageList
	 * @see #lampUnit
	 * @see #lampController
	 * @see #libngatfitsList
	 * @see #fitsHeaderDefaultsList
	 * @see #initLoggers
	 * @see #implementationList
	 * @see #initImplementationList
	 * @see #reInit
	 * @see #titPortNumber
	 * @see #frodospecPortNumber
	 * @see #issPortNumber
	 * @see #dprtPortNumber
	 * @see #issAddress
	 * @see #dprtAddress
	 * @see #netConfigurationFilename
	 * @see #configurationFilename
	 * @see #filterConfigurationFilename
	 * @see #currentFilterConfigurationFilename
	 * @see #fitsHeaderDefaultsFilename
	 * @see #logLevel
	 * @see #setLogLevel
	 */
	private void init() throws FileNotFoundException,IOException,
		CCDLibraryFormatException,NumberFormatException,CCDLibraryNativeException,ArcomESSNativeException, 
				   NewmarkNativeException, Exception
	{
		String filename = null;
		String s = null;
		int time;

	// create status object and load frodospec properties into it
		status = new FrodoSpecStatus();
		try
		{
		// note these filenames can be null if the command line argument was not given,
		// but status.load uses the default in this case.
			status.load(netConfigurationFilename,configurationFilename);
		}
		catch(FileNotFoundException e)
		{
			error(this.getClass().getName()+":init:loading properties:",e);
			throw e;
		}
		catch(IOException e)
		{
			error(this.getClass().getName()+":init:loading properties:",e);
			throw e;
		}
	// Logging
		initLoggers();
		// for each arm (FrodoSpecConfig.RED_ARM=1,FrodoSpecConfig.BLUE_ARM=2)
		for(int i = FrodoSpecConfig.RED_ARM; i <= FrodoSpecConfig.BLUE_ARM; i++)
		{
			// create the fits filename object
			fitsFilenameList[i] = new FitsFilename();
			fitsFilenameList[i].setDirectory(status.getProperty("frodospec.file.fits.path"));
			if(i == FrodoSpecConfig.RED_ARM)
			{
				s = status.getProperty("frodospec.file.fits.instrument_code.red");
			}
			else if(i == FrodoSpecConfig.BLUE_ARM)
			{
				s = status.getProperty("frodospec.file.fits.instrument_code.blue");
			}
			fitsFilenameList[i].setInstrumentCode(s);
			fitsFilenameList[i].initialise();
			// Create instance of the FITS header JNI library.
			libngatfitsList[i] = new FitsHeader();
			// Create instance of FITS header defaults
			fitsHeaderDefaultsList[i] = new FitsHeaderDefaults();
			// Load defaults from properties file.
			try
			{
				// note the filename can be null if the command line argument was not given,
				// but status.load uses the default in this case.
				if(fitsHeaderDefaultsFilename == null)
					fitsHeaderDefaultsList[i].load();
				else
					fitsHeaderDefaultsList[i].load(fitsHeaderDefaultsFilename);
			}
			catch(FileNotFoundException e)
			{
				error(this.getClass().getName()+":init:loading default FITS header properties:",e);
				throw e;
			}
			catch(IOException e)
			{
				error(this.getClass().getName()+":init:loading default FITS header properties:",e);
				throw e;
			}
		}// end for on arms
	// create hardware control objects
		redCCD = new CCDLibrary();
		blueCCD = new CCDLibrary();
		plc = new Plc();
		for(int i = FrodoSpecConfig.RED_ARM; i <= FrodoSpecConfig.BLUE_ARM; i++)
		{
			focusStageList[i] = new FocusStage();
		}
		// initialise lamp unit
		lampUnit = new LTAGLampUnit();
		lampUnit.loadConfig(status.getProperty("frodospec.lamp.configuration.file"));
		// initialise lamp controller
		lampController = new LampController();
		lampController.setLampUnit(lampUnit);
	// initialise sub-system loggers, after creating status, hardware control objects
		setLogLevel(logLevel);
	// Create and initialise the implementationList
		initImplementationList();
	// initialise port numbers from properties file/ command line arguments
		try
		{
			// Check whether number set in parseArguments, it not get from net config
			if(frodospecPortNumber == 0)
				frodospecPortNumber = status.getPropertyInteger("frodospec.net.default_frodospec_port_number");
			// Check whether number set in parseArguments, it not get from net config
			if(issPortNumber == 0)
				issPortNumber = status.getPropertyInteger("frodospec.net.default_iss_port_number");
			// Check whether number set in parseArguments, it not get from net config
			if(dprtPortNumber == 0)
				dprtPortNumber = status.getPropertyInteger("frodospec.net.default_dprt_port_number");
			// Check whether number set in parseArguments, it not get from net config
			if(titPortNumber == 0)
				titPortNumber = status.getPropertyInteger("frodospec.net.default_tit_port_number");
		}
		catch(NumberFormatException e)
		{
			error(this.getClass().getName()+":init:initialsing port number:",e);
			throw e;
		}
	// initialise address's from properties file
		try
		{
			// Check whether address set in parseArguments, it not get from net config
			if(issAddress == null)
				issAddress = InetAddress.getByName(status.getProperty("frodospec.net.default_iss_address"));
			// Check whether address set in parseArguments, it not get from net config
			if(dprtAddress == null)
				dprtAddress = InetAddress.getByName(status.getProperty("frodospec.net.default_dprt_address"));
		}
		catch(UnknownHostException e)
		{
			error(this.getClass().getName()+":illegal internet address:",e);
			throw e;
		}
	// initialise default connection response times from properties file
		try
		{
			time = status.getPropertyInteger("frodospec.server_connection.default_acknowledge_time");
			FrodoSpecTCPServerConnectionThread.setDefaultAcknowledgeTime(time);
			time = status.getPropertyInteger("frodospec.server_connection.min_acknowledge_time");
			FrodoSpecTCPServerConnectionThread.setMinAcknowledgeTime(time);
		}
		catch(NumberFormatException e)
		{
			error(this.getClass().getName()+":init:initialsing server connection thread times:",e);
			// don't throw the error - failing to get this property is not 'vital' to frodospec.
		}		
	}

	/**
	 * Initialise log handlers. Called from init only, not re-configured on a REDATUM level reboot.
	 * @see #LOGGER_CHANNEL_ID
	 * @see #init
	 * @see #initLogHandlers
	 * @see #copyLogHandlers
	 * @see #errorLogger
	 * @see #logLogger
	 */
	protected void initLoggers()
	{
	// errorLogger setup
		errorLogger = LogManager.getLogger("error");
		errorLogger.setChannelID(LOGGER_CHANNEL_ID+"-ERROR");
		initLogHandlers(errorLogger);
		errorLogger.setLogLevel(Logging.ALL);
	// ngat.net error loggers
		copyLogHandlers(errorLogger,LogManager.getLogger("ngat.net.TCPServer"),null,Logging.ALL);
		copyLogHandlers(errorLogger,LogManager.getLogger("ngat.net.TCPServerConnectionThread"),null,
				Logging.ALL);
		copyLogHandlers(errorLogger,LogManager.getLogger("ngat.net.TCPClientConnectionThreadMA"),null,
				Logging.ALL);
	// logLogger setup
		logLogger = LogManager.getLogger("log");
		logLogger.setChannelID(LOGGER_CHANNEL_ID);
		initLogHandlers(logLogger);
		logLogger.setLogLevel(status.getLogLevel());
	// library logging loggers
		copyLogHandlers(logLogger,LogManager.getLogger("ngat.frodospec.ccd.CCDLibrary"),null,
				status.getLogLevel());
		copyLogHandlers(logLogger,LogManager.getLogger("ngat.eip.EIPPLC"),null,status.getLogLevel());
		copyLogHandlers(logLogger,LogManager.getLogger("ngat.frodospec.newmark.Newmark"),null,
				status.getLogLevel());
		copyLogHandlers(logLogger,LogManager.getLogger("ngat.net.TitServer"),null,status.getLogLevel());
		copyLogHandlers(logLogger,LogManager.getLogger("ngat.serial.arcomess.ArcomESS"),null,
				status.getLogLevel());
		copyLogHandlers(logLogger,LogManager.getLogger("ngat.lamp.LTAGLampUnit"),null,status.getLogLevel());
		copyLogHandlers(logLogger,LogManager.getLogger("ngat.lamp.LTLamp"),null,status.getLogLevel());
		copyLogHandlers(logLogger,LogManager.getLogger("ngat.lamp.PLCConnection"),null,status.getLogLevel());
	}

	/**
	 * Method to create and add all the handlers for the specified logger.
	 * These handlers are in the status properties:
	 * "frodospec.log."+l.getName()+".handler."+index+".name" retrieves the relevant class name
	 * for each handler.
	 * @param l The logger.
	 * @see #initFileLogHandler
	 * @see #initConsoleLogHandler
	 * @see #initDatagramLogHandler
	 * @see #initMulticastLogHandler
	 * @see #initMulticastLogRelay
	 */
	protected void initLogHandlers(Logger l)
	{
		LogHandler handler = null;
		String handlerName = null;
		int index = 0;

		do
		{
			handlerName = status.getProperty("frodospec.log."+l.getName()+".handler."+index+".name");
			if(handlerName != null)
			{
				try
				{
					handler = null;
					if(handlerName.equals("ngat.util.logging.FileLogHandler"))
					{
						handler = initFileLogHandler(l,index);
					}
					else if(handlerName.equals("ngat.util.logging.ConsoleLogHandler"))
					{
						handler = initConsoleLogHandler(l,index);
					}
					else if(handlerName.equals("ngat.util.logging.MulticastLogHandler"))
					{
						handler = initMulticastLogHandler(l,index);
					}
					else if(handlerName.equals("ngat.util.logging.MulticastLogRelay"))
					{
						handler = initMulticastLogRelay(l,index);
					}
					else if(handlerName.equals("ngat.util.logging.DatagramLogHandler"))
					{
						handler = initDatagramLogHandler(l,index);
					}
					else
					{
						error("initLogHandlers:Unknown handler:"+handlerName);
					}
					if(handler != null)
					{
						handler.setLogLevel(Logging.ALL);
						l.addHandler(handler);
					}
				}
				catch(Exception e)
				{
					error("initLogHandlers:Adding Handler failed:",e);
				}
				index++;
			}
		}
		while(handlerName != null);
	}

	/**
	 * Routine to add a FileLogHandler to the specified logger.
	 * This method expects either 3 or 6 constructor parameters to be in the status properties.
	 * If there are 6 parameters, we create a record limited file log handler with parameters:
	 * <ul>
	 * <li>param.0 is the filename.
	 * <li>param.1 is the formatter class name.
	 * <li>param.2 is the record limit in each file.
	 * <li>param.3 is the start index for file suffixes.
	 * <li>param.4 is the end index for file suffixes.
	 * <li>param.5 is a boolean saying whether to append to files.
	 * </ul>
	 * If there are 3 parameters, we create a time period file log handler with parameters:
	 * <ul>
	 * <li><b>param.0</b> is the filename.
	 * <li><b>param.1</b> is the formatter class name.
	 * <li><b>param.2</b> is the time period, either 'HOURLY_ROTATION','DAILY_ROTATION' or 'WEEKLY_ROTATION'.
	 * </ul>
	 * @param l The logger to add the handler to.
	 * @param index The index in the property file of the handler we are adding.
	 * @return A LogHandler of the relevant class is returned, if no exception occurs.
	 * @exception NumberFormatException Thrown if the numeric parameters in the properties
	 * 	file are not valid numbers.
	 * @exception FileNotFoundException Thrown if the specified filename is not valid in some way.
	 * @see #status
	 * @see #initLogFormatter
	 * @see FrodoSpecStatus#getProperty
	 * @see FrodoSpecStatus#getPropertyInteger
	 * @see FrodoSpecStatus#getPropertyBoolean
	 * @see FrodoSpecStatus#propertyContainsKey
	 * @see FrodoSpecStatus#getPropertyLogHandlerTimePeriod
	 */
	protected LogHandler initFileLogHandler(Logger l,int index) throws NumberFormatException,
		FileNotFoundException
	{
		LogFormatter formatter = null;
		LogHandler handler = null;
		String fileName;
		int recordLimit,fileStart,fileLimit,timePeriod;
		boolean append;

		fileName = status.getProperty("frodospec.log."+l.getName()+".handler."+index+".param.0");
		formatter = initLogFormatter("frodospec.log."+l.getName()+".handler."+index+".param.1");
		// if we have more then 3 parameters, we are using a recordLimit FileLogHandler
		// rather than a time period log handler.
		if(status.propertyContainsKey("frodospec.log."+l.getName()+".handler."+index+".param.3"))
		{
			recordLimit = status.getPropertyInteger("frodospec.log."+l.getName()+".handler."+
								index+".param.2");
			fileStart = status.getPropertyInteger("frodospec.log."+l.getName()+".handler."+
							      index+".param.3");
			fileLimit = status.getPropertyInteger("frodospec.log."+l.getName()+".handler."+
							      index+".param.4");
			append = status.getPropertyBoolean("frodospec.log."+l.getName()+".handler."+index+".param.5");
			handler = new FileLogHandler(fileName,formatter,recordLimit,fileStart,fileLimit,append);
		}
		else
		{
			// This is a time period log handler.
			timePeriod = status.getPropertyLogHandlerTimePeriod("frodospec.log."+l.getName()+".handler."+
									    index+".param.2");
			handler = new FileLogHandler(fileName,formatter,timePeriod);
		}
		return handler;
	}

	/**
	 * Routine to add a MulticastLogHandler to the specified logger.
	 * The parameters to the constructor are stored in the status properties:
	 * <ul>
	 * <li>param.0 is the multicast group name i.e. "228.0.0.1".
	 * <li>param.1 is the port number i.e. 5000.
	 * <li>param.2 is the formatter class name.
	 * </ul>
	 * @param l The logger to add the handler to.
	 * @param index The index in the property file of the handler we are adding.
	 * @return A LogHandler of the relevant class is returned, if no exception occurs.
	 * @exception IOException Thrown if the multicast socket cannot be created for some reason.
	 */
	protected LogHandler initMulticastLogHandler(Logger l,int index) throws IOException
	{
		LogFormatter formatter = null;
		LogHandler handler = null;
		String groupName = null;
		int portNumber;

		groupName = status.getProperty("frodospec.log."+l.getName()+".handler."+index+".param.0");
		portNumber = status.getPropertyInteger("frodospec.log."+l.getName()+".handler."+index+".param.1");
		formatter = initLogFormatter("frodospec.log."+l.getName()+".handler."+index+".param.2");
		handler = new MulticastLogHandler(groupName,portNumber,formatter);
		return handler;
	}

	/**
	 * Routine to add a MulticastLogRelay to the specified logger.
	 * The parameters to the constructor are stored in the status properties:
	 * <ul>
	 * <li>param.0 is the multicast group name i.e. "228.0.0.1".
	 * <li>param.1 is the port number i.e. 5000.
	 * </ul>
	 * @param l The logger to add the handler to.
	 * @param index The index in the property file of the handler we are adding.
	 * @return A LogHandler of the relevant class is returned, if no exception occurs.
	 * @exception IOException Thrown if the multicast socket cannot be created for some reason.
	 */
	protected LogHandler initMulticastLogRelay(Logger l,int index) throws IOException
	{
		LogHandler handler = null;
		String groupName = null;
		int portNumber;

		groupName = status.getProperty("frodospec.log."+l.getName()+".handler."+index+".param.0");
		portNumber = status.getPropertyInteger("frodospec.log."+l.getName()+".handler."+index+".param.1");
		handler = new MulticastLogRelay(groupName,portNumber);
		return handler;
	}

	/**
	 * Routine to add a DatagramLogHandler to the specified logger.
	 * The parameters to the constructor are stored in the status properties:
	 * <ul>
	 * <li>param.0 is the hostname i.e. "ltproxy".
	 * <li>param.1 is the port number i.e. 2371.
	 * </ul>
	 * @param l The logger to add the handler to.
	 * @param index The index in the property file of the handler we are adding.
	 * @return A LogHandler of the relevant class is returned, if no exception occurs.
	 * @exception IOException Thrown if the multicast socket cannot be created for some reason.
	 */
	protected LogHandler initDatagramLogHandler(Logger l,int index) throws IOException
	{
		LogHandler handler = null;
		String hostname = null;
		int portNumber;

		hostname = status.getProperty("frodospec.log."+l.getName()+".handler."+index+".param.0");
		portNumber = status.getPropertyInteger("frodospec.log."+l.getName()+".handler."+index+".param.1");
		handler = new DatagramLogHandler(hostname,portNumber);
		return handler;
	}

	/**
	 * Routine to add a ConsoleLogHandler to the specified logger.
	 * The parameters to the constructor are stored in the status properties:
	 * <ul>
	 * <li>param.0 is the formatter class name.
	 * </ul>
	 * @param l The logger to add the handler to.
	 * @param index The index in the property file of the handler we are adding.
	 * @return A LogHandler of class FileLogHandler is returned, if no exception occurs.
	 */
	protected LogHandler initConsoleLogHandler(Logger l,int index)
	{
		LogFormatter formatter = null;
		LogHandler handler = null;

		formatter = initLogFormatter("frodospec.log."+l.getName()+".handler."+index+".param.0");
		handler = new ConsoleLogHandler(formatter);
		return handler;
	}

	/**
	 * Method to create an instance of a LogFormatter, given a property name
	 * to retrieve it's details from. If the property does not exist, or the class does not exist
	 * or an instance cannot be instansiated we try to return a ngat.util.logging.BogstanLogFormatter.
	 * @param propertyName A property name, present in the status's properties, 
	 * 	which has a value of a valid LogFormatter sub-class name. i.e.
	 * 	<pre>frodospec.log.log.handler.0.param.1 =ngat.util.logging.BogstanLogFormatter</pre>
	 * @return An instance of LogFormatter is returned.
	 */
	protected LogFormatter initLogFormatter(String propertyName)
	{
		LogFormatter formatter = null;
		String formatterName = null;
		Class formatterClass = null;

		formatterName = status.getProperty(propertyName);
		if(formatterName == null)
		{
			error("initLogFormatter:NULL formatter for:"+propertyName);
			formatterName = "ngat.util.logging.BogstanLogFormatter";
		}
		try
		{
			formatterClass = Class.forName(formatterName);
		}
		catch(ClassNotFoundException e)
		{
			error("initLogFormatter:Unknown class formatter:"+formatterName+
				" from property "+propertyName);
			formatterClass = BogstanLogFormatter.class;
		}
		try
		{
			formatter = (LogFormatter)formatterClass.newInstance();
		}
		catch(Exception e)
		{
			error("initLogFormatter:Cannot create instance of formatter:"+formatterName+
				" from property "+propertyName);
			formatter = (LogFormatter)new BogstanLogFormatter();
		}
	// set better date format if formatter allows this.
	// Note we really need LogFormatter to generically allow us to do this
		if(formatter instanceof BogstanLogFormatter)
		{
			BogstanLogFormatter blf = (BogstanLogFormatter)formatter;

			blf.setDateFormat(new SimpleDateFormat("yyyy/MM/dd HH:mm:ss.SSS z"));
		}
		if(formatter instanceof SimpleLogFormatter)
		{
			SimpleLogFormatter slf = (SimpleLogFormatter)formatter;

			slf.setDateFormat(new SimpleDateFormat("yyyy/MM/dd HH:mm:ss.SSS z"));
		}
		return formatter;
	}

	/**
	 * Method to copy handlers from one logger to another. The outputLogger's channel ID is also
	 * copied from the input logger.
	 * @param inputLogger The logger to copy handlers from.
	 * @param outputLogger The logger to copy handlers to.
	 * @param lf The log filter to apply to the output logger. If this is null, the filter is not set.
	 * @param logLevel The log level to set the logger to filter against.
	 */
	protected void copyLogHandlers(Logger inputLogger,Logger outputLogger,LogFilter lf,int logLevel)
	{
		LogHandler handlerList[] = null;
		LogHandler handler = null;

		handlerList = inputLogger.getHandlers();
		for(int i = 0; i < handlerList.length; i++)
		{
			handler = handlerList[i];
			outputLogger.addHandler(handler);
		}
		outputLogger.setLogLevel(inputLogger.getLogLevel());
		if(lf != null)
			outputLogger.setFilter(lf);
		outputLogger.setChannelID(inputLogger.getChannelID());
		outputLogger.setLogLevel(logLevel);
	}

	/**
	 * This is the re-initialisation routine. This is called on a REDATUM level reboot, and
	 * does some of the operations in the init routine. It re-loads the FrodoSpec,filter and FITS configuration
	 * files, but NOT the network one. It resets the FitsFilename directory and instrument code. 
	 * It re-loads FITS header defaults.
	 * It re-initialises default connection response times from properties file.
	 * The init method must be kept up to date with respect to this method.
	 * @exception FileNotFoundException Thrown if the property file cannot be found.
	 * @exception IOException Thrown if the property file cannot be accessed and the properties cannot
	 * 	be loaded for some reason.
	 * @exception Exception Thrown from FitsFilename.initialise, if the directory listing failed.
	 * @see #status
	 * @see #fitsFilenameList
	 * @see #fitsHeaderDefaultsList
	 * @see #init
	 * @see #configurationFilename
	 * @see #fitsHeaderDefaultsFilename
	 * @see #setLogLevel
	 */
	public void reInit() throws FileNotFoundException,IOException,
		CCDLibraryFormatException,NumberFormatException,CCDLibraryNativeException,Exception
	{
		String filename = null;
		String s  = null;
		int time;

	// reload properties into the status object
		try
		{
			status.reload(configurationFilename);
		}
		catch(FileNotFoundException e)
		{
			error(this.getClass().getName()+":reinit:loading properties:",e);
			throw e;
		}
		catch(IOException e)
		{
			error(this.getClass().getName()+":reinit:loading properties:",e);
			throw e;
		}
	// don't change errorLogger to files defined in loaded properties
	// don't change logLogger to files defined in loaded properties
	// set the fits filename instrument code/directory, and re-initialise runnum etc.
		for(int i = FrodoSpecConfig.RED_ARM; i <= FrodoSpecConfig.BLUE_ARM; i++)
		{
			fitsFilenameList[i].setDirectory(status.getProperty("frodospec.file.fits.path"));
			if(i == FrodoSpecConfig.RED_ARM)
			{
				s= status.getProperty("frodospec.file.fits.instrument_code.red");
			}
			else if(i == FrodoSpecConfig.BLUE_ARM)
			{
				s = status.getProperty("frodospec.file.fits.instrument_code.blue");
			}
			fitsFilenameList[i].setInstrumentCode(s);
			fitsFilenameList[i].initialise();
	// don't create instance of FITS header defaults
	// reload  FITS header defaults from properties file.
			try
			{
				if(fitsHeaderDefaultsFilename == null)
					fitsHeaderDefaultsList[i].load();
				else
					fitsHeaderDefaultsList[i].load(fitsHeaderDefaultsFilename);
			}
			catch(FileNotFoundException e)
			{
				error(this.getClass().getName()+":reinit:loading default FITS header properties:",e);
				throw e;
			}
			catch(IOException e)
			{
				error(this.getClass().getName()+":reinit:loading default FITS header properties:",e);
				throw e;
			}
		}// end for on arm
	// don't create hardware control objects
		lampUnit.loadConfig(status.getProperty("frodospec.lamp.configuration.file"));
	// initialise sub-system loggers
		setLogLevel(logLevel);
	// don't create and initialise the implementationList
	// don't initialise port numbers from properties file
	// don't initialise address's from properties file
	// initialise default connection response times from properties file
		try
		{
			time = status.getPropertyInteger("frodospec.server_connection.default_acknowledge_time");
			FrodoSpecTCPServerConnectionThread.setDefaultAcknowledgeTime(time);
			time = status.getPropertyInteger("frodospec.server_connection.min_acknowledge_time");
			FrodoSpecTCPServerConnectionThread.setMinAcknowledgeTime(time);
		}
		catch(NumberFormatException e)
		{
			error(this.getClass().getName()+":reinit:initialsing server connection thread times:",e);
			// don't throw the error - failing to get this property is not 'vital' to frodospec.
		}
	}

	/**
	 * This method creates the implementationList, and fills it with Class objects of sub-classes
	 * of CommandImplementation. The command implementation namess are retrieved from the FrodoSpec property files,
	 * using keys of the form <b>frodospec.command.implmentation.&lt;<i>N</i>&gt;</b>, where <i>N</i> is
	 * an integer is incremented. It puts the class object reference in the Hashtable with the 
	 * results of it's getImplementString static method
	 * as the key. If an implementation object class fails to be put in the hashtable for some reason
	 * it ignores it and continues for the next object in the list.
	 * @see #implementationList
	 * @see CommandImplementation#getImplementString
	 */
	private void initImplementationList()
	{
		Class cl = null;
		Class oldClass = null;
		Method method = null;
		Class methodClassParameterList[] = {};
		Object methodParameterList[] = {};
		String implementString = null;
		String className = null;
		int index;
		boolean done;

		implementationList = new Hashtable();
		index = 0;
		done = false;
		while(done == false)
		{
			className = status.getProperty("frodospec.command.implmentation."+index);
			if(className != null)
			{
				try
				{
				// get Class object associated with class name
					cl = Class.forName(className);
				// get method object associated with getImplementString method of cl.
					method = cl.getDeclaredMethod("getImplementString",methodClassParameterList);
				// invoke getImplementString class method to get ngat.message class name it implements
					implementString = (String)method.invoke(null,methodParameterList);
				// put key and class into implementationList
					oldClass = (Class)implementationList.put(implementString,cl);
					if(oldClass != null)// the put returned another class with the same key.
					{
						error(this.getClass().getName()+":initImplementationList:Classes "+
							oldClass.getName()+" and "+cl.getName()+
							" both implement command:"+implementString);
					}
				}
				catch(ClassNotFoundException e)//Class.forName exception
				{
					error(this.getClass().getName()+":initImplementationList:Class "+
						className+":ClassNotFoundException:",e);
					// keep trying for next implementation in the list
				}
				catch(NoSuchMethodException e)//Class.getDeclaredMethod exception
				{
					error(this.getClass().getName()+":initImplementationList:Class "+
						className+":NoSuchMethodException:",e);
					// keep trying for next implementation in the list
				}
				catch(SecurityException e)//Class.getDeclaredMethod exception
				{
					error(this.getClass().getName()+":initImplementationList:Class "+
						className+":SecurityException:",e);
					// keep trying for next implementation in the list
				}
				catch(NullPointerException e)// Hashtable.put exception - null key.
				{
					error(this.getClass().getName()+":initImplementationList:Class "+
						cl.getName()+" implement string is null?:",e);
					// keep trying for next implementation in the list
				}
				catch(IllegalAccessException e)// Method.invoke exception
				{
					error(this.getClass().getName()+":initImplementationList:Class "+
						cl.getName()+":IllegalAccessException:",e);
					// keep trying for next implementation in the list
				}
				catch(IllegalArgumentException e)// Method.invoke exception
				{
					error(this.getClass().getName()+":initImplementationList:Class "+
						cl.getName()+":IllegalArgumentException:",e);
					// keep trying for next implementation in the list
				}
				catch(InvocationTargetException e)// Method.invoke exception
				{
					error(this.getClass().getName()+":initImplementationList:Class "+
						cl.getName()+":InvocationTargetException:",e);
					// keep trying for next implementation in the list
				}
			// try next class name in list
				index++;
			}
			else
				done = true;
		}// end while not done
	}

	/**
	 * Method to open a connection to the FrodoSpec control objects and send initialisation control sequences
	 * to them. 
	 * @exception CCDLibraryFormatException Thrown if the CCD configuration properties cannot be determined.
	 * @exception CCDLibraryNativeException Thrown if the call to open or setup the CCD controllers fails.
	 * @exception IllegalArgumentException Thrown by the startupFocusStage method.
	 * @exception ArcomESSNativeException Thrown by the startupFocusStage method.
	 * @exception NewmarkNativeException Thrown by the startupFocusStage method.
	 * @exception EIPNativeException Thrown by the startupPLC method, or the lampUnit init method.
	 * @exception Exception Thrown if an error occurs.
	 * @see #startupCCDController
	 * @see #startupPLC
	 * @see #startupFocusStage
	 * @see #lampUnit
	 * @see ngat.lamp.LTAGLampUnit#init
	 */
	public void startupHardware() throws CCDLibraryFormatException, CCDLibraryNativeException, 
					     EIPNativeException, ArcomESSNativeException, 
					     NewmarkNativeException, IllegalArgumentException, Exception
	{
		startupCCDController();
		startupPLC();
		startupFocusStage();
		lampUnit.init();
	}

	/**
	 * Method to open a connection to the FrodoSpec control objects and send initialisation control sequences
	 * to them. The SDSU CCD Controller(s) are configured. 
	 * This method assumes the <a href="#init">init</a> method has already been run to
	 * construct the redCCD, blueCCD,and <a href="#status">status</a> objects.
	 * It gets it's configuration from the FrodoSpec config file.
	 * <ul>
	 * <li>It gets it's configuration from the FrodoSpec config file.
	 * <li>The CCD librarys are initialised, the interfaces opened, and the controllers setup.
	 * </ul>
	 * @exception CCDLibraryFormatException Thrown if the configuration properties cannot be determined.
	 * @exception CCDLibraryNativeException Thrown if the call to open or setup the CCD controllers fails.
	 * @exception Exception Thrown if an error occurs.
	 * @see #redCCD
	 * @see #blueCCD
	 * @see #status
	 * @see FrodoSpecStatus#getProperty
	 * @see FrodoSpecStatus#getPropertyInteger
	 * @see FrodoSpecStatus#getPropertyBoolean
	 * @see FrodoSpecStatus#getPropertyDouble
	 * @see ngat.frodospec.ccd.CCDLibrary#loadTypeFromString
	 * @see ngat.frodospec.ccd.CCDLibrary#initialise
	 * @see ngat.frodospec.ccd.CCDLibrary#setTextPrintLevel
	 * @see ngat.frodospec.ccd.CCDLibrary#interfaceOpen
	 * @see ngat.frodospec.ccd.CCDLibrary#setup
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 * @see FrodoSpecConstants#ARM_STRING_LIST
	 */
	public void startupCCDController() throws CCDLibraryFormatException, CCDLibraryNativeException, 
					       Exception
	{
		CCDLibrary ccd = null;
		int deviceNumber,textPrintLevel;
		int pciLoadType,timingLoadType,timingApplicationNumber,utilityLoadType,utilityApplicationNumber,gain;
		int startExposureClearTime,startExposureOffsetTime,readoutRemainingTime;
		boolean gainSpeed,idle,enable;
		double targetTemperature;
		String deviceString,pciFilename,timingFilename,utilityFilename,devicePathname;

		for(int arm = FrodoSpecConfig.RED_ARM; arm <= FrodoSpecConfig.BLUE_ARM; arm++)
		{
			// get the relevant configuration information from the FrodoSpec configuration file.
			// CCDLibraryFormatException is caught and re-thrown by this method.
			// Other exceptions (NumberFormatException) are not caught here, 
			// but by the calling method catch(Exception e)
			try
			{
				log(Logger.VERBOSITY_VERY_TERSE,
				    ":startupCCDController:Getting frodospec.ccd."+
				    FrodoSpecConstants.ARM_STRING_LIST[arm]+".enable");
				// ccd controller
				enable = status.getPropertyBoolean("frodospec.ccd."+
								   FrodoSpecConstants.ARM_STRING_LIST[arm]+".enable");
				log(Logger.VERBOSITY_VERY_TERSE,
				    ":startupCCDController:Getting frodospec.ccd."+
				    FrodoSpecConstants.ARM_STRING_LIST[arm]+".device");
				deviceString = status.getProperty("frodospec.ccd."+
								  FrodoSpecConstants.ARM_STRING_LIST[arm]+".device");
				deviceNumber = CCDLibrary.interfaceDeviceFromString(deviceString);
				devicePathname = status.getProperty("frodospec.ccd."+
								    FrodoSpecConstants.ARM_STRING_LIST[arm]+
								    ".device.pathname");
				textPrintLevel = CCDLibrary.textPrintLevelFromString(
				    status.getProperty("frodospec.ccd."+FrodoSpecConstants.ARM_STRING_LIST[arm]+
						       ".device.text.print_level"));
				pciLoadType = CCDLibrary.loadTypeFromString(status.
				    getProperty("frodospec.ccd."+FrodoSpecConstants.ARM_STRING_LIST[arm]+
						".config.pci_load_type"));
				pciFilename = status.getProperty("frodospec.ccd."+
								 FrodoSpecConstants.ARM_STRING_LIST[arm]+
								 ".config.pci_filename");
				timingLoadType = CCDLibrary.loadTypeFromString(status.
				    getProperty("frodospec.ccd."+FrodoSpecConstants.ARM_STRING_LIST[arm]+
						".config.timing_load_type"));
				timingApplicationNumber = status.getPropertyInteger("frodospec.ccd."+
				    FrodoSpecConstants.ARM_STRING_LIST[arm]+".config.timing_application_number");
				timingFilename = status.getProperty("frodospec.ccd."+
								    FrodoSpecConstants.ARM_STRING_LIST[arm]+
								    ".config.timing_filename");
				utilityLoadType = CCDLibrary.loadTypeFromString(status.
				    getProperty("frodospec.ccd."+FrodoSpecConstants.ARM_STRING_LIST[arm]+
						".config.utility_load_type"));
				utilityApplicationNumber = status.getPropertyInteger("frodospec.ccd."+
				    FrodoSpecConstants.ARM_STRING_LIST[arm]+".config.utility_application_number");
				utilityFilename = status.getProperty("frodospec.ccd."+
								     FrodoSpecConstants.ARM_STRING_LIST[arm]+
								     ".config.utility_filename");
				targetTemperature = status.getPropertyDouble("frodospec.ccd."+
				    FrodoSpecConstants.ARM_STRING_LIST[arm]+".config.temperature.target");
				gain = CCDLibrary.dspGainFromString(status.getProperty("frodospec.ccd."+
				    FrodoSpecConstants.ARM_STRING_LIST[arm]+".config.gain"));
				gainSpeed = status.getPropertyBoolean("frodospec.ccd."+
								      FrodoSpecConstants.ARM_STRING_LIST[arm]+
								      ".config.gain_speed");
				idle = status.getPropertyBoolean("frodospec.ccd."+
								 FrodoSpecConstants.ARM_STRING_LIST[arm]+
								 ".config.idle");
				startExposureClearTime = status.getPropertyInteger("frodospec.ccd."+
				    FrodoSpecConstants.ARM_STRING_LIST[arm]+".config.start_exposure_clear_time");
				startExposureOffsetTime = status.getPropertyInteger("frodospec.ccd."+
				    FrodoSpecConstants.ARM_STRING_LIST[arm]+".config.start_exposure_offset_time");
				readoutRemainingTime = status.getPropertyInteger("frodospec.ccd."+
				    FrodoSpecConstants.ARM_STRING_LIST[arm]+".config.readout_remaining_time");
			}
			catch(CCDLibraryFormatException e)
			{
				error(this.getClass().getName()+":startupCCDController:",e);
				throw e;
			}
			// configure ccd controller
			try
			{
				if(enable)
				{
					if(arm == FrodoSpecConfig.RED_ARM)
						ccd = redCCD;
					else if(arm == FrodoSpecConfig.BLUE_ARM)
						ccd = blueCCD;
					ccd.initialise();
					ccd.setTextPrintLevel(textPrintLevel);
					ccd.interfaceOpen(deviceNumber,devicePathname);
					ccd.setup(pciLoadType,pciFilename,
						  timingLoadType,timingApplicationNumber,timingFilename,
						  utilityLoadType,utilityApplicationNumber,utilityFilename,
						  targetTemperature,gain,gainSpeed,idle);
					// diddly not supported yet
					//libccd.CCDExposureSetStartExposureClearTime(startExposureClearTime);
					//libccd.CCDExposureSetStartExposureOffsetTime(startExposureOffsetTime);
					//libccd.CCDExposureSetReadoutRemainingTime(readoutRemainingTime);
				}// end if enable
			}
			catch (CCDLibraryNativeException e)
			{
				error(this.getClass().getName()+":startupCCDController:CCD:",e);
				throw e;
			}
		}// end for
	}

	/**
	 * Initialise variables used for communicating with the main FrodoSpec PLC.
	 * @exception EIPNativeException Thrown if a PLC error occurs.
	 * @see #status
	 * @see #plc
	 * @see Plc#init
	 */
	public void startupPLC() throws EIPNativeException
	{
		plc.init(status);
		log(Logger.VERBOSITY_VERY_TERSE,":startupPLC:Finished.");
	}

	/**
	 * Startup (initialise) the focus stage. For each focus stage this calls it's init method.
	 * @exception IllegalArgumentException Thrown by the init method.
	 * @exception ArcomESSNativeException Thrown by the init method.
	 * @exception NewmarkNativeException Thrown by the init method.
	 * @see #focusStageList
	 * @see FocusStage#init
	 */
	public void startupFocusStage() throws IllegalArgumentException, ArcomESSNativeException, 
								NewmarkNativeException
	{
		for(int i = FrodoSpecConfig.RED_ARM; i <= FrodoSpecConfig.BLUE_ARM; i++)
		{
			focusStageList[i].init(status,i);
		}
	}

	/**
	 * Close connections to the hardware.
	 * @exception CCDLibraryNativeException Thrown if the CCD device failed to shut down.
	 * @exception EIPNativeException Thrown if a PLC error occurs.
	 * @exception Exception Thrown if the lamp unit turnAllLampsOff method fails.
	 * @see #shutdownCCDController
	 * @see #plc
	 * @see Plc#destroyHandle
	 * @see #lampUnit
	 * @see ngat.lamp.LTAGLampUnit#turnAllLampsOff
	 */
	public void shutdownHardware() throws CCDLibraryNativeException, EIPNativeException, Exception
	{
		shutdownCCDController();
		plc.destroyHandle();
		// focus stage is now closed at the end of each individual operation, no need to close here
		// ensure all lamps are turned off
		lampUnit.turnAllLampsOff();
	}

	/**
	 * Method to shut down the connection to the hardware controllers.
	 * <ul>
	 * <li>The CCD setup is shutdown (memory map), and the interface closed.
	 * </ul>
	 * @exception CCDLibraryNativeException Thrown if the device failed to shut down.
	 * @see #status
	 * @see #redCCD
	 * @see #blueCCD
	 * @see FrodoSpecStatus#getPropertyBoolean
	 * @see ngat.frodospec.ccd.CCDLibrary#setupShutdown
	 * @see ngat.frodospec.ccd.CCDLibrary#interfaceClose
	 */
	public void shutdownCCDController() throws CCDLibraryNativeException
	{
		boolean enable;

		enable = status.getPropertyBoolean("frodospec.ccd.red.enable");
		if(enable)
		{
			redCCD.setupShutdown();
			redCCD.interfaceClose();
		}
		enable = status.getPropertyBoolean("frodospec.ccd.blue.enable");
		if(enable)
		{
			blueCCD.setupShutdown();
			blueCCD.interfaceClose();
		}
	}

	/**
	 * This is the run routine. It starts a new server to handle incoming requests, and waits for the
	 * server to terminate. A thread monitor is also started if it was requested from the command line.
	 * @see #server
	 * @see #frodospecPortNumber
	 * @see #titServer
	 * @see #titPortNumber
	 * @see #threadMonitor
	 */
	private void run()
	{
		int threadMonitorUpdateTime;
		Date nowDate = null;

		server = new FrodoSpecTCPServer("FrodoSpec",frodospecPortNumber);
		server.setFrodoSpec(this);
		server.setPriority(status.getThreadPriorityServer());
		titServer = new TitServer("TitServer on port "+titPortNumber,titPortNumber);
		titServer.setPriority(status.getThreadPriorityTIT());
		nowDate = new Date();
		log(Logger.VERBOSITY_VERY_TERSE,
			this.getClass().getName()+":run:server started at:"+nowDate.toString());
		log(Logger.VERBOSITY_VERY_TERSE,
			this.getClass().getName()+":run:server started on port:"+frodospecPortNumber);
		error(this.getClass().getName()+":run:server started at:"+nowDate.toString());
		error(this.getClass().getName()+":run:server started on port:"+frodospecPortNumber);
		if(threadMonitor)
		{
			threadMonitorFrame = new ThreadMonitorFrame(this.getClass().getName());
			threadMonitorFrame.pack();
			threadMonitorFrame.setVisible(true);
			try
			{
				threadMonitorUpdateTime = status.getPropertyInteger("frodospec.thread_monitor.update_time");
			}
			catch(NumberFormatException e)
			{
				error(this.getClass().getName()+":run:getting thread monitor update time:",e);
				threadMonitorUpdateTime  = 1000;
			}
			threadMonitorFrame.getThreadMonitor().setUpdateTime(threadMonitorUpdateTime);
		}
		server.start();
		titServer.start();
		try
		{
			server.join();
		}
		catch(InterruptedException e)
		{
			error(this.getClass().getName()+":run:",e);
		}
	}

	/**
	 * Routine to be called at the end of execution of FrodoSpec to close down communications.
	 * Currently closes CCDLibrary, FrodoSpecTCPServer and TitServer.
	 * @see FrodoSpecTCPServer#close
	 * @see #server
	 * @see TitServer#close
	 * @see #titServer
	 * @see #shutdownHardware
	 */
	public void close()
	{
		try
		{
			shutdownHardware();
		}
		catch(Exception e)
		{
			error(this.getClass().getName()+":close:",e);
		}
		server.close();
		titServer.close();
	}

	/**
	 * Get Socket Server instance.
	 * @return The server instance.
	 * @see #server
	 * @see FrodoSpecTCPServer
	 */
	public FrodoSpecTCPServer getServer()
	{
		return server;
	}

	/**
	 * Get Fits filename generation object instance.
	 * @param arm Which FitsFilename to return (which arm). Either FrodoSpecConfig.RED_ARM
	 *        or FrodoSpecConfig.BLUE_ARM.
	 * @return The FrodoSpec FitsFilename fitsFilename instance.
	 * @exception IllegalArgumentException Thrown if the index is out of range.
	 */
	public FitsFilename getFitsFilename(int arm) throws IllegalArgumentException
	{
		if((arm < FrodoSpecConfig.RED_ARM)||(arm > FrodoSpecConfig.BLUE_ARM))
		{
			throw new IllegalArgumentException(this.getClass().getName()+":getFitsFilename:Arm "+arm+
							   " out of range.");
		}
		return fitsFilenameList[arm];
	}

	/**
	 * Get ccd instance.
	 * @param arm Which arm. Either FrodoSpecConfig.RED_ARM or FrodoSpecConfig.BLUE_ARM.
	 * @return The ccd instance.
	 * @exception IllegalArgumentException Thrown if the arm is out of range.
	 * @see #redCCD
	 * @see #blueCCD
	 * @see ngat.frodospec.ccd.CCDLibrary
	 */
	public CCDLibrary getCCD(int arm) throws IllegalArgumentException
	{
		if(arm == FrodoSpecConfig.RED_ARM)
			return redCCD;
		else if(arm == FrodoSpecConfig.BLUE_ARM)
			return blueCCD;
		else
			throw new IllegalArgumentException(this.getClass().getName()+":Illegal arm number "+arm+".");
	}

	/**
	 * Get focus stage instance.
	 * @param arm Which arm. Either FrodoSpecConfig.RED_ARM or FrodoSpecConfig.BLUE_ARM.
	 * @return The FocusStage instance.
	 * @exception IllegalArgumentException Thrown if the arm is out of range.
	 * @see #focusStageList
	 * @see FocusStage
	 */
	public FocusStage getFocusStage(int arm) throws IllegalArgumentException
	{
		if((arm == FrodoSpecConfig.RED_ARM)||(arm == FrodoSpecConfig.BLUE_ARM))
			return focusStageList[arm];
		else
			throw new IllegalArgumentException(this.getClass().getName()+":Illegal arm number "+arm+".");
	}

	/**
	 * Get the PLC instance.
	 * @return The Plc instance.
	 * @see #plc
	 * @see ngat.frodospec.Plc
	 */
	public Plc getPLC()
	{
		return plc;
	}

	/**
	 * Get the lamp unit instance. This can be used to get lamp status information (fluxs etc).
	 * Turning on and off the lamp should be done using the lamp controller, which will synchronise
	 * requests across the two arms correctly.
	 * @return The lamp unit instance.
	 * @see #lampUnit
	 * @see #getLampController
	 */
	public LTAGLampUnit getLampUnit()
	{
		return lampUnit;
	}

	/**
	 * Get the lamp controller instance.
	 * @return The lamp controller instance.
	 * @see #lampController
	 * @see ngat.frodospec.LampController
	 */
	public LampController getLampController()
	{
		return lampController;
	}

	/**
	 * Get libngatfitsList instance. 
	 * There is an instance of the ngat.fits.FitsHeader class for each arm in this application.
	 * It is used to write FITS header cards to disk, ready to append the relevant data to it.
	 * @param arm Which arm. Either FrodoSpecConfig.RED_ARM or FrodoSpecConfig.BLUE_ARM.
	 * @return The FitsHeader instance.
	 * @exception IllegalArgumentException Thrown if the arm is out of range.
	 * @see ngat.fits.FitsHeader
	 * @see #libngatfitsList
	 */
	public FitsHeader getFitsHeader(int arm) throws IllegalArgumentException
	{
		if((arm != FrodoSpecConfig.RED_ARM)&&(arm != FrodoSpecConfig.BLUE_ARM))
		{
			throw new IllegalArgumentException(this.getClass().getName()+":getFitsHeader:Arm "+arm+
							   " out of range.");
		}
		return libngatfitsList[arm];
	}

	/**
	 * Get FitsHeaderDefaults instance. There is an instance of the ngat.fits.FitsHeaderDefaults 
	 * class for each arm in this application.
	 * It is used to get defaults values for field in the FITS headers the application writes to disk.
	 * @param arm Which arm. Either FrodoSpecConfig.RED_ARM or FrodoSpecConfig.BLUE_ARM.
	 * @return The FitsHeaderDefaults instance.
	 * @exception IllegalArgumentException Thrown if the arm is out of range.
	 * @see #fitsHeaderDefaultsList
	 */
	public FitsHeaderDefaults getFitsHeaderDefaults(int arm) throws IllegalArgumentException
	{
		if((arm != FrodoSpecConfig.RED_ARM)&&(arm != FrodoSpecConfig.BLUE_ARM))
		{
			throw new IllegalArgumentException(this.getClass().getName()+":getFitsHeaderDefaults:Arm "+arm+
							   " out of range.");
		}
		return fitsHeaderDefaultsList[arm];
	}

	/**
	 * Get status instance.
	 * @return The status instance.
	 * @see #status
	 * @see FrodoSpecStatus
	 */
	public FrodoSpecStatus getStatus()
	{
		return status;
	}

	/**
	 * This routine returns an instance of the sub-class of CommandImplementation that
	 * implements the command with class name commandClassName. If an implementation is
	 * not found or an instance cannot be created, an instance of UnknownCommandImplementation is returned instead.
	 * The instance is constructed 
	 * using a null argument constructor, from the Class object stored in the implementationList.
	 * @param commandClassName The class-name of a COMMAND sub-class.
	 * @return A new instance of a sub-class of CommandImplementation that implements the 
	 * 	command, or an instance of UnknownCommandImplementation.
	 */
	public JMSCommandImplementation getImplementation(String commandClassName)
	{
		JMSCommandImplementation unknownCommandImplementation = new UnknownCommandImplementation();
		JMSCommandImplementation object = null;
		Class cl = null;

		cl = (Class)implementationList.get(commandClassName);
		if(cl != null)
		{
			try
			{
				object = (JMSCommandImplementation)cl.newInstance();
			}
			catch(InstantiationException e)//Class.newInstance exception
			{
				error(this.getClass().getName()+":getImplementation:Class "+
					cl.getName()+":InstantiationException:",e);
				object = null;
			}
			catch(IllegalAccessException e)//Class.newInstance exception
			{
				error(this.getClass().getName()+":getImplementation:Class "+
					cl.getName()+":IllegalAccessException:",e);
				object = null;
			}
		}// end if found class
		if(object != null)
			return object;
		else
			return unknownCommandImplementation;
	}

	/**
	 * Method to set the level of logging filtered. The status, error and log loggers, and filters associated with
	 * the CCDLibrary, EIPPLC, FocusStage, lamp unit objects all have their filters set.
	 * @param level An integer, used as a verbosity level.
	 * @see #status
	 * @see #logLogger
	 * @see #errorLogger
	 * @see #redCCD
	 * @see #blueCCD
	 * @see #plc
	 * @see #focusStageList
	 * @see #lampUnit
	 * @see FocusStage#setLogLevel
	 * @see Plc#setLogLevel
	 * @see ngat.frodospec.FrodoSpecStatus#setLogLevel
	 * @see ngat.frodospec.ccd.CCDLibrary#setLogFilterLevel
	 * @see ngat.lamp.LTAGLampUnit#setLogLevel
	 */
	public void setLogLevel(int level)
	{
		status.setLogLevel(logLevel);
		logLogger.setLogLevel(logLevel);
		errorLogger.setLogLevel(logLevel);
		redCCD.setLogFilterLevel(level);
		blueCCD.setLogFilterLevel(level);
		plc.setLogLevel(level);
		for(int i = FrodoSpecConfig.RED_ARM; i <= FrodoSpecConfig.BLUE_ARM; i++)
		{
			focusStageList[i].setLogLevel(level);
		}
		if(lampUnit != null)
			lampUnit.setLogLevel(level);
	}

	/**
	 * Routine to send a command from the instrument (this application/FrodoSpec) to the ISS. The routine
	 * waits until the command's done message has been returned from the ISS and returns this.
	 * If the commandThread is aborted this also stops waiting for the done message to be returned.
	 * @param command The command to send to the ISS.
	 * @param commandThread The thread the passed in command (and this method) is running on.
	 * @return The done message returned from te ISS, or an error message created by this routine
	 * 	if the done was null.
	 * @see #issAddress
	 * @see #issPortNumber
	 * @see #sendISSCommand(INST_TO_ISS,FrodoSpecTCPServerConnectionThread,boolean)
	 * @see FrodoSpecTCPClientConnectionThread
	 * @see FrodoSpecTCPServerConnectionThread#getAbortProcessCommand
	 */
	public INST_TO_ISS_DONE sendISSCommand(INST_TO_ISS command,FrodoSpecTCPServerConnectionThread commandThread)
	{
		return sendISSCommand(command,commandThread,true);
	}

	/**
	 * Routine to send a command from the instrument (this application/FrodoSpec) to the ISS. The routine
	 * waits until the command's done message has been returned from the ISS and returns this.
	 * If checkAbort is set and the commandThread is aborted this also stops waiting for the 
	 * done message to be returned.
	 * @param command The command to send to the ISS.
	 * @param commandThread The thread the passed in command (and this method) is running on.
	 * @param checkAbort A boolean, set to true if we want to check for commandThread aborting.
	 * 	This should be set to false when the command is being sent to the ISS in response
	 * 	to an abort occuring.
	 * @return The done message returned from te ISS, or an error message created by this routine
	 * 	if the done was null.
	 * @see #issAddress
	 * @see #issPortNumber
	 * @see FrodoSpecTCPClientConnectionThread
	 * @see FrodoSpecTCPServerConnectionThread#getAbortProcessCommand
	 */
	public INST_TO_ISS_DONE sendISSCommand(INST_TO_ISS command,FrodoSpecTCPServerConnectionThread commandThread,
		boolean checkAbort)
	{
		FrodoSpecTCPClientConnectionThread thread = null;
		INST_TO_ISS_DONE done = null;
		boolean finished = false;

		log(Logger.VERBOSITY_VERY_TERSE,
			this.getClass().getName()+":sendISSCommand:"+command.getClass().getName());
		thread = new FrodoSpecTCPClientConnectionThread(issAddress,issPortNumber,command,commandThread);
		thread.setFrodoSpec(this);
		thread.start();
		finished = false;
		while(finished == false)
		{
			try
			{
				thread.join(100);// wait 100 millis for the thread to finish
			}
			catch(InterruptedException e)
			{
				error("run:join interrupted:",e);
			}
		// If the thread has finished so has this loop
			finished = (thread.isAlive() == false);
		// check if the thread has been aborted, if checkAbort has been set.
			if(checkAbort)
			{
			// If the commandThread has been aborted, stop processing this thread
				if(commandThread.getAbortProcessCommand())
					finished = true;
			}
		}
		done = (INST_TO_ISS_DONE)thread.getDone();
		if(done == null)
		{
			// one reason the done is null is if we escaped from the loop
			// because the FrodoSpec server thread was aborted.
			if(commandThread.getAbortProcessCommand())
			{
				done = new INST_TO_ISS_DONE(command.getId());
				error(this.getClass().getName()+":sendISSCommand:"+
					command.getClass().getName()+":Server thread Aborted");
				done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+1);
				done.setErrorString("sendISSCommand:Server thread Aborted:"+
					command.getClass().getName());
				done.setSuccessful(false);		
			}
			else // a communication failure occured
			{
				done = new INST_TO_ISS_DONE(command.getId());
				error(this.getClass().getName()+":sendISSCommand:"+
					command.getClass().getName()+":Getting Done failed");
				done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2);
				done.setErrorString("sendISSCommand:Getting Done failed:"+
					command.getClass().getName());
				done.setSuccessful(false);
			}
		}
		log(Logger.VERBOSITY_VERY_TERSE,
			"Done:"+done.getClass().getName()+":successful:"+done.getSuccessful()+
			":error number:"+done.getErrorNum()+":error string:"+done.getErrorString());
		return done;
	}

	/**
	 * Routine to send a command from the instrument (this application/FrodoSpec) to the DP(RT). The routine
	 * waits until the command's done message has been returned from the DP(RT) and returns this.
	 * If the commandThread is aborted this also stops waiting for the done message to be returned.
	 * @param command The command to send to the DP(RT).
	 * @param commandThread The thread the passed in command (and this method) is running on.
	 * @return The done message returned from te DP(RT), or an error message created by this routine
	 * 	if the done was null.
	 * @see #dprtAddress
	 * @see #dprtPortNumber
	 * @see FrodoSpecTCPClientConnectionThread
	 * @see FrodoSpecTCPServerConnectionThread#getAbortProcessCommand
	 */
	public INST_TO_DP_DONE sendDpRtCommand(INST_TO_DP command,FrodoSpecTCPServerConnectionThread commandThread)
	{
		FrodoSpecTCPClientConnectionThread thread = null;
		INST_TO_DP_DONE done = null;
		boolean finished = false;

		log(Logger.VERBOSITY_VERY_TERSE,
			this.getClass().getName()+":sendDpRtCommand:"+command.getClass().getName());
		thread = new FrodoSpecTCPClientConnectionThread(dprtAddress,dprtPortNumber,command,commandThread);
		thread.setFrodoSpec(this);
		thread.start();
		finished = false;
		while(finished == false)
		{
			try
			{
				thread.join(100);// wait 100 millis for the thread to finish
			}
			catch(InterruptedException e)
			{
				error("run:join interrupted:",e);
			}
		// If the thread has finished so has this loop
			finished = (thread.isAlive() == false);
		// If the commandThread has been aborted, stop processing this thread
			if(commandThread.getAbortProcessCommand())
				finished = true;
		}
		done = (INST_TO_DP_DONE)thread.getDone();
		if(done == null)
		{
			// one reason the done is null is if we escaped from the loop
			// because the FrodoSpec server thread was aborted.
			if(commandThread.getAbortProcessCommand())
			{
				done = new INST_TO_DP_DONE(command.getId());
				error(this.getClass().getName()+":sendDpRtCommand:"+
					command.getClass().getName()+":Server thread Aborted");
				done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+3);
				done.setErrorString("sendDpRtCommand:Server thread Aborted:"+
					command.getClass().getName());
				done.setSuccessful(false);
			}
			else // a communication failure occured
			{
				done = new INST_TO_DP_DONE(command.getId());
				error(this.getClass().getName()+":sendDpRtCommand:"+
					command.getClass().getName()+":Getting Done failed");
				done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+4);
				done.setErrorString("sendDpRtCommand:Getting Done failed:"+
					command.getClass().getName());
				done.setSuccessful(false);
			}
		}
		log(Logger.VERBOSITY_VERY_TERSE,
			"Done:"+done.getClass().getName()+":successful:"+done.getSuccessful()+
			":error number:"+done.getErrorNum()+":error string:"+done.getErrorString());
		return done;
	}

	/**
	 * Routine to write the string to the relevant logger. If the relevant logger has not been
	 * created yet the error gets written to System.out.
	 * @param level The level of logging this message belongs to.
	 * @param s The string to write.
	 * @see #logLogger
	 */
	public void log(int level,String s)
	{
		if(logLogger != null)
			logLogger.log(level,s);
		else
		{
			if((status.getLogLevel()&level) > 0)
				System.out.println(s);
		}
	}

	/**
	 * Routine to write the string to the relevant logger. If the relevant logger has not been
	 * created yet the error gets written to System.err.
	 * @param s The string to write.
	 * @see #errorLogger
	 */
	public void error(String s)
	{
		if(errorLogger != null)
			errorLogger.log(Logger.VERBOSITY_VERY_TERSE,s);
		else
			System.err.println(s);
	}

	/**
	 * Routine to write the string to the relevant logger. If the relevant logger has not been
	 * created yet the error gets written to System.err.
	 * @param s The string to write.
	 * @param e An exception that caused the error to occur.
	 * @see #errorLogger
	 */
	public void error(String s,Exception e)
	{
		if(errorLogger != null)
		{
			errorLogger.log(Logger.VERBOSITY_VERY_TERSE,s,e);
			errorLogger.dumpStack(Logger.VERBOSITY_VERY_TERSE,e);
		}
		else
		{
			System.err.println(s);
			e.printStackTrace(System.err);
		}
	}

	/**
	 * Help message routine. Prints all command line arguments.
	 */
	public void help()
	{
		System.out.println("FrodoSpec Help:");
		System.out.println("FrodoSpec is the 'FrodoSpec Control System', which controls: ");
		System.out.println("\tTwo SDSU CCD controllers.");
		System.out.println("Arguments are:");
		System.out.println("\t-p[ort] <port number> - Port to wait for client connections on.");
		System.out.println("\t-t[itport] <port number> - Port to wait for Telescope Image Transfer "+
					"client connections on.");
		System.out.println("\t-i[ssport] <port number> - Port to send ISS commands to.");
		System.out.println("\t-[issip]|[issaddress] <address> - Address to send ISS commands to.");
		System.out.println("\t-d[prtport] <port number> - Port to send DP(RT) commands to.");
		System.out.println("\t-[dprtip]|[dprtaddress] <address> - Address to send DP(RT) commands to.");
		System.out.println("\t-[nc]|[netconfig] <filename> - Location of FrodoSpec network "+
			"configuration properties file.");
		System.out.println("\t-[co]|[config] <filename> - Location of FrodoSpec configuration properties file.");
		System.out.println("\t-f[itsconfig] <filename> - Location of FITS header defaults "+
			"configuration properties file.");
		System.out.println("\t-l[og] <log level> - log level.");
		System.out.println("\t-threadmonitor - show thread monitor.");
	}

	/**
	 * Parse the arguments.
	 * @param args The string list of arguments.
	 * @see #logLevel
	 * @see #frodospecPortNumber
	 * @see #issPortNumber
	 * @see #issPortNumber
	 * @see #issAddress
	 * @see #dprtPortNumber
	 * @see #dprtAddress
	 * @see #netConfigurationFilename
	 * @see #configurationFilename
	 * @see #fitsHeaderDefaultsFilename
	 * @see #titPortNumber
	 * @see #threadMonitor
	 */
	private void parseArguments(String args[])
	{
		for(int i = 0; i < args.length;i++)
		{
			if(args[i].equals("-l")||args[i].equals("-log"))
			{
				if((i+1)< args.length)
				{
					logLevel = Integer.parseInt(args[i+1]);
					i++;
				}
				else
					System.err.println("-log requires a log level");
			}
			else if(args[i].equals("-p")||args[i].equals("-port"))
			{
				if((i+1)< args.length)
				{
					frodospecPortNumber = Integer.parseInt(args[i+1]);
					i++;
				}
				else
					System.err.println("-port requires a port number");
			}
			else if(args[i].equals("-i")||args[i].equals("-issport"))
			{
				if((i+1)< args.length)
				{
					issPortNumber = Integer.parseInt(args[i+1]);
					i++;
				}
				else
					System.err.println("-issport requires a port number");
			}
			else if(args[i].equals("-issip")||args[i].equals("-issaddress"))
			{
				if((i+1)< args.length)
				{
					try
					{
						issAddress = InetAddress.getByName(args[i+1]);
					}
					catch(UnknownHostException e)
					{
						System.err.println("FrodoSpec:argument parser:illegal ISS address:"+
							args[i+1]+":"+e);
					}
					i++;
				}
				else
					System.err.println("-issaddress requires a valid ip address");
			}
			else if(args[i].equals("-d")||args[i].equals("-dprtport"))
			{
				if((i+1)< args.length)
				{
					dprtPortNumber = Integer.parseInt(args[i+1]);
					i++;
				}
				else
					System.err.println("-dprtport requires a port number");
			}
			else if(args[i].equals("-dprtip")||args[i].equals("-dprtaddress"))
			{
				if((i+1)< args.length)
				{
					try
					{
						dprtAddress = InetAddress.getByName(args[i+1]);
					}
					catch(UnknownHostException e)
					{
						System.err.println("FrodoSpec:argument parser::illegal DP(RT) address:"+
							args[i+1]+":"+e);
					}
					i++;
				}
				else
					System.err.println("-dprtaddress requires a valid ip address");
			}
			else if(args[i].equals("-nc")||args[i].equals("-netconfig"))
			{
				if((i+1)< args.length)
				{
					netConfigurationFilename = new String(args[i+1]);
					i++;
				}
				else
					System.err.println("-netconfig requires a filename");
			}
			else if(args[i].equals("-co")||args[i].equals("-config"))
			{
				if((i+1)< args.length)
				{
					configurationFilename = new String(args[i+1]);
					i++;
				}
				else
					System.err.println("-config requires a filename");
			}
			else if(args[i].equals("-f")||args[i].equals("-fitsconfig"))
			{
				if((i+1)< args.length)
				{
					fitsHeaderDefaultsFilename = new String(args[i+1]);
					i++;
				}
				else
					System.err.println("-fitsconfig requires a filename");
			}
			else if(args[i].equals("-t")||args[i].equals("-titport"))
			{
				if((i+1)< args.length)
				{
					titPortNumber = Integer.parseInt(args[i+1]);
					i++;
				}
				else
					System.err.println("-titport requires a port number");
			}
			else if(args[i].equals("-threadmonitor"))
			{
				threadMonitor = true;
			}
			else if(args[i].equals("-h")||args[i].equals("-help"))
			{
				help();
				System.exit(0);
			}
			else
				System.err.println("FrodoSpec '"+args[i]+"' not a recognised option");
		}// end for
	}

	/**
	 * Routine that checks whether the arguments loaded from the property files and set using the arguments
	 * are sensible.
	 * @see #frodospecPortNumber
	 * @see #issAddress
	 * @see #issPortNumber
	 * @see #dprtAddress
	 * @see #dprtPortNumber
	 * @see #titPortNumber
	 * @exception Exception Thrown when an argument is not acceptable.
	 */
	private void checkArgs() throws Exception
	{
		if(issAddress == null)
		{
			help();
			throw new Exception("No ISS Address Specified.");
		}
		if(dprtAddress == null)
		{
			help();
			throw new Exception("No DP(RT) Address Specified.");
		}
		if((frodospecPortNumber < MINIMUM_PORT_NUMBER)||(frodospecPortNumber > MAXIMUM_PORT_NUMBER))
		{
			throw new Exception("Server Port Number '"+frodospecPortNumber+"' out of range.");
		}
		if((issPortNumber < MINIMUM_PORT_NUMBER)||(issPortNumber > MAXIMUM_PORT_NUMBER))
		{
			throw new Exception("ISS Port Number '"+issPortNumber+"' out of range.");
		}
		if((dprtPortNumber < MINIMUM_PORT_NUMBER)||(dprtPortNumber > MAXIMUM_PORT_NUMBER))
		{
			throw new Exception("DP(RT) Port Number '"+dprtPortNumber+"' out of range.");
		}
		if((titPortNumber < MINIMUM_PORT_NUMBER)||(titPortNumber > MAXIMUM_PORT_NUMBER))
		{
			throw new Exception("TIT Server Port Number '"+titPortNumber+"' out of range.");
		}
	}

	/**
	 * The main routine, called when FrodoSpec is executed. This createsa new instance of the FrodoSpec class.
	 * It calls the following methods:
	 * <ul>
	 * <li>Calls parseArguments.
	 * <li>init.
	 * <li>checkArgs.
	 * <li>startupHardware.
	 * <li>run.
	 * </ul>
	 * @see #init
	 * @see #parseArguments
	 * @see #checkArgs
	 * @see #startupHardware
	 * @see #run
	 */
	public static void main(String[] args)
	{
		FrodoSpec frodospec = new FrodoSpec();

		try
		{
			frodospec.parseArguments(args);
			frodospec.init();
		}
		catch(Exception e)
		{
			frodospec.error("main:init failed:",e);
			System.exit(1);
		}
		try
		{
			frodospec.checkArgs();
		}
		catch(Exception e)
		{
			frodospec.error("main:checkArgs failed:",e);
			System.exit(1);
		}
		try
		{
			frodospec.startupHardware();
		}
		catch(Exception e)
		{
 			frodospec.error("main:startupHardware failed:",e);
			System.exit(1);
		}
		frodospec.run();
	// We get here if the server thread has terminated. If it has been quit
	// this is a successfull termination, otherwise an error has occured.
	// Note the program can also be terminated from within a REBOOT call.
		if(frodospec.server.getQuit() == false)
			System.exit(1);
	}
}
//
// $Log: not supported by cvs2svn $
// Revision 1.6  2009/02/06 15:28:04  cjm
// Fixed comment.
//
// Revision 1.5  2009/02/05 11:38:59  cjm
// Swapped Bitwise for Absolute logging levels.
//
// Revision 1.4  2008/12/15 18:04:29  cjm
// Added ngat.lamp to list of handlers to copy.
//
// Revision 1.3  2008/12/05 12:16:38  cjm
// Added DatagramLogHandler support.
//
// Revision 1.2  2008/12/05 11:50:08  cjm
// Added logger channel IDs.
//
// Revision 1.1  2008/11/20 11:33:35  cjm
// Initial revision
//
//

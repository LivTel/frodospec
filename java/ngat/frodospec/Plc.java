// Plc.java
package ngat.frodospec;

import java.lang.*;

import ngat.eip.*;
import ngat.phase2.*;
import ngat.util.*;
import ngat.util.logging.*;

/**
 * An instance of this class is used to control the FrodoSpec Plc.
 * @author Chris Mottram
 * @version $Revision: 1.4 $
 */
public class Plc
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: Plc.java,v 1.4 2009-09-02 16:35:55 cjm Exp $");
	/**
	 * The number of temperature probes in FrodoSpec.
	 */
	public final static int TEMPERATURE_PROBE_COUNT = 5;
	/**
	 * Mechanism status bit.
	 */
	public final static int MECH_STATUS_GRATING_RED_IN_TRANSIT         = (1<<0);
	/**
	 * Mechanism status bit.
	 */
	public final static int MECH_STATUS_GRATING_BLUE_IN_TRANSIT        = (1<<1);
	/**
	 * Mechanism status bit.
	 */
	public final static int MECH_STATUS_SHUTTER_RED_IN_TRANSIT         = (1<<2);
	/**
	 * Mechanism status bit.
	 */
	public final static int MECH_STATUS_SHUTTER_BLUE_IN_TRANSIT        = (1<<3);
	/**
	 * Mechanism status bit.
	 */
	public final static int MECH_STATUS_GRATING_RED_IN_POSITION_HIGH   = (1<<4);
	/**
	 * Mechanism status bit.
	 */
	public final static int MECH_STATUS_GRATING_RED_IN_POSITION_LOW    = (1<<5);
	/**
	 * Mechanism status bit.
	 */
	public final static int MECH_STATUS_GRATING_BLUE_IN_POSITION_HIGH  = (1<<6);
	/**
	 * Mechanism status bit.
	 */
	public final static int MECH_STATUS_GRATING_BLUE_IN_POSITION_LOW   = (1<<7);
	/**
	 * Mechanism status bit.
	 */
	public final static int MECH_STATUS_SHUTTER_RED_OPEN               = (1<<8);
	/**
	 * Mechanism status bit.
	 */
	public final static int MECH_STATUS_SHUTTER_RED_CLOSED             = (1<<9);
	/**
	 * Mechanism status bit.
	 */
	public final static int MECH_STATUS_SHUTTER_BLUE_OPEN              = (1<<10);
	/**
	 * Mechanism status bit.
	 */
	public final static int MECH_STATUS_SHUTTER_BLUE_CLOSED            = (1<<11);
	/**
	 * Mechanism status bit.
	 */
	public final static int MECH_STATUS_PANEL_IN_LOCAL                 = (1<<12);
	/**
	 * Fault status bit.
	 */
	public final static int FAULT_STATUS_AIR_PRESSURE_HIGH          = (1<<0);
	/**
	 * Fault status bit.
	 */
	public final static int FAULT_STATUS_AIR_PRESSURE_LOW           = (1<<1);
	/**
	 * Fault status bit.
	 */
	public final static int FAULT_STATUS_HUMIDITY_HIGH              = (1<<2);
	/**
	 * Fault status bit.
	 */
	public final static int FAULT_STATUS_GRATING_POSITION_RED_HIGH  = (1<<3);
	/**
	 * Fault status bit.
	 */
	public final static int FAULT_STATUS_GRATING_POSITION_RED_LOW   = (1<<4);
	/**
	 * Fault status bit.
	 */
	public final static int FAULT_STATUS_GRATING_POSITION_BLUE_HIGH = (1<<5);
	/**
	 * Fault status bit.
	 */
	public final static int FAULT_STATUS_GRATING_POSITION_BLUE_LOW  = (1<<6);
	/**
	 * Fault status bit.
	 */
	public final static int FAULT_STATUS_SHUTTER_RED_OPEN           = (1<<7);
	/**
	 * Fault status bit.
	 */
	public final static int FAULT_STATUS_SHUTTER_RED_CLOSE          = (1<<8);
	/**
	 * Fault status bit.
	 */
	public final static int FAULT_STATUS_SHUTTER_BLUE_OPEN          = (1<<9);
	/**
	 * Fault status bit.
	 */
	public final static int FAULT_STATUS_SHUTTER_BLUE_CLOSE         = (1<<10);
	/**
	 * Fault status bit.
	 */
	public final static int FAULT_STATUS_COOLING                    = (1<<11);
	/**
	 * Fault status bit.
	 */
	public final static int FAULT_STATUS_INST_TEMPERATURE_HIGH      = (1<<12);
	/**
	 * Fault status bit.
	 */
	public final static int FAULT_STATUS_PANEL_TEMPERATURE_HIGH     = (1<<13);
	/**
	 * Fault status bit.
	 */
	public final static int FAULT_STATUS_AIR_FLOW_HIGH              = (1<<14);
	/**
	 * Fault status bit. This indicates the panel has been in local since the last
	 * fault reset. Therefore power to the focus stages has been pulled (to stop them moving)
	 * so a fault reset is needed as part of a reboot that will redatum/home the focus stages.
	 */
	public final static int FAULT_STATUS_PANEL_WAS_IN_LOCAL         = (1<<15);
	/**
	 * The EIPPLC instance used to communicate with the PLC.
	 */
	protected EIPPLC plc = null;
	/**
	 * Connection handle used for comms to the Plc.
	 */
	protected EIPHandle handle = null;
	/**
	 * The thread handling when the connection has been idle for long enough to close it.
	 */
	protected ConnectionIdleThread connectionIdleThread = null;
	/**
	 * How long the conenction can be open and idle (in milliseconds) before it is closed.
	 */
	protected long connectionIdleTime = 5000L;
	/**
	 * Boolean determining whether the we should attempt communication with the Plc.
	 */
	protected boolean enable = false;
	/**
	 * Hostname of the PLC.
	 */
	protected String hostname = null;
	/**
	 * The backplane containing the PLC. Part of the Ethernet/IP addressing.
	 */
	protected int backplane = 1;
	/**
	 * The slot containing the PLC. Part of the Ethernet/IP addressing.
	 */
	protected int slot = 0;
	/**
	 * Which type of device to try to connect to.
	 * @see ngat.eip.EIPPLC#PLC_TYPE_MICROLOGIX1100
	 */
	protected int plcType = EIPPLC.PLC_TYPE_MICROLOGIX1100;
	/**
	 * Whether the PLC is allowed to command movement of mechanisms.
	 */
	protected boolean moveEnable = true;
	/**
	 * Whether we configure the PLC timers and set-points on startup.
	 */
	protected boolean configurationEnable = false;
	/**
	 * The logger to use.
	 */
	protected Logger logger = null;
	/**
	 * Fault Reset Demand PLC Address.
	 */
	protected String faultResetPLCAddress                = new String("N10:0/15");
	/**
	 * Grating Demand PLC Addresses.
	 * In arm numbers, therefore 3 indexes in array, first is empty.
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	protected String gratingDemandPLCAddress[]           = {"","N10:0/0","N10:0/1"};
	/**
	 * Shutter Demand PLC Address.
	 * In arm numbers, therefore 3 indexes in array, first is empty.
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	protected String shutterDemandPLCAddress[]          = {"","N10:0/2","N10:0/3"};
	/**
	 * Ignore SDSU Shutter Demand PLC Address.
	 */
	protected String ignoreSDSUShutterDemandPLCAddress   = new String("N10:0/4");
	/**
	 * Cooling Demand PLC Address.
	 */
	protected String coolingDemandPLCAddress             = new String("N10:0/5");
	/**
	 * Power Control for SDSU PLC Address.
	 * In arm numbers, therefore 3 indexes in array, first is empty.
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	protected String powerDemandSDSUPLCAddress[]        = {"","N10:0/7","N10:0/8"};
	/**
	 * Power Control for Focus Stage PLC Address.
	 * In arm numbers, therefore 3 indexes in array, first is empty.
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	protected String powerDemandFocusStagePLCAddress[]  = {"","N10:0/9","N10:0/10"};
	/**
	 * Power Control for ArcomESS Address.
	 */
	protected String powerDemandArcomESSPLCAddress       = new String("N10:0/11");
	/**
	 * Power Control for Light source PLC Address.
	 */
	protected String powerDemandLightPLCAddress          = new String("N10:0/12");
	/**
	 * Power Control for Maintenance Light PLC Address.
	 */
	protected String powerDemandMaintLightPLCAddress     = new String("N10:0/13");
	/**
	 * PLC Address for configuring the timer for the Grating Low->High movement.
	 * In arm numbers, therefore 3 indexes in array, first (ARM_NONE) is empty.
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	protected String timerGratingLowHighPLCAddress[]       = {"","N10:4","N10:6"};
	/**
	 * PLC Address for configuring the timer for the Grating High->Low movement.
	 * In arm numbers, therefore 3 indexes in array, first is empty.
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	protected String timerGratingHighLowPLCAddress[]       = {"","N10:5","N10:7"};
	/**
	 * PLC Address for configuring the timer for the Shutter Open movement.
	 * In arm numbers, therefore 3 indexes in array, first is empty.
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	protected String timerShutterOpenPLCAddress[]          = {"","N10:8","N10:10"};
	/**
	 * PLC Address for configuring the timer for the Shutter Close movement.
	 * In arm numbers, therefore 3 indexes in array, first is empty.
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	protected String timerShutterClosePLCAddress[]         = {"","N10:9","N10:11"};
	/**
	 * PLC Address for configuring the temperature set point for turning cooling on.
	 */
	protected String setPointCoolingOnPLCAddress            = new String("N10:12");
	/**
	 * PLC Address for configuring the temperature set point for turning cooling off.
	 */
	protected String setPointCoolingOffPLCAddress           = new String("N10:13");
	/**
	 * PLC Address for configuring the high temperature of the instrument set point.
	 */
	protected String setPointHighInstTemperaturePLCAddress  = new String("N10:14");
	/**
	 * PLC Address for configuring the high temperature of the panel set point.
	 */
	protected String setPointHighPanelTemperaturePLCAddress = new String("N10:15");
	/**
	 * Humidity PLC Address.
	 */
	protected String humidityPLCAddress               = new String("F21:0");
	/**
	 * Temperature PLC Address.
	 * @see #TEMPERATURE_PROBE_COUNT
	 */
	protected String[] temperaturePLCAddress = {"F21:1","F21:2","F21:3","F21:4","F21:5"};
	/**
	 * Instrument Temperature PLC Address.
	 */
	protected String instrumentTemperaturePLCAddress = new String("F21:6");
	/**
	 * Panel Temperature PLC Address.
	 */
	protected String panelTemperaturePLCAddress      = new String("F21:7");
	/**
	 * Air Flow PLC Address.
	 */
	protected String airFlowPLCAddress               = new String("F21:8");
	/**
	 * Air Pressure PLC Address.
	 */
	protected String airPressurePLCAddress           = new String("F21:9");
	/**
	 * Cooling time PLC Address.
	 */
	protected String coolingTimePLCAddress           = new String("F21:10");
	/**
	 * PLC Address of linear encoders for grating arms.
	 * In arm numbers, therefore 3 indexes in array, first (ARM_NONE) is empty.
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	protected String linearEncoderPositionPLCAddress[] = {"","F21:11","F21:12"};
	/**
	 * Mechanism Status PLC Address.
	 */
	protected String mechStatusPLCAddress            = new String("N20:1");
	/**
	 * Fault Status PLC Address.
	 */
	protected String faultStatusPLCAddress           = new String("N20:0");
	/**
	 * PLC configuration value: Timer for moving the grating from low to high.
	 * In arm numbers, therefore 3 indexes in array, first is empty.
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	protected int timerMovementLowHighValue[] = {0,5,5};
	/**
	 * PLC configuration value: Timer for moving the grating from high to low.
	 * In arm numbers, therefore 3 indexes in array, first is empty.
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	protected int timerMovementHighLowValue[] = {0,5,5};
	/**
	 * PLC configuration value: Timer for opening the shutter (in 100ths of a second).
	 * In arm numbers, therefore 3 indexes in array, first is empty.
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	protected int timerMovementShutterOpenValue[] = {0,100,100};
	/**
	 * PLC configuration value: Timer for closing the shutter (in 100ths of a second).
	 * In arm numbers, therefore 3 indexes in array, first is empty.
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	protected int timerMovementShutterCloseValue[] = {0,100,100};
	/**
	 * PLC configuration value: Set Point for turning cooling on.
	 */
	protected int coolingSetPointOnValue = 25;
	/**
	 * PLC configuration value: Set Point for turning cooling off.
	 */
	protected int coolingSetPointOffValue = 20;
	/**
	 * PLC configuration value: Set Point for high instrument temperature.
	 */
	protected int coolingSetPointInstrumentHighValue = 30;
	/**
	 * PLC configuration value: Set Point for high panel temperature.
	 */
	protected int coolingSetPointPanelHighValue = 50;
	/**
	 * Internal configuration : Length of time to wait in the setGrating while loop
	 * between plc calls to determine whether the move has finished.
	 * @see #setGrating
	 */
	protected int gratingMoveSleepTime = 100;
	/**
	 * Abort flag set by abort method. Currently used to abort setGrating
	 * as part of a CONFIG abort.
	 */
	protected boolean abortMovement = false;

	/**
	 * Constructor. Also creates plc instance at this point, so we can configure
	 * logging for it before the init method is called.
	 * @see #plc
	 */
	public Plc()
	{
		super();
		plc = new EIPPLC();
	}

	/**
	 * Method to set the hostname. Not used in normal operation - this is usually read from the config.
	 * @param s The hostname as a string i.e. 150.204.240.114 / frodospecplc.
	 * @see #hostname
	 */
	public void setHostname(String s)
	{
		hostname = s;
	}

	/**
	 * Initialise the PLC, and load PLC configuration.
	 * @param status An instance of FrodoSpecStatus. Used to load config from.
	 * @exception IllegalArgumentException Thrown if arm was not either RED_ARM or BLUE_ARM.
	 * @exception EIPNativeException Thrown if creating a handle fails.
	 * @see ngat.eip.EIPPLC#createHandle
	 * @see #loadConfig
	 * @see #plc
	 * @see #handle
	 * @see #configurationEnable
	 * @see #configureTimersSetPoints
	 */
	public void init(FrodoSpecStatus status) throws IllegalArgumentException, EIPNativeException
	{
		// set logger
		logger = LogManager.getLogger("log");
		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":init:Started.");
		// load config
		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":init:Loading configuration.");
		loadConfig(status);
		if(enable)
		{
			handle = plc.createHandle(hostname,backplane,slot,plcType);
			// reset any faults
			faultReset();
			if(configurationEnable)
				configureTimersSetPoints();
		}
		else
		{
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":init:enable was false:PLC not initialised.");
		}
		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+":init:Finished.");
	}

	/**
	 * Configure the various timers and set-points in the PLC, from the loaded defaults.
	 * @exception EIPNativeException Thrown if PLC comms fail.
	 * @see #configurationEnable
	 * @see #plc
	 * @see #handle
	 * @see #open
	 * @see #connectionIdleThread
	 * @see #logger
	 * @see #timerGratingLowHighPLCAddress
	 * @see #timerGratingHighLowPLCAddress
	 * @see #timerShutterOpenPLCAddress
	 * @see #timerShutterClosePLCAddress
	 * @see #setPointCoolingOnPLCAddress
	 * @see #setPointCoolingOffPLCAddress
	 * @see #setPointHighInstTemperaturePLCAddress
	 * @see #timerMovementLowHighValue
	 * @see #timerMovementHighLowValue
	 * @see #timerMovementShutterOpenValue
	 * @see #timerMovementShutterCloseValue
	 * @see #coolingSetPointOnValue
	 * @see #coolingSetPointOffValue
	 * @see #coolingSetPointInstrumentHighValue
	 */
	public void configureTimersSetPoints() throws EIPNativeException
	{
		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":configureTimersSetPoints:Started.");
		if(configurationEnable)
		{
			// ensure connection is opened correctly
			synchronized(handle)
			{
				if(handle.isOpen() == false)
					open();
			}
			connectionIdleThread.setBusy();	
			// grating/shutter movement timers
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":configureTimersSetPoints:Setting Grating/Shutter movement timers.");
			for(int arm = FrodoSpecConfig.RED_ARM; arm <= FrodoSpecConfig.BLUE_ARM; arm++)
			{
				plc.setInteger(handle,timerGratingLowHighPLCAddress[arm],
					       timerMovementLowHighValue[arm]);
				plc.setInteger(handle,timerGratingHighLowPLCAddress[arm],
					       timerMovementHighLowValue[arm]);
				plc.setInteger(handle,timerShutterOpenPLCAddress[arm],
					       timerMovementShutterOpenValue[arm]);
				plc.setInteger(handle,timerShutterClosePLCAddress[arm],
					       timerMovementShutterCloseValue[arm]);
			}
			// cooling set-points
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":configureTimersSetPoints:Setting Cooling set-points.");
			plc.setInteger(handle,setPointCoolingOnPLCAddress,coolingSetPointOnValue);
			plc.setInteger(handle,setPointCoolingOffPLCAddress,coolingSetPointOffValue);
			plc.setInteger(handle,setPointHighInstTemperaturePLCAddress,coolingSetPointInstrumentHighValue);
		}
		else
		{
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":configureTimersSetPoints:Configuration not enabled:"+
				   "Timers and set-points not configured.");
		}
		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":configureTimersSetPoints:Finished.");
	}

	/**
	 * Reset the fault indicator on the PLC.
	 * @exception EIPNativeException Thrown if PLC comms fail.
	 * @see #enable
	 * @see #logger
	 * @see #plc
	 * @see #handle
	 * @see #open
	 * @see #connectionIdleThread
	 * @see #faultResetPLCAddress
	 */
	public void faultReset() throws EIPNativeException
	{
		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":faultReset:Started.");
		if(enable)
		{
			// ensure connection is opened correctly
			synchronized(handle)
			{
				if(handle.isOpen() == false)
					open();
			}
			connectionIdleThread.setBusy();	
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":faultReset:Setting "+faultResetPLCAddress);
			plc.setBoolean(handle,faultResetPLCAddress,true);
		}
		else
		{
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":faultReset:PLC not enabled:Not resetting fault.");
			
		}
		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":faultReset:Finished.");
	}

	/**
	 * Turn enclosure cooling on or off in the PLC.
	 * @param onoff If true turn the cooling on, otherwise turn it off.
	 * @exception EIPNativeException Thrown if PLC comms fail.
	 * @see #enable
	 * @see #logger
	 * @see #plc
	 * @see #handle
	 * @see #connectionIdleThread
	 * @see #open
	 * @see #coolingDemandPLCAddress
	 */
	public void setCooling(boolean onoff) throws EIPNativeException
	{
		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":setCooling:Started.");
		if(enable)
		{
			// ensure connection is opened correctly
			synchronized(handle)
			{
				if(handle.isOpen() == false)
					open();
			}
			connectionIdleThread.setBusy();	
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":setCooling:Setting "+coolingDemandPLCAddress+" to "+onoff);
			plc.setBoolean(handle,coolingDemandPLCAddress,onoff);
		}
		else
		{
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":setCooling:PLC not enabled:Not setting cooling to "+onoff+".");
			
		}
		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":setCooling:Finished.");
	}

	/**
	 * Get the mechanism status word from the PLC.
	 * @return An integer. Various bits are set depending on the status of the grating.
	 *        See MECH_STATUS_GRATING_RED_IN_TRANSIT, MECH_STATUS_GRATING_BLUE_IN_TRANSIT, 
	 *        MECH_STATUS_GRATING_RED_IN_POSITION_HIGH, MECH_STATUS_GRATING_RED_IN_POSITION_LOW,
	 *        MECH_STATUS_GRATING_BLUE_IN_POSITION_HIGH, MECH_STATUS_GRATING_BLUE_IN_POSITION_LOW.
	 * @exception EIPNativeException Thrown if PLC comms fail.
	 * @see #enable
	 * @see #logger
	 * @see #plc
	 * @see #handle
	 * @see #connectionIdleThread
	 * @see #open
	 * @see #mechStatusPLCAddress
	 * @see #printBits
	 * @see #MECH_STATUS_GRATING_RED_IN_TRANSIT
	 * @see #MECH_STATUS_GRATING_BLUE_IN_TRANSIT
	 * @see #MECH_STATUS_GRATING_RED_IN_POSITION_HIGH
	 * @see #MECH_STATUS_GRATING_RED_IN_POSITION_LOW
	 * @see #MECH_STATUS_GRATING_BLUE_IN_POSITION_HIGH
	 * @see #MECH_STATUS_GRATING_BLUE_IN_POSITION_LOW
	 */
	public int getMechanismStatus() throws EIPNativeException
	{
		int mechStatus;

		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":getMechanismStatus:Started.");
		if(enable)
		{
			// ensure connection is opened correctly
			synchronized(handle)
			{
				if(handle.isOpen() == false)
					open();
			}
			connectionIdleThread.setBusy();	
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":getMechanismStatus:Retrieving from: "+mechStatusPLCAddress);
			mechStatus = plc.getInteger(handle,mechStatusPLCAddress);
		}
		else
		{
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":getMechanismStatus:PLC not enabled:Not resetting fault.");
			mechStatus = 0;
		}
		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":getMechanismStatus:Finished with status:"+printBits(mechStatus));
		return mechStatus;
	}

	/**
	 * Get the fault status word from the PLC.
	 * @return An integer. Various bits are set depending on the fault status of the PLC.
	 *        See FAULT_STATUS_AIR_PRESSURE_HIGH, FAULT_STATUS_AIR_PRESSURE_LOW, FAULT_STATUS_HUMIDITY_HIGH,
	 *        FAULT_STATUS_GRATING_POSITION_RED_HIGH, FAULT_STATUS_GRATING_POSITION_RED_LOW,
	 *        FAULT_STATUS_GRATING_POSITION_BLUE_HIGH, FAULT_STATUS_GRATING_POSITION_BLUE_LOW,
	 *        FAULT_STATUS_SHUTTER_POSITION_RED, FAULT_STATUS_SHUTTER_POSITION_BLUE, 
	 *        FAULT_STATUS_COOLING, FAULT_STATUS_TEMPERATURE_HIGH.
	 * @exception EIPNativeException Thrown if PLC comms fail.
	 * @see #enable
	 * @see #logger
	 * @see #plc
	 * @see #handle
	 * @see #connectionIdleThread
	 * @see #open
	 * @see #printBits
	 * @see #faultStatusPLCAddress
	 * @see #FAULT_STATUS_AIR_PRESSURE_HIGH
	 * @see #FAULT_STATUS_AIR_PRESSURE_LOW
	 * @see #FAULT_STATUS_HUMIDITY_HIGH
	 * @see #FAULT_STATUS_GRATING_POSITION_RED_HIGH
	 * @see #FAULT_STATUS_GRATING_POSITION_RED_LOW
	 * @see #FAULT_STATUS_GRATING_POSITION_BLUE_HIGH
	 * @see #FAULT_STATUS_GRATING_POSITION_BLUE_LOW
	 * @see #FAULT_STATUS_SHUTTER_RED_OPEN
	 * @see #FAULT_STATUS_SHUTTER_RED_CLOSE
	 * @see #FAULT_STATUS_SHUTTER_BLUE_OPEN
	 * @see #FAULT_STATUS_SHUTTER_BLUE_CLOSE
	 * @see #FAULT_STATUS_COOLING
	 * @see #FAULT_STATUS_INST_TEMPERATURE_HIGH
	 * @see #FAULT_STATUS_PANEL_TEMPERATURE_HIGH
	 */
	public int getFaultStatus() throws EIPNativeException
	{
		int faultStatus;

		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":getFaultStatus:Started.");
		if(enable)
		{
			// ensure connection is opened correctly
			synchronized(handle)
			{
				if(handle.isOpen() == false)
					open();
			}
			connectionIdleThread.setBusy();	
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":getFaultStatus:Retrieving from: "+faultStatusPLCAddress);
			faultStatus = plc.getInteger(handle,faultStatusPLCAddress);
		}
		else
		{
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":getFaultStatus:PLC not enabled:Not resetting fault.");
			faultStatus = 0;
		}
		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":getFaultStatus:Finished with status:"+printBits(faultStatus));
		return faultStatus;
	}

	/**
	 * Method to set the grating position.
	 * <ul>
	 * <li>if <b>enable</b> is true, we:
	 *     <ul>
	 *     <li>Explicity <b>open</b> a session/connection, to stop many individual open/closes
	 *         in the while loop causing the PLC to fail open session.
	 *     <li>Reset any faults using <b>faultReset</b>
	 *     <li>if <b>moveEnable</b> is true, we:
	 *         <ul>
	 *         <li>Reset the abortMovement flag.
	 *         <li>We select the demand PLC address from the arm, or throw an error is the arm is illegal.
	 *         <li>We work out the transit bit based on arm.
	 *         <li>We work out the in position and fault bit based on arm / resolution.
	 *         <li>We work out the demand value based on resolution.
	 *         <li>We set the PLC demand PLC address to the demand value.
	 *         <li>We enter a loop until completion/error:
	 *             <ul>
	 *             <li>We sleep a bit (<b>gratingMoveSleepTime</b>) to stop overloading the PLC with requests.
	 *             <li>We read the mechanism status using <b>getMechanismStatus</b>.
	 *             <li>We determine if we are in transit and log accordingly
	 *             <li>We determine if we are in position and log accordingly, and set loop termination.
	 *             <li>We determine if the PLC is in local, log and error accordingly. 
	 *             <li>We read the fault status using <b>getFaultStatus</b>.
	 *             <li>If the relevant fault bit is set we log and error accordingly.
	 *             <li>If the abortMovement flag has been set by another (abort) thread 
	 *                 we log and error accordingly.
	 *             </ul>
	 *         </ul>
	 *     <li>else if <b>moveEnable</b> is false we log we havn't moved the grating.
	 *     <li><i>finally</i>, we call <b>close</b> to close the expliticty opened session/connection.
	 *     </ul>
	 * <li>else if <b>enable</b> is false, we log we havn't moved the grating.
	 * </ul>
	 * @param arm Which arm grating to use, either RED_ARM or BLUE_ARM.
	 * @param resolution Which grating we want in the light beam, either RESOLUTION_HIGH or RESOLUTION_LOW.
	 *        RESOLUTION_HIGH is the VPH grating, RESOLUTION_LOW is the normal grating.
	 * @exception EIPNativeException Thrown if PLC comms fail.
	 * @exception IllegalArgumentException Thrown if arm or resolution are not legal values.
	 * @exception Exception Thrown if the relevant 'not in position' PLC fault bit is set whilst moving the
	 *            grating (the PLC thinks the grating move failed).
	 * @see #logger
	 * @see #enable
	 * @see #moveEnable
	 * @see #plc
	 * @see #handle
	 * @see #open
	 * @see #connectionIdleThread
	 * @see #faultReset
	 * @see #getMechanismStatus
	 * @see #getFaultStatus
	 * @see #printBits
	 * @see #gratingDemandPLCAddress
	 * @see #gratingMoveSleepTime
	 * @see #abortMovement
	 * @see #MECH_STATUS_GRATING_RED_IN_TRANSIT
	 * @see #MECH_STATUS_GRATING_BLUE_IN_TRANSIT
	 * @see #MECH_STATUS_GRATING_RED_IN_POSITION_HIGH
	 * @see #MECH_STATUS_GRATING_RED_IN_POSITION_LOW
	 * @see #MECH_STATUS_GRATING_BLUE_IN_POSITION_HIGH
	 * @see #MECH_STATUS_GRATING_BLUE_IN_POSITION_LOW
	 * @see #MECH_STATUS_PANEL_IN_LOCAL
	 * @see #FAULT_STATUS_GRATING_POSITION_RED_HIGH
	 * @see #FAULT_STATUS_GRATING_POSITION_RED_LOW
	 * @see #FAULT_STATUS_GRATING_POSITION_BLUE_HIGH
	 * @see #FAULT_STATUS_GRATING_POSITION_BLUE_LOW
	 * @see FrodoSpecConstants#RESOLUTION_STRING_LIST
	 * @see FrodoSpecConstants#ARM_STRING_LIST
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 * @see ngat.phase2.FrodoSpecConfig#RESOLUTION_HIGH
	 * @see ngat.phase2.FrodoSpecConfig#RESOLUTION_LOW
	 */
	public void setGrating(int arm,int resolution) throws IllegalArgumentException, EIPNativeException, Exception
	{
		String demandPLCAddress = null;
		boolean demandValue,done;
		int mechStatus,transitBit=0,inPositionBit=0,faultStatus,faultBit=0;

		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":setGrating:Started.");
		if(enable)
		{
			// explicity open a session/connection, to stop many individual open/closes
			// in the while loop causing the PLC to fail open session.
			// EIPPLC synchronises plc access on the handle so 
			// this handle can also be used by concurrent
			// GET_STATUS calls without causing threading problems.
			// ensure connection is opened correctly
			synchronized(handle)
			{
				if(handle.isOpen() == false)
					open();
			}
			connectionIdleThread.setBusy();	
			// reset any faults
			faultReset();
			if(moveEnable)
			{
				// reset abort flag
				abortMovement = false;
				// work out PLC address from arm
				if(arm == FrodoSpecConfig.RED_ARM)
					demandPLCAddress = gratingDemandPLCAddress[arm];
				else if(arm == FrodoSpecConfig.BLUE_ARM)
					demandPLCAddress = gratingDemandPLCAddress[arm];
				else
				{
					throw new IllegalArgumentException(this.getClass().getName()+
									   ":setGrating:Illegal arm:"+arm);
				}
				// work out value to write from resolution
				if(resolution == FrodoSpecConfig.RESOLUTION_HIGH)
					demandValue = true;
				else if(resolution == FrodoSpecConfig.RESOLUTION_LOW)
					demandValue = false;
				else
				{
					throw new IllegalArgumentException(this.getClass().getName()+
									":setGrating:Illegal resolution:"+resolution);
				}
				// work out transit / in position / fault bits
				if(arm == FrodoSpecConfig.RED_ARM)
				{
					transitBit = MECH_STATUS_GRATING_RED_IN_TRANSIT;
					if(resolution == FrodoSpecConfig.RESOLUTION_HIGH)
					{
						inPositionBit = MECH_STATUS_GRATING_RED_IN_POSITION_HIGH;
						faultBit = FAULT_STATUS_GRATING_POSITION_RED_HIGH;
					}
					else if(resolution == FrodoSpecConfig.RESOLUTION_LOW)
					{
						inPositionBit = MECH_STATUS_GRATING_RED_IN_POSITION_LOW;
						faultBit = FAULT_STATUS_GRATING_POSITION_RED_LOW;
					}
				}
				else if(arm == FrodoSpecConfig.BLUE_ARM)
				{
					transitBit = MECH_STATUS_GRATING_BLUE_IN_TRANSIT;
					if(resolution == FrodoSpecConfig.RESOLUTION_HIGH)
					{
						inPositionBit = MECH_STATUS_GRATING_BLUE_IN_POSITION_HIGH;
						faultBit = FAULT_STATUS_GRATING_POSITION_BLUE_HIGH;
					}
					else if(resolution == FrodoSpecConfig.RESOLUTION_LOW)
					{
						inPositionBit = MECH_STATUS_GRATING_BLUE_IN_POSITION_LOW;
						faultBit = FAULT_STATUS_GRATING_POSITION_BLUE_LOW;
					}
				}
				// set demand
				logger.log(Logger.VERBOSITY_VERBOSE,
					   this.getClass().getName()+
					   ":setGrating:Moving Arm "+FrodoSpecConstants.ARM_STRING_LIST[arm]+
					   " to resolution "+FrodoSpecConstants.RESOLUTION_STRING_LIST[resolution]+
					   ":Setting "+demandPLCAddress+" to "+demandValue+".");
				plc.setBoolean(handle,demandPLCAddress,demandValue);
				// monitor for completion/error
				done = false;	
				while(done == false)
				{
					// sleep for a bit
					try
					{
						Thread.sleep(gratingMoveSleepTime);
					}
					catch(InterruptedException e)
					{
						logger.log(Logger.VERBOSITY_VERBOSE,
							   this.getClass().getName()+
							   ":setGrating:Sleep interrupted.");
					}
					// get mechanism status
					mechStatus = getMechanismStatus();
					// are we in the correct position?
					if((mechStatus & transitBit) == transitBit)
					{
						logger.log(Logger.VERBOSITY_VERBOSE,
							   this.getClass().getName()+
							   ":setGrating:Grating is in transit.");
					}
					if((mechStatus & inPositionBit) == inPositionBit)
					{
						// exit loop
						done = true;
						logger.log(Logger.VERBOSITY_VERBOSE,
							   this.getClass().getName()+
							   ":setGrating:Grating is in position.");
					}
					// has PLC been turned to local?
					if((mechStatus & MECH_STATUS_PANEL_IN_LOCAL) == 
					   MECH_STATUS_PANEL_IN_LOCAL)
					{
						logger.log(Logger.VERBOSITY_VERBOSE,
							   this.getClass().getName()+
							   ":setGrating:Plc is in local.");
						throw new Exception(this.getClass().getName()+
				      		    ":setGrating:Plc is in local:"+printBits(mechStatus));
					}
					// get fault status
					faultStatus = getFaultStatus();
					if((faultStatus & faultBit) == faultBit)
					{
						logger.log(Logger.VERBOSITY_VERBOSE,
							   this.getClass().getName()+
							   ":setGrating:Grating Position Fault bit set:"+
							   printBits(faultStatus));
						throw new Exception(this.getClass().getName()+
								    ":setGrating:Grating Position Fault bit set:"+
								    printBits(faultStatus));
					}
					// check abort flag
					if(abortMovement)
					{
						logger.log(Logger.VERBOSITY_VERBOSE,
							   this.getClass().getName()+
							   ":setGrating:Abort movement flag set:Aborting.");
						throw new Exception(this.getClass().getName()+
								    ":setGrating:Abort movement flag set:Aborting.");
					}
				}// end while on done
			}
			else
			{
				logger.log(Logger.VERBOSITY_VERBOSE,
					   this.getClass().getName()+
					   ":setGrating:Movement not enabled:Grating not moved.");
			}
		}
		else
		{
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":setGrating:PLC not enabled:Grating not moved.");
		}
		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":setGrating:Finished.");
	}

	/**
	 * Abort method. This currently sets the abortMovement flag to true.
	 * This should abort (throw an exception) in any running setGrating methods.
	 * @see #abortMovement
	 */
	public void abort()
	{
		abortMovement = true;
	}

	/**
	 * Get the current position of the Grating for the specified arm.
	 * @param arm Which arm grating to use, either RED_ARM or BLUE_ARM.
	 * @return A integer describing the grating position of the specified arm; one of:
	 *         RESOLUTION_HIGH, RESOLUTION_LOW or 0 if unknown or in transit.
	 * @exception EIPNativeException Thrown if PLC comms fail.
	 * @exception IllegalArgumentException Thrown if arm is not legal a value.
	 * @see #MECH_STATUS_GRATING_RED_IN_TRANSIT
	 * @see #MECH_STATUS_GRATING_BLUE_IN_TRANSIT
	 * @see #MECH_STATUS_GRATING_RED_IN_POSITION_HIGH
	 * @see #MECH_STATUS_GRATING_RED_IN_POSITION_LOW
	 * @see #MECH_STATUS_GRATING_BLUE_IN_POSITION_HIGH
	 * @see #MECH_STATUS_GRATING_BLUE_IN_POSITION_LOW
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 * @see ngat.phase2.FrodoSpecConfig#RESOLUTION_HIGH
	 * @see ngat.phase2.FrodoSpecConfig#RESOLUTION_LOW
	 */
	public int getGratingResolution(int arm) throws EIPNativeException, IllegalArgumentException
	{
		int mechStatus,retval;

		// unknown / in transit
		retval = 0;
		// get mechanism status
		mechStatus = getMechanismStatus();
		if(arm == FrodoSpecConfig.RED_ARM)
		{
			if((mechStatus & MECH_STATUS_GRATING_RED_IN_TRANSIT) == MECH_STATUS_GRATING_RED_IN_TRANSIT)
				retval = 0;// in transit
			else if((mechStatus & MECH_STATUS_GRATING_RED_IN_POSITION_HIGH) == 
				MECH_STATUS_GRATING_RED_IN_POSITION_HIGH)
				retval = FrodoSpecConfig.RESOLUTION_HIGH;
			else if((mechStatus & MECH_STATUS_GRATING_RED_IN_POSITION_LOW) == 
				MECH_STATUS_GRATING_RED_IN_POSITION_LOW)
				retval = FrodoSpecConfig.RESOLUTION_LOW;
		}
		else if(arm == FrodoSpecConfig.BLUE_ARM)
		{
			if((mechStatus & MECH_STATUS_GRATING_BLUE_IN_TRANSIT) == MECH_STATUS_GRATING_BLUE_IN_TRANSIT)
				retval = 0;// in transit
			else if((mechStatus & MECH_STATUS_GRATING_BLUE_IN_POSITION_HIGH) == 
				MECH_STATUS_GRATING_BLUE_IN_POSITION_HIGH)
				retval = FrodoSpecConfig.RESOLUTION_HIGH;
			else if((mechStatus & MECH_STATUS_GRATING_BLUE_IN_POSITION_LOW) == 
				MECH_STATUS_GRATING_BLUE_IN_POSITION_LOW)
				retval = FrodoSpecConfig.RESOLUTION_LOW;
		}
		else
		{
			throw new IllegalArgumentException(this.getClass().getName()+
							   ":getGratingPositionString:Illegal arm:"+arm);
		}
		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":getGratingPositionString("+FrodoSpecConstants.ARM_STRING_LIST[arm]+
			   "):Finished with resolution:"+retval);
		return retval;
	}

	/**
	 * Get the current position of the Grating for the specified arm.
	 * @param arm Which arm grating to use, either RED_ARM or BLUE_ARM.
	 * @return A string describing the grating position of the specified arm; one of:
	 *         "high", "low", "in-transit", "UNKNOWN".
	 * @exception EIPNativeException Thrown if PLC comms fail.
	 * @exception IllegalArgumentException Thrown if arm is not legal a value.
	 * @see #MECH_STATUS_GRATING_RED_IN_TRANSIT
	 * @see #MECH_STATUS_GRATING_BLUE_IN_TRANSIT
	 * @see #MECH_STATUS_GRATING_RED_IN_POSITION_HIGH
	 * @see #MECH_STATUS_GRATING_RED_IN_POSITION_LOW
	 * @see #MECH_STATUS_GRATING_BLUE_IN_POSITION_HIGH
	 * @see #MECH_STATUS_GRATING_BLUE_IN_POSITION_LOW
	 * @see FrodoSpecConstants#ARM_STRING_LIST
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	public String getGratingPositionString(int arm) throws EIPNativeException, IllegalArgumentException
	{
		int mechStatus;
		String statusString = null;

		statusString = new String("UNKNOWN");
		// get mechanism status
		mechStatus = getMechanismStatus();
		if(arm == FrodoSpecConfig.RED_ARM)
		{
			if((mechStatus & MECH_STATUS_GRATING_RED_IN_TRANSIT) == MECH_STATUS_GRATING_RED_IN_TRANSIT)
				statusString = new String("in-transit");
			else if((mechStatus & MECH_STATUS_GRATING_RED_IN_POSITION_HIGH) == 
				MECH_STATUS_GRATING_RED_IN_POSITION_HIGH)
				statusString = new String("high");
			else if((mechStatus & MECH_STATUS_GRATING_RED_IN_POSITION_LOW) == 
				MECH_STATUS_GRATING_RED_IN_POSITION_LOW)
				statusString = new String("low");
		}
		else if(arm == FrodoSpecConfig.BLUE_ARM)
		{
			if((mechStatus & MECH_STATUS_GRATING_BLUE_IN_TRANSIT) == MECH_STATUS_GRATING_BLUE_IN_TRANSIT)
				statusString = new String("in-transit");
			else if((mechStatus & MECH_STATUS_GRATING_BLUE_IN_POSITION_HIGH) == 
				MECH_STATUS_GRATING_BLUE_IN_POSITION_HIGH)
				statusString = new String("high");
			else if((mechStatus & MECH_STATUS_GRATING_BLUE_IN_POSITION_LOW) == 
				MECH_STATUS_GRATING_BLUE_IN_POSITION_LOW)
				statusString = new String("low");
		}
		else
		{
			throw new IllegalArgumentException(this.getClass().getName()+
							   ":getGratingPositionString:Illegal arm:"+arm);
		}
		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":getGratingPositionString("+FrodoSpecConstants.ARM_STRING_LIST[arm]+
			   "):Finished with status:"+statusString);
		return statusString;
	}

	/**
	 * Get the humidity of the FrodoSpec enclosure from the PLC.
	 * @return A float. The relative humidity in percent (0..100).
	 * @exception EIPNativeException Thrown if PLC comms fail.
	 * @see #enable
	 * @see #logger
	 * @see #plc
	 * @see #handle
	 * @see #open
	 * @see #connectionIdleThread
	 * @see #humidityPLCAddress
	 */
	public float getHumidity() throws EIPNativeException
	{
		float humidity;

		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":getHumidity:Started.");
		if(enable)
		{
			// ensure connection is opened correctly
			synchronized(handle)
			{
				if(handle.isOpen() == false)
					open();
			}
			connectionIdleThread.setBusy();	
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":getHumidity:Retrieving from: "+humidityPLCAddress);
			humidity = plc.getFloat(handle,humidityPLCAddress);
		}
		else
		{
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":getHumidity:PLC not enabled:Humidity not retrieved.");
			humidity = 0.0f;
		}
		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":getHumidity:Finished with humidity:"+humidity);
		return humidity;
	}

	/**
	 * Get the temperature of the FrodoSpec enclosure from the PLC.
	 * @param index Which temperature probe to read. An integer from 0..TEMPERATURE_PROBE_COUNT-1.
	 * @return A float. The temperature in degrees centigrade.
	 * @exception EIPNativeException Thrown if PLC comms fail.
	 * @exception IllegalArgumentException Thrown if the index is out of range.
	 * @see #enable
	 * @see #logger
	 * @see #plc
	 * @see #handle
	 * @see #open
	 * @see #connectionIdleThread
	 * @see #temperaturePLCAddress
	 * @see #TEMPERATURE_PROBE_COUNT
	 */
	public float getTemperature(int index) throws EIPNativeException, IllegalArgumentException
	{
		float temperature;

		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":getTemperature("+index+"):Started.");
		if((index < 0)||(index >= TEMPERATURE_PROBE_COUNT))
		{
			throw new IllegalArgumentException(this.getClass().getName()+":getTemperature:index "+
							   index+" our of range 0.."+TEMPERATURE_PROBE_COUNT+".");
		}
		if(enable)
		{
			// ensure connection is opened correctly
			synchronized(handle)
			{
				if(handle.isOpen() == false)
					open();
			}
			connectionIdleThread.setBusy();	
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":getTemperature:Retrieving from: "+temperaturePLCAddress[index]);
			temperature = plc.getFloat(handle,temperaturePLCAddress[index]);
		}
		else
		{
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":getTemperature:PLC not enabled:Temperature not retrieved.");
			temperature = 0.0f;
		}
		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":getTemperature("+index+"):Finished with temperature:"+temperature);
		return temperature;
	}

	/**
	 * Get the instrument temperature of the FrodoSpec enclosure from the PLC.
	 * @return A float. The instrument temperature in degrees C.
	 * @exception EIPNativeException Thrown if PLC comms fail.
	 * @see #enable
	 * @see #logger
	 * @see #plc
	 * @see #handle
	 * @see #open
	 * @see #connectionIdleThread
	 * @see #instrumentTemperaturePLCAddress
	 */
	public float getInstrumentTemperature() throws EIPNativeException
	{
		float temperature;

		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":getInstrumentTemperature:Started.");
		if(enable)
		{
			// ensure connection is opened correctly
			synchronized(handle)
			{
				if(handle.isOpen() == false)
					open();
			}
			connectionIdleThread.setBusy();	
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":getInstrumentTemperature:Retrieving from: "+instrumentTemperaturePLCAddress);
			temperature = plc.getFloat(handle,instrumentTemperaturePLCAddress);
		}
		else
		{
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":getInstrumentTemperature:PLC not enabled:Instrument Temperature not retrieved.");
			temperature = 0.0f;
		}
		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":getInstrumentTemperature:Finished with instrument temperature:"+temperature);
		return temperature;
	}

	/**
	 * Get the panel temperature of the FrodoSpec enclosure from the PLC.
	 * @return A float. The panel temperature in degrees C.
	 * @exception EIPNativeException Thrown if PLC comms fail.
	 * @see #enable
	 * @see #logger
	 * @see #plc
	 * @see #handle
	 * @see #open
	 * @see #connectionIdleThread
	 * @see #panelTemperaturePLCAddress
	 */
	public float getPanelTemperature() throws EIPNativeException
	{
		float temperature;

		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":getPanelTemperature:Started.");
		if(enable)
		{
			// ensure connection is opened correctly
			synchronized(handle)
			{
				if(handle.isOpen() == false)
					open();
			}
			connectionIdleThread.setBusy();	
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":getPanelTemperature:Retrieving from: "+panelTemperaturePLCAddress);
			temperature = plc.getFloat(handle,panelTemperaturePLCAddress);
		}
		else
		{
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":getPanelTemperature:PLC not enabled:Panel Temperature not retrieved.");
			temperature = 0.0f;
		}
		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":getPanelTemperature:Finished with panel temperature:"+temperature);
		return temperature;
	}

	/**
	 * Get the air flow of the FrodoSpec pneumatics.
	 * @return A float. The pneumatic air flow in litres/minute.
	 * @exception EIPNativeException Thrown if PLC comms fail.
	 * @see #enable
	 * @see #logger
	 * @see #plc
	 * @see #handle
	 * @see #open
	 * @see #connectionIdleThread
	 * @see #airFlowPLCAddress
	 */
	public float getAirFlow() throws EIPNativeException
	{
		float airFlow;

		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":getAirFlow:Started.");
		if(enable)
		{
			// ensure connection is opened correctly
			synchronized(handle)
			{
				if(handle.isOpen() == false)
					open();
			}
			connectionIdleThread.setBusy();	
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":getAirFlow:Retrieving from: "+airFlowPLCAddress);
			airFlow = plc.getFloat(handle,airFlowPLCAddress);
		}
		else
		{
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":getAirFlow:PLC not enabled:Air Flow not retrieved.");
			airFlow = 0.0f;
		}
		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":getAirFlow:Finished with air flow:"+airFlow);
		return airFlow;
	}

	/**
	 * Get the air pressure of the FrodoSpec pneumatics.
	 * @return A float. The pneumatic air pressure in bar.
	 * @exception EIPNativeException Thrown if PLC comms fail.
	 * @see #enable
	 * @see #logger
	 * @see #plc
	 * @see #handle
	 * @see #open
	 * @see #connectionIdleThread
	 * @see #airPressurePLCAddress
	 */
	public float getAirPressure() throws EIPNativeException
	{
		float airPressure;

		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":getAirPressure:Started.");
		if(enable)
		{
			// ensure connection is opened correctly
			synchronized(handle)
			{
				if(handle.isOpen() == false)
					open();
			}
			connectionIdleThread.setBusy();	
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":getAirPressure:Retrieving from: "+airPressurePLCAddress);
			airPressure = plc.getFloat(handle,airPressurePLCAddress);
		}
		else
		{
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":getAirPressure:PLC not enabled:Air Pressure not retrieved.");
			airPressure = 0.0f;
		}
		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":getAirPressure:Finished with air pressure:"+airPressure);
		return airPressure;
	}

	/**
	 * Get the length of time the FrodoSpec cooling has been on.
	 * @return A float. The length of time the cooling has been on, in seconds.
	 * @exception EIPNativeException Thrown if PLC comms fail.
	 * @see #enable
	 * @see #logger
	 * @see #plc
	 * @see #handle
	 * @see #open
	 * @see #connectionIdleThread
	 * @see #coolingTimePLCAddress
	 */
	public float getCoolingTimeOn() throws EIPNativeException
	{
		float time;

		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":getCoolingTimeOn:Started.");
		if(enable)
		{
			// ensure connection is opened correctly
			synchronized(handle)
			{
				if(handle.isOpen() == false)
					open();
			}
			connectionIdleThread.setBusy();	
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":getCoolingTimeOn:Retrieving from: "+coolingTimePLCAddress);
			time = plc.getFloat(handle,coolingTimePLCAddress);
		}
		else
		{
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":getCoolingTimeOn:PLC not enabled:Cooling time not retrieved.");
			time = 0.0f;
		}
		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":getCoolingTimeOn:Finished with cooling time:"+time);
		return time;
	}

	/**
	 * Get the linear encoder position of the focus stage linear encoder.
	 * @param arm Which arm to get the linear encoder position for. 
	 * @return A float. The linear encoder position for the specified arm, in mm.
	 * @exception IllegalArgumentException Thrown if arm is not a legal value.
	 * @exception EIPNativeException Thrown if PLC comms fail.
	 * @see #enable
	 * @see #logger
	 * @see #plc
	 * @see #handle
	 * @see #open
	 * @see #connectionIdleThread
	 * @see #linearEncoderPositionPLCAddress
	 * @see FrodoSpecConstants#ARM_STRING_LIST
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	public float getLinearEncoderPosition(int arm) throws EIPNativeException, IllegalArgumentException
	{
		float position;

		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":getLinearEncoderPosition:Started.");
		// check arm is legal
		if((arm != FrodoSpecConfig.RED_ARM)&&(arm != FrodoSpecConfig.BLUE_ARM))
		{
			throw new IllegalArgumentException(this.getClass().getName()+
							   ":getLinearEncoderPosition:Illegal arm:"+arm);
		}
		if(enable)
		{
			// ensure connection is opened correctly
			synchronized(handle)
			{
				if(handle.isOpen() == false)
					open();
			}
			connectionIdleThread.setBusy();	
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":getLinearEncoderPosition(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+
				   "):Retrieving from: "+linearEncoderPositionPLCAddress[arm]);
			position = plc.getFloat(handle,linearEncoderPositionPLCAddress[arm]);
		}
		else
		{
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":getLinearEncoderPosition:PLC not enabled:Position not retrieved.");
			position = 0.0f;
		}
		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":getLinearEncoderPosition(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+
			   "):Finished with linear encoder position:"+position);
		return position;
	}

	/**
	 * Method to destroy the handle used for comms to the PLC. This should only be called
	 * at the end of PLC comms (or before init is called again).
	 * @see #enable
	 * @see #plc
	 * @see #handle
	 * @see ngat.eip.EIPPLC#destroyHandle
	 */
	public void destroyHandle() throws EIPNativeException
	{
		if(enable)
		{
			synchronized(handle)
			{
				if(handle.isOpen() == true)
				{
					plc.close(handle);
				}
				plc.destroyHandle(handle);
			}
		}
		else
		{
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":destroyHandle:enable was false:Handle not destroyed.");
		}
	}

	/**
	 * Set the log level. This only works if the <b>init</b> method has been previously called
	 * @param level The log level.
	 * @see #init
	 * @see #plc
	 */
	public void setLogLevel(int level)
	{
		if(plc != null)
			plc.setLogFilterLevel(level);
	}

	/**
	 * Get whether the PLC is enabled.
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

	/**
	 * Convert the first 16 bits of the integer to a bit string of the form 'nnnnnnnn nnnnnnnn', where
	 * each 'n' is one bit (either a 0 or 1).
	 * @param n The integer to convert.
	 * @return A string containing the bits.
	 */
	public static String printBits(int n)
	{
		StringBuffer sb = new StringBuffer();

		for(int i = 0; i < 16; i++)
		{
			if((n & (1<<i)) > 0)
				sb.insert(0,'1');
			else
				sb.insert(0,'0');
			if((i % 8) == 7)
				sb.insert(0,' ');
		}
		return sb.toString();
	}

	// protected methods
	/**
	 * Method to hold open the connection handle to the PLC.
	 * Normally the handle is opened/closed internally to EIPPLC (plc), for each call
	 * to get/set Integer/Float/Boolean. However, this can cause the EIP_Session_Handle_Open call to fail
	 * when a lot of open/closes are done close together (I think due to CLOSE_WAIT on the underlying
	 * TCP/IP socket). So EIPPLC (plc) supports an explicit open/close call so we can hold a session
	 * open whilst performing an operation that requires multiple PLC manipultations i.e. moving the grating
	 * or querying a whole load of status.
	 * @see #enable
	 * @see #plc
	 * @see #handle
	 * @see #connectionIdleThread
	 * @see ngat.eip.EIPPLC#open
	 */
	protected void open() throws EIPNativeException
	{
		if(enable)
		{
			synchronized(handle)
			{
				if(handle.isOpen() == false)
				{
					logger.log(Logger.VERBOSITY_VERBOSE,
						   this.getClass().getName()+":open:Opening handle.");
					plc.open(handle);
					logger.log(Logger.VERBOSITY_VERBOSE,
						   this.getClass().getName()+":open:Starting connection idle thread.");
					connectionIdleThread = new ConnectionIdleThread();
					connectionIdleThread.start();
				}
				else
					connectionIdleThread.setBusy();
			}
		}
		else
		{
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":open:enable was false:PLC not opened.");
		}
	}

	/**
	 * Method to close the open connection handle to the PLC.
	 * Normally the handle is opened/closed internally to EIPPLC (plc), for each call
	 * to get/set Integer/Float/Boolean. However, this can cause the EIP_Session_Handle_Open call to fail
	 * when a lot of open/closes are done close together (I think due to CLOSE_WAIT on the underlying
	 * TCP/IP socket). So EIPPLC (plc) supports an explicit open/close call so we can hold a session
	 * open whilst performing an operation that requires multiple PLC manipultations i.e. moving the grating
	 * or querying a whole load of status.
	 * @see #enable
	 * @see #plc
	 * @see #handle
	 * @see ngat.eip.EIPPLC#close
	 */
	protected void close() throws EIPNativeException
	{
		if(enable)
		{
			synchronized(handle)
			{
				if(handle.isOpen() == true)
				{
					plc.close(handle);
				}
			}
		}
		else
		{
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":close:enable was false:PLC not closed.");
		}
	}

	/**
	 * Method to load configuration from the status object.
	 * @exception EIPFormatException Thrown if the plc type is illegal.
	 * @exception NumberFormatException Thrown if an integer/float is not a valid number.
	 * @see #logger
	 * @see #enable
	 * @see #hostname
	 * @see #backplane
	 * @see #slot
	 * @see #plcType
	 * @see #moveEnable
	 * @see #configurationEnable
	 * @see #faultResetPLCAddress
	 * @see #gratingDemandPLCAddress
	 * @see #shutterDemandPLCAddress
	 * @see #ignoreSDSUShutterDemandPLCAddress
	 * @see #coolingDemandPLCAddress
	 * @see #powerDemandSDSUPLCAddress
	 * @see #powerDemandFocusStagePLCAddress
	 * @see #powerDemandArcomESSPLCAddress
	 * @see #powerDemandLightPLCAddress
	 * @see #powerDemandMaintLightPLCAddress
	 * @see #timerGratingLowHighPLCAddress
	 * @see #timerGratingHighLowPLCAddress
	 * @see #timerShutterOpenPLCAddress
	 * @see #timerShutterClosePLCAddress
	 * @see #setPointCoolingOnPLCAddress
	 * @see #setPointCoolingOffPLCAddress
	 * @see #setPointHighInstTemperaturePLCAddress
	 * @see #humidityPLCAddress
	 * @see #temperaturePLCAddress
	 * @see #TEMPERATURE_PROBE_COUNT
	 * @see #instrumentTemperaturePLCAddress
	 * @see #panelTemperaturePLCAddress
	 * @see #airPressurePLCAddress
	 * @see #airFlowPLCAddress
	 * @see #coolingTimePLCAddress
	 * @see #mechStatusPLCAddress
	 * @see #faultStatusPLCAddress
	 * @see #timerMovementLowHighValue
	 * @see #timerMovementHighLowValue
	 * @see #timerMovementShutterOpenValue
	 * @see #timerMovementShutterCloseValue
	 * @see #coolingSetPointOnValue
	 * @see #coolingSetPointOffValue
	 * @see #coolingSetPointInstrumentHighValue
	 * @see #gratingMoveSleepTime
	 * @see #connectionIdleTime
	 * @see FrodoSpecConstants#ARM_STRING_LIST
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	protected void loadConfig(FrodoSpecStatus status) throws EIPFormatException, NumberFormatException
	{
		String plcTypeString = null;

		logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":loadConfig:Started.");
		// load config
		enable = status.getPropertyBoolean("frodospec.plc.enable");
		if(enable)
		{
			moveEnable = status.getPropertyBoolean("frodospec.plc.enable.move");
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":loadConfig:PLC enabled.");
			hostname = status.getProperty("frodospec.plc.hostname");
			backplane = status.getPropertyInteger("frodospec.plc.backplane");
		        slot = status.getPropertyInteger("frodospec.plc.slot");
			plcTypeString = status.getProperty("frodospec.plc.type");
			plcType = EIPPLC.plcTypeFromString(plcTypeString);
			// Addressing
			// demand / inputs
			faultResetPLCAddress = status.getProperty("frodospec.plc.address.demand.fault.reset");
			for(int arm = FrodoSpecConfig.RED_ARM; arm <= FrodoSpecConfig.BLUE_ARM; arm++)
			{
				gratingDemandPLCAddress[arm] = status.getProperty("frodospec.plc.address.demand."+
								   FrodoSpecConstants.ARM_STRING_LIST[arm]+".grating");
				shutterDemandPLCAddress[arm] = status.getProperty("frodospec.plc.address.demand."+
								   FrodoSpecConstants.ARM_STRING_LIST[arm]+".shutter");
				powerDemandSDSUPLCAddress[arm] = status.getProperty("frodospec.plc.address.demand."+
							   "power."+FrodoSpecConstants.ARM_STRING_LIST[arm]+".sdsu");
				powerDemandFocusStagePLCAddress[arm] = status.getProperty("frodospec.plc.address."+
					     "demand.power."+FrodoSpecConstants.ARM_STRING_LIST[arm]+".focus.stage");
			}
			ignoreSDSUShutterDemandPLCAddress = status.getProperty("frodospec.plc.address.demand."+
									       "ignore.sdsu.shutter");
			coolingDemandPLCAddress = status.getProperty("frodospec.plc.address.demand.cooling");
			powerDemandArcomESSPLCAddress = status.getProperty("frodospec.plc.address.demand.power."+
									   "arcom.ess");
			powerDemandLightPLCAddress = status.getProperty("frodospec.plc.address.demand.power."+
									"light.source");
			powerDemandMaintLightPLCAddress = status.getProperty("frodospec.plc.address.demand.power."+
									     "maint.light");
			// timer addresses
			for(int arm = FrodoSpecConfig.RED_ARM; arm <= FrodoSpecConfig.BLUE_ARM; arm++)
			{
				timerGratingLowHighPLCAddress[arm] = status.getProperty("frodospec.plc.address.timer."+
											"movement.grating."+
							    FrodoSpecConstants.ARM_STRING_LIST[arm]+".low.high");
				timerGratingHighLowPLCAddress[arm] = status.getProperty("frodospec.plc.address.timer."+
										   "movement.grating."+
							    FrodoSpecConstants.ARM_STRING_LIST[arm]+".high.low");
				timerShutterOpenPLCAddress[arm] = status.getProperty("frodospec.plc.address.timer."+
										     "movement.shutter.open."+
							    FrodoSpecConstants.ARM_STRING_LIST[arm]);
				timerShutterClosePLCAddress[arm] = status.getProperty("frodospec.plc.address.timer."+
										     "movement.shutter.close."+
							    FrodoSpecConstants.ARM_STRING_LIST[arm]);
			}
			// cooling setpoint addresses
			setPointCoolingOnPLCAddress = status.getProperty("frodospec.plc.address.cooling."+
									 "set.point.on");
			setPointCoolingOffPLCAddress = status.getProperty("frodospec.plc.address.cooling."+
									 "set.point.off");
			setPointHighInstTemperaturePLCAddress = status.getProperty("frodospec.plc.address.cooling."+
									       "set.point.inst.high");
			// status / output addresses
			humidityPLCAddress = status.getProperty("frodospec.plc.address.status.humidity");
			for(int i = 0; i < TEMPERATURE_PROBE_COUNT; i++)
			{
				temperaturePLCAddress[i] = status.getProperty("frodospec.plc.address.status."+
									      "temperature."+i);
			}
			instrumentTemperaturePLCAddress = status.getProperty("frodospec.plc.address.status."+
									     "temperature.instrument");
			panelTemperaturePLCAddress = status.getProperty("frodospec.plc.address.status."+
									     "temperature.panel");
			airPressurePLCAddress = status.getProperty("frodospec.plc.address.status.air.pressure");
			airFlowPLCAddress = status.getProperty("frodospec.plc.address.status.air.flow");
			coolingTimePLCAddress = status.getProperty("frodospec.plc.address.status.cooling.time");
			mechStatusPLCAddress = status.getProperty("frodospec.plc.address.status.mechanism");
			faultStatusPLCAddress = status.getProperty("frodospec.plc.address.status.fault");
			// PLC config values
			configurationEnable = status.getPropertyBoolean("frodospec.plc.configuration.enable");
			for(int arm = FrodoSpecConfig.RED_ARM; arm <= FrodoSpecConfig.BLUE_ARM; arm++)
			{
				timerMovementLowHighValue[arm] = status.getPropertyInteger("frodospec.plc.value."+
					    "timer.movement."+FrodoSpecConstants.ARM_STRING_LIST[arm]+".low.high");
				timerMovementHighLowValue[arm] = status.getPropertyInteger("frodospec.plc.value."+
					    "timer.movement."+FrodoSpecConstants.ARM_STRING_LIST[arm]+".high.low");
				timerMovementShutterOpenValue[arm] = status.getPropertyInteger("frodospec.plc.value."+
					    "timer.movement.shutter.open."+FrodoSpecConstants.ARM_STRING_LIST[arm]);
				timerMovementShutterCloseValue[arm] = status.getPropertyInteger("frodospec.plc.value."+
					   "timer.movement.shutter.close."+FrodoSpecConstants.ARM_STRING_LIST[arm]);
			}
			coolingSetPointOnValue = status.getPropertyInteger("frodospec.plc.value.cooling."+
									   "set.point.on");
			coolingSetPointOffValue = status.getPropertyInteger("frodospec.plc.value.cooling."+
									    "set.point.off");
			coolingSetPointInstrumentHighValue = status.getPropertyInteger("frodospec.plc.value.cooling."+
									     "set.point.inst.high");
			// internal software config
			gratingMoveSleepTime = status.getPropertyInteger("frodospec.plc.grating.move.sleep.time");
			connectionIdleTime = status.getPropertyInteger("frodospec.plc.connection.idle.time");
		}
		else
		{
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":loadConfig:enable was false:PLC not initialised.");
		}
	}

	/**
	 * Inner class.
	 * Thread started when the handle is opened. Used to monitor when the opened handle has not been used,
	 * it then closes the open handle.
	 */
	public class ConnectionIdleThread extends Thread
	{
		/**
		 * Boolean keeping track of wether the connection is idle or not.
		 */
		protected boolean isIdle = false;

		/**
		 * Thread run method.
		 * <ul>
		 * <li>A loop is entered. 
		 * <li>isIdle is set to true. 
		 * <li>The thread then sleeps for connectionIdleTime (from the parent). 
		 * <li>The loop is exited if the isIdle is still true at the
		 *     end of the sleep (has not been reset by another thread calling setBusy).
		 * <li>When the loop is exited the connection is closed.
		 * </ul>
		 * @see #connectionIdleTime
		 * @see #isIdle
		 * @see #close
		 */
		public void run()
		{
			boolean done = false;

			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+":run:"+
				   hostname+":Started.");
			while(done == false)
			{
				logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
					   ":run:"+hostname+":Sleeping.");
				isIdle = true;
				try
				{
					Thread.sleep(connectionIdleTime);
				}
				catch(Exception e)
				{
				}
				logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
					   ":run:"+hostname+":Checking whether connection is idle:"+isIdle+".");
				done = (isIdle == true);
			}// while not done
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":run:"+hostname+":Closing connection.");
			try
			{
				close();
			}
			catch(Exception e)
			{
				logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
					   ":run:"+hostname+":Close connection failed.",e);
			}
			logger.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":run:"+hostname+":Finished.");
		}

		/**
		 * Method called to set isIdle to false, to stop the thread terminating.
		 * @see #isIdle
		 */
		public void setBusy()
		{
			isIdle = false;	
		}
	}// end inner class
}
//
// $Log: not supported by cvs2svn $
// Revision 1.3  2009/04/30 09:54:13  cjm
// Added abortMovement and abort method to abort setGrating calls.
//
// Revision 1.2  2009/02/05 11:38:59  cjm
// Swapped Bitwise for Absolute logging levels.
//
// Revision 1.1  2008/11/20 11:33:35  cjm
// Initial revision
//
//

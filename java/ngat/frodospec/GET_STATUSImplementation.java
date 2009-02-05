// GET_STATUSImplementation.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/GET_STATUSImplementation.java,v 1.2 2009-02-05 11:38:59 cjm Exp $
package ngat.frodospec;

import java.lang.*;
import java.util.Hashtable;

import ngat.eip.EIPNativeException;
import ngat.frodospec.ccd.*;
import ngat.frodospec.newmark.NewmarkNativeException;
import ngat.message.base.*;
import ngat.message.ISS_INST.ISS_TO_INST;
import ngat.message.ISS_INST.GET_STATUS;
import ngat.message.ISS_INST.GET_STATUS_DONE;
import ngat.phase2.FrodoSpecConfig;
import ngat.util.ExecuteCommand;
import ngat.util.logging.*;

/**
 * This class provides the implementation for the GET_STATUS command sent to a server using the
 * Java Message System.
 * @author Chris Mottram
 * @version $Revision: 1.2 $
 */
public class GET_STATUSImplementation extends INTERRUPTImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: GET_STATUSImplementation.java,v 1.2 2009-02-05 11:38:59 cjm Exp $");
	/**
	 * Internal constant used when converting temperatures in centigrade (from the CCD controller) to Kelvin 
	 * returned in GET_STATUS.
	 * @see #getIntermediateStatus
	 */
	private final static double CENTIGRADE_TO_KELVIN = 273.15;
	/**
	 * GET_STATUS hastable keyword for the PLC instrument status.
	 * Like KEYWORD_INSTRUMENT_STATUS/KEYWORD_DETECTOR_TEMPERATURE_INSTRUMENT_STATUS, but for FrodoSpec
	 * specific Plc.
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#KEYWORD_INSTRUMENT_STATUS
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#KEYWORD_DETECTOR_TEMPERATURE_INSTRUMENT_STATUS
	 */
	protected final static String KEYWORD_PLC_INSTRUMENT_STATUS = "Instrument.Status.Plc";
	/**
	 * GET_STATUS hastable keyword for the Focus Stage instrument status.
	 * Line KEYWORD_INSTRUMENT_STATUS/KEYWORD_DETECTOR_TEMPERATURE_INSTRUMENT_STATUS, but for FrodoSpec
	 * specific Focus Stage.
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#KEYWORD_INSTRUMENT_STATUS
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#KEYWORD_DETECTOR_TEMPERATURE_INSTRUMENT_STATUS
	 */
	protected final static String KEYWORD_FOCUS_STAGE_INSTRUMENT_STATUS = "Instrument.Status.Focus.Stage";
	/**
	 * GET_STATUS hastable keyword for the SDSU communications instrument status.
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#KEYWORD_INSTRUMENT_STATUS
	 */
	protected final static String KEYWORD_SDSU_COMMS_INSTRUMENT_STATUS = "Instrument.Status.SDSU.Comms";
	/**
	 * This hashtable is created in processCommand, and filled with status data,
	 * and is returned in the GET_STATUS_DONE object.
	 */
	private Hashtable hashTable = null;
	/**
	 * List of CCDLibrary instances to query. The first index is left as null, so the 
	 * array can be indexed by arm numbers RED_ARM and BLUE_ARM.
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 * @see #redCCD
	 * @see #blueCCD
	 */
	private CCDLibrary ccdList[] = {null,null,null};
	/**
	 * Standard status string passed back in the hashTable, describing the instrument status health,
	 * using the standard keyword KEYWORD_INSTRUMENT_STATUS. Initialised to VALUE_STATUS_UNKNOWN.
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#KEYWORD_INSTRUMENT_STATUS
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#VALUE_STATUS_UNKNOWN
	 */
	private String instrumentStatus = GET_STATUS_DONE.VALUE_STATUS_UNKNOWN;
	/**
	 * Standard status string passed back in the hashTable, describing the detector temperature status health,
	 * using the standard keyword KEYWORD_DETECTOR_TEMPERATURE_INSTRUMENT_STATUS. 
	 * Initialised to VALUE_STATUS_UNKNOWN.The first index is not used, so the 
	 * array can be indexed by arm numbers RED_ARM and BLUE_ARM.
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#KEYWORD_DETECTOR_TEMPERATURE_INSTRUMENT_STATUS
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#VALUE_STATUS_UNKNOWN
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	private String armDetectorTemperatureInstrumentStatus[] = {GET_STATUS_DONE.VALUE_STATUS_UNKNOWN,
								   GET_STATUS_DONE.VALUE_STATUS_UNKNOWN,
								   GET_STATUS_DONE.VALUE_STATUS_UNKNOWN};
	/**
	 * Values for the SDSU Communications instrument status.
	 * FrodoSpec status string passed back in the hashTable, describing whether we can talk to the SDSU 
	 * controllers, using the keyword KEYWORD_SDSU_COMMS_INSTRUMENT_STATUS. 
	 * Initialised to VALUE_STATUS_UNKNOWN.The first index is not used, so the 
	 * array can be indexed by arm numbers RED_ARM and BLUE_ARM.
	 * @see #KEYWORD_SDSU_COMMS_INSTRUMENT_STATUS
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#VALUE_STATUS_UNKNOWN
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	private String sdsuCommsInstrumentStatus[] = {GET_STATUS_DONE.VALUE_STATUS_UNKNOWN,
						      GET_STATUS_DONE.VALUE_STATUS_UNKNOWN,
						      GET_STATUS_DONE.VALUE_STATUS_UNKNOWN};

	/**
	 * Constructor. 
	 */
	public GET_STATUSImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.GET_STATUS&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.GET_STATUS";
	}

	/**
	 * This method gets the GET_STATUS command's acknowledge time. 
	 * This takes the default acknowledge time to implement.
	 * @param command The command instance we are implementing.
	 * @return An instance of ACK with the timeToComplete set.
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see FrodoSpecTCPServerConnectionThread#getDefaultAcknowledgeTime
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		ACK acknowledge = null;

		acknowledge = new ACK(command.getId());
		acknowledge.setTimeToComplete(serverConnectionThread.getDefaultAcknowledgeTime());
		return acknowledge;
	}

	/**
	 * This method implements the GET_STATUS command. 
	 * The local hashTable is setup (returned in the done object) and a local copy of status setup.
	 * The current mode of the camera is returned by calling getCurrentMode.
	 * The following data is put into the hashTable:
	 * <ul>
	 * <li><b>currentCommand</b> The current command from the status object, or blank if no current command.
	 * <li><b>Instrument</b> The name of this instrument, retrieved from the property 
	 * 	<i>frodospec.get_status.instrument_name</i>.
	 *      <li>For each arm, the following is added:
	 *      <ul>
	 *      <li><b>&lt;arm&gt;.NRows, &lt;arm&gt;.NCols</b> Number of rows and columns setup on the CCD 
	 *          (from libccd, not the camera hardware).
	 *      <li><b>&lt;arm&gt;.NSBin, &lt;arm&gt;.NPBin</b> Binning factor for rows and columns setup on the CCD 
	 * 	    (from libccd, not the camera hardware).
	 *      <li><b>&lt;arm&gt;.DeInterlace Type</b> The de-interlace type, which tells us how many 
	 *          readouts we are using (from libccd, not the camera hardware).
	 *      <li><b>&lt;arm&gt;.Window Flags</b> The window flags, which tell us which windows are in effect
	 * 	    (from libccd, not the camera hardware).
	 *      <li><b>&lt;arm&gt;.Setup Status</b> Whether the camera has been setup sufficiently for 
	 *          exposures to be taken (from libccd, not the camera hardware).
	 *      <li><b>&lt;arm&gt;.Exposure Start Time, &lt;arm&gt;.Exposure Length</b> The exposure start time, 
	 *          and the length of the current (or last) exposure.
	 *      <li><b>&lt;arm&gt;.Exposure Count, &lt;arm&gt;.Exposure Number</b> How many exposures the 
	 *          current command has taken and how many it will do in total (from the status object).
	 * </ul>
	 * If the command requests a <b>INTERMEDIATE</b> level status, getIntermediateStatus is called.
	 * If the command requests a <b>FULL</b> level status, getFullStatus is called.
	 * An object of class GET_STATUS_DONE is returned, with the information retrieved.
	 * @see #status
	 * @see #hashTable
	 * @see #getCurrentMode
	 * @see #getIntermediateStatus
	 * @see #getFullStatus
	 * @see #ccdList
	 * @see FrodoSpecStatus#getCurrentCommand
	 * @see ngat.frodospec.ccd.CCDLibrary#getExposureStatus
	 * @see ngat.frodospec.ccd.CCDLibrary#getExposureLength
	 * @see ngat.frodospec.ccd.CCDLibrary#getExposureStartTime
	 * @see ngat.frodospec.ccd.CCDLibrary#getNCols
	 * @see ngat.frodospec.ccd.CCDLibrary#getNRows
	 * @see ngat.frodospec.ccd.CCDLibrary#getXBin
	 * @see ngat.frodospec.ccd.CCDLibrary#getYBin
	 * @see ngat.frodospec.ccd.CCDLibrary#getDeInterlaceType
	 * @see ngat.frodospec.ccd.CCDLibrary#setupGetWindowFlags
	 * @see ngat.frodospec.ccd.CCDLibrary#getSetupComplete
	 * @see FrodoSpecStatus#getExposureCount
	 * @see FrodoSpecStatus#getExposureNumber
	 * @see FrodoSpecStatus#getProperty
	 * @see FrodoSpecStatus#getPropertyInteger
	 * @see FrodoSpecStatus#getPropertyBoolean
	 * @see FrodoSpecConstants#ARM_STRING_LIST
	 * @see ngat.message.ISS_INST.GET_STATUS#getLevel
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		GET_STATUS getStatusCommand = (GET_STATUS)command;
		GET_STATUS_DONE getStatusDone = new GET_STATUS_DONE(command.getId());
		ISS_TO_INST currentCommand = null;
		String lampControllerStatus = null;
		int currentMode;

		frodospec.log(Logger.VERBOSITY_VERY_TERSE,
			      this.getClass().getName()+":processCommand:Setting index 0 to redCCD "+redCCD+".");
		ccdList[FrodoSpecConfig.RED_ARM] = redCCD;
		frodospec.log(Logger.VERBOSITY_VERY_TERSE,
			      this.getClass().getName()+":processCommand:Setting index 1 to blueCCD "+blueCCD+".");
		ccdList[FrodoSpecConfig.BLUE_ARM] = blueCCD;
	 // Create new hashtable to be returned
		hashTable = new Hashtable();
	// current mode
		currentMode = getCurrentMode();
		getStatusDone.setCurrentMode(currentMode);
	// What instrument is this?
		hashTable.put("Instrument",status.getProperty("frodospec.get_status.instrument_name"));
	// Initialise Standard status to UNKNOWN
		instrumentStatus = GET_STATUS_DONE.VALUE_STATUS_UNKNOWN;
		hashTable.put(GET_STATUS_DONE.KEYWORD_INSTRUMENT_STATUS,instrumentStatus);
		hashTable.put(GET_STATUS_DONE.KEYWORD_DETECTOR_TEMPERATURE_INSTRUMENT_STATUS,
			      GET_STATUS_DONE.VALUE_STATUS_UNKNOWN);
		for(int arm = FrodoSpecConfig.RED_ARM; arm <= FrodoSpecConfig.BLUE_ARM; arm++)
		{
			// initialise per arm instrument and detector status
			armDetectorTemperatureInstrumentStatus[arm] = GET_STATUS_DONE.VALUE_STATUS_UNKNOWN;
			hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+"."+
				      GET_STATUS_DONE.KEYWORD_DETECTOR_TEMPERATURE_INSTRUMENT_STATUS,
				      armDetectorTemperatureInstrumentStatus[arm]);
			hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+"."+
				      GET_STATUS_DONE.KEYWORD_INSTRUMENT_STATUS,
				      GET_STATUS_DONE.VALUE_STATUS_UNKNOWN);
			// current command
			currentCommand = status.getCurrentCommand(arm);
			if(currentCommand == null)
				hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+".currentCommand","");
			else
				hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+".currentCommand",
					      currentCommand.getClass().getName());
			// Currently, we query libccd setup stored settings, not hardware.
			frodospec.log(Logger.VERBOSITY_VERY_TERSE,
				      this.getClass().getName()+":processCommand:Getting dimension settings for "+
				      FrodoSpecConstants.ARM_STRING_LIST[arm]+".");
			hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+".Current Mode",
				      new Integer(getCurrentMode(arm)));
			hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+".NCols",
				      new Integer(ccdList[arm].getNCols()));
			hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+".NRows",
				      new Integer(ccdList[arm].getNRows()));
			hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+".NSBin",
				      new Integer(ccdList[arm].getXBin()));
			hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+".NPBin",
				      new Integer(ccdList[arm].getYBin()));
			hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+".DeInterlace Type",
				      new Integer(ccdList[arm].getDeInterlaceType()));
			hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+".Window Flags",
				      new Integer(ccdList[arm].setupGetWindowFlags()));
			hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+".Setup Status",
				      new Boolean(ccdList[arm].getSetupComplete()));
			hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+".Exposure Length",
				      new Integer(ccdList[arm].getExposureLength()));
			hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+".Exposure Start Time",
				      new Long(ccdList[arm].getExposureStartTime()));
			hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+".Exposure Count",
				      new Integer(status.getExposureCount(arm)));
			hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+".Exposure Number",
				      new Integer(status.getExposureNumber(arm)));
		}// end for
		// Lamp Controller status
		lampControllerStatus = frodospec.getLampController().getLampControllerStatus();
		frodospec.log(Logger.VERBOSITY_VERY_TERSE,
			 this.getClass().getName()+":processCommand:Lamp Controller Status:"+lampControllerStatus);
		hashTable.put("Lamp.Controller.Status",lampControllerStatus);
	// intermediate level information - basic plus controller calls.
		if(getStatusCommand.getLevel() >= GET_STATUS.LEVEL_INTERMEDIATE)
		{
			frodospec.log(Logger.VERBOSITY_VERY_TERSE,
				      this.getClass().getName()+":processCommand:Getting intermediate status.");
			getIntermediateStatus();
		}// end if intermediate level status
	// Get full status information.
		if(getStatusCommand.getLevel() >= GET_STATUS.LEVEL_FULL)
		{
			frodospec.log(Logger.VERBOSITY_VERY_TERSE,
				      this.getClass().getName()+":processCommand:Getting full status.");
			getFullStatus();
		}
	// set hashtable and return values.
		getStatusDone.setDisplayInfo(hashTable);
		getStatusDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_NO_ERROR);
		getStatusDone.setErrorString("");
		getStatusDone.setSuccessful(true);
	// return done object.
		return getStatusDone;
	}

	/**
	 * Internal method to get the current mode, the GET_STATUS command will return.
	 * @return The current mode, as defined in GET_STATUS_DONE.
	 * @see #ccdList
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_IDLE
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_CONFIGURING
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_WAITING_TO_START
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_EXPOSING
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_PRE_READOUT
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_READING_OUT
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_POST_READOUT
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_ERROR
	 */
	private int getCurrentMode()
	{
		int currentMode;
		int arm;

		currentMode = GET_STATUS_DONE.MODE_IDLE;
		arm = FrodoSpecConfig.RED_ARM;
		// Go through list of arms until we find one doing something
		// Of course, we really need 2 currentModes
		while((arm <= FrodoSpecConfig.BLUE_ARM)&&(currentMode == GET_STATUS_DONE.MODE_IDLE))
		{
			frodospec.log(Logger.VERBOSITY_VERY_TERSE,
				      this.getClass().getName()+":getCurrentMode:Trying arm "+arm+".");
			currentMode = getCurrentMode(arm);
			arm++;
		}// end while
		return currentMode;
	}

	/**
	 * Internal method to get the current mode, the GET_STATUS command will return.
	 * @param arm Which arm in ccdList to query. One of RED_ARM or BLUE_ARM, arm index 0 is illegal.
	 * @return The current mode, as defined in GET_STATUS_DONE.
	 * @exception IllegalArgumentException Throwm if arm is out of range.
	 * @see #ccdList
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_IDLE
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_CONFIGURING
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_WAITING_TO_START
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_EXPOSING
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_PRE_READOUT
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_READING_OUT
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_POST_READOUT
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_ERROR
	 */
	private int getCurrentMode(int arm) throws IllegalArgumentException
	{
		int currentMode;

		if((arm != FrodoSpecConfig.RED_ARM)&&(arm != FrodoSpecConfig.BLUE_ARM))
		{
			throw new IllegalArgumentException(this.getClass().getName()+
							   ":getCurrentMode: Illegal arm:"+arm);
		}
		currentMode = GET_STATUS_DONE.MODE_IDLE;
		frodospec.log(Logger.VERBOSITY_VERY_TERSE,
			      this.getClass().getName()+":getCurrentMode:Trying to get exposure status with arm "+arm+
			      ", library reference "+ccdList[arm]+".");
		switch(ccdList[arm].getExposureStatus())
		{
			case CCDLibrary.EXPOSURE_STATUS_NONE:
				if(ccdList[arm].getSetupInProgress())
					currentMode = GET_STATUS_DONE.MODE_CONFIGURING;
				break;
				/* diddly Not present for FrodoSpec
			case CCDLibrary.EXPOSURE_STATUS_CLEAR:
				currentMode =  GET_STATUS_DONE.MODE_CLEARING;
				break;
				*/
			case CCDLibrary.EXPOSURE_STATUS_WAIT_START:
				currentMode =  GET_STATUS_DONE.MODE_WAITING_TO_START;
				break;
			case CCDLibrary.EXPOSURE_STATUS_EXPOSE:
				currentMode = GET_STATUS_DONE.MODE_EXPOSING;
				break;
			case CCDLibrary.EXPOSURE_STATUS_PRE_READOUT:
				currentMode = GET_STATUS_DONE.MODE_PRE_READOUT;
				break;
			case CCDLibrary.EXPOSURE_STATUS_READOUT:
				currentMode = GET_STATUS_DONE.MODE_READING_OUT;
				break;
			case CCDLibrary.EXPOSURE_STATUS_POST_READOUT:
				currentMode = GET_STATUS_DONE.MODE_POST_READOUT;
				break;
			default:
				currentMode = GET_STATUS_DONE.MODE_ERROR;
				break;
		}// end switch
		return currentMode;
	}

	/**
	 * Routine to get status, when level INTERMEDIATE has been selected.
	 * Intermediate level status is usually useful data which can only be retrieved by querying the
	 * SDSU controller directly. 
	 * The following data is put into the hashTable:
	 * <ul>
	 * <li><b>&lt;arm&gt;.Elapsed Exposure Time</b> The Elapsed Exposure Time, this is read from the controller.
	 * </ul>
	 * If the <i>frodospec.get_status.ccd.temperature</i> boolean property is TRUE, 
	 * the following data is put into the hashTable:
	 * <ul>
	 * <li><b>&lt;arm&gt;.Temperature</b> The current CCD (dewar) temperature, this is read from the controller.
	 *       <i>setDetectorTemperatureInstrumentStatus</i> is then called to set the hashtable entry 
	 *       KEYWORD_DETECTOR_TEMPERATURE_INSTRUMENT_STATUS and armDetectorTemperatureInstrumentStatus.
	 * <li><b>&lt;arm&gt;.Heater ADU</b> The current Heater ADU count, this is read from the controller.
	 * <li><b>&lt;arm&gt;.Utility Board Temperature ADU</b> The Utility Board ADU count, 
	 * 	this is read from the utility board temperature sensor.
	 * </ul>
	 * If the <i>frodospec.get_status.supply_voltages</i> boolean property is TRUE, 
	 * the following data is put into the hashTable:
	 * <ul>
	 * <li><b>&lt;arm&gt;.High Voltage Supply ADU</b> The SDSU High Voltage supply ADU count, 
	 * 	this is read from the utility board.
	 * <li><b>&lt;arm&gt;.Low Voltage Supply ADU</b> The SDSU Low Voltage supply ADU count, 
	 * 	this is read from the utility board.
	 * <li><b>&lt;arm&gt;.Minus Low Voltage Supply ADU</b> The SDSU Negative Voltage supply ADU count, 
	 * 	this is read from the utility board.
	 * </ul>
	 * If the <i>frodospec.get_status.gratings</i> boolean property is TRUE, 
	 * the following data is put into the hashTable:
	 * <ul>
	 * <li><b>&lt;arm&gt;.Grating Position String</b> This is either "High" or "Low", and is derived
	 *       from reading the demand position boolean in the PLC, using <b>getGratingPositionString</b>.
	 * <li><b>&lt;arm&gt;.Grating Position</b> This is  a boolean, true for High and false for Low, and is derived
	 *       from reading the demand position boolean in the PLC, using the 
	 *       plc address derived from <i>frodospec.config.grating.&lt;arm&gt;.plc_address</i> config.
	 * </ul>
	 * Finally, <i>setInstrumentStatus</i> is called to set the hashTable's arm and overall instrument status,
	 * in the KEYWORD_INSTRUMENT_STATUS.
	 * @see #ccdList
	 * @see #status
	 * @see #hashTable
	 * @see #KEYWORD_SDSU_COMMS_INSTRUMENT_STATUS
	 * @see #sdsuCommsInstrumentStatus
	 * @see #setDetectorTemperatureInstrumentStatus
	 * @see #setInstrumentStatus
	 * @see #CENTIGRADE_TO_KELVIN
	 * @see ngat.frodospec.ccd.CCDLibrary#getElapsedExposureTime
	 * @see ngat.frodospec.ccd.CCDLibrary#getHighVoltageAnalogueADU
	 * @see ngat.frodospec.ccd.CCDLibrary#getLowVoltageAnalogueADU
	 * @see ngat.frodospec.ccd.CCDLibrary#getMinusLowVoltageAnalogueADU
	 * @see ngat.frodospec.ccd.CCDLibrary#temperatureGet
	 * @see ngat.frodospec.ccd.CCDLibrary#temperatureGetHeaterADU
	 * @see ngat.frodospec.ccd.CCDLibrary#temperatureGetUtilityBoardADU
	 * @see FrodoSpecStatus#getPropertyBoolean
	 * @see FrodoSpecStatus#getProperty
	 * @see #plc
	 * @see FrodoSpec#getPLC
	 * @see Plc
	 * @see Plc#getFaultStatus
	 * @see Plc#getMechanismStatus
	 * @see Plc#getGratingPositionString
	 * @see Plc#getTemperature
	 * @see Plc#getHumidity
	 * @see Plc#getInstrumentTemperature
	 * @see Plc#getPanelTemperature
	 * @see Plc#getAirFlow
	 * @see Plc#getAirPressure
	 * @see Plc#getCoolingTimeOn
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 * @see FrodoSpecConstants#ARM_STRING_LIST
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_WAITING_TO_START
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_EXPOSING
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_PRE_READOUT
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_READING_OUT
	 */
	private void getIntermediateStatus()
	{
		Plc plc = null;
		String focusStageInstrumentStatusString[] = {GET_STATUS_DONE.VALUE_STATUS_UNKNOWN,
				  GET_STATUS_DONE.VALUE_STATUS_UNKNOWN,GET_STATUS_DONE.VALUE_STATUS_UNKNOWN};
		int elapsedExposureTime,adu,ivalue,index,plcFaultStatus = 0,plcMechanismStatus = 0, currentMode;
		double dvalue;
		float fvalue;
		boolean bvalue,done;

		for(int arm = FrodoSpecConfig.RED_ARM; arm <= FrodoSpecConfig.BLUE_ARM; arm++)
		{
			// These setting are queried directly from the controller.
			// CCDLibrary routines that do DSP code may cause problems here as the instrument
			// may be in the process of reading out or similar.
			// Now that we have compiled mutex support in CCDLibrary 
			// around controller commands this should work.
			// elapsed exposure time - this seems to work when an exposure is in progress.
			frodospec.log(Logger.VERBOSITY_VERY_TERSE,
				      this.getClass().getName()+
				      ":processCommand:Getting elapsed exposure time for arm "+
				      FrodoSpecConstants.ARM_STRING_LIST[arm]+".");
			elapsedExposureTime = ccdList[arm].getElapsedExposureTime();
			// Always add the exposure time, if we are reading out it has been set to 0
			hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+".Elapsed Exposure Time",
				      new Integer(elapsedExposureTime));
			// This involves a read of the utility board, which will fail when exposing...
			// Therefore only put the temperature in the hashtable on success.
			// Return temperature in degrees kelvin.
			// Assuming CCD_DSP_UTIL_EXPOSURE_CHECK == 3 (ccd_dsp.c):
			currentMode = getCurrentMode(arm);
			if((currentMode != GET_STATUS_DONE.MODE_WAITING_TO_START) &&
			   (currentMode != GET_STATUS_DONE.MODE_EXPOSING) &&
			   (currentMode != GET_STATUS_DONE.MODE_PRE_READOUT) &&
			   (currentMode != GET_STATUS_DONE.MODE_READING_OUT) )
			{
				if(status.getPropertyBoolean("frodospec.get_status.ccd.temperature"))
				{
					// CCD temperature
					// This involves a read of the utility board, which will fail when exposing...
					// Therefore only put the temperature in the hashtable on success.
					// Return temperature in degrees kelvin.
					try
					{
						frodospec.log(Logger.VERBOSITY_VERY_TERSE,
							      this.getClass().getName()+
							      ":processCommand:Getting temperature for arm "+
							      FrodoSpecConstants.ARM_STRING_LIST[arm]+".");
						dvalue = ccdList[arm].temperatureGet();
						hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+".Temperature",
							      new Double(dvalue+CENTIGRADE_TO_KELVIN));
						sdsuCommsInstrumentStatus[arm] = GET_STATUS_DONE.VALUE_STATUS_OK;
						// set standard status value based on current temperature
						setDetectorTemperatureInstrumentStatus(arm,dvalue);
					}
					catch(CCDLibraryNativeException e)
					{
						frodospec.error(this.getClass().getName()+
								":getIntermediateStatus:Get Temperature failed.",e);
						// This exception can be thrown if the temperature is being read 
						// during readout. If the CCD camera has genuinely dropped off line
						// (i.e. power cycled) we seem to get TOUT messages so here is a 
						// primitive test for that.
						if(e.getMessage().indexOf("Reply was TOUT") > -1)
						{
							sdsuCommsInstrumentStatus[arm] = GET_STATUS_DONE.
								VALUE_STATUS_FAIL;
						}
						else
						{
							sdsuCommsInstrumentStatus[arm] = GET_STATUS_DONE.
								VALUE_STATUS_UNKNOWN;
						}
					}// catch
					// Dewar heater ADU counts - 
					// how much we are heating the dewar to control the temperature.
					// Utility Board ADU counts - 
					// how hot the temperature sensor is on the utility board.
					try
					{
						frodospec.log(Logger.VERBOSITY_VERY_TERSE,
							      this.getClass().getName()+
							      ":processCommand:Getting temperature ADUs for arm "+
							      FrodoSpecConstants.ARM_STRING_LIST[arm]+".");
						adu = ccdList[arm].temperatureGetHeaterADU();
						hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+".Heater ADU",
							      new Integer(adu));
						adu = ccdList[arm].temperatureGetUtilityBoardADU();
						hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+
							      ".Utility Board Temperature ADU",new Integer(adu));
					}
					catch(CCDLibraryNativeException e)
					{
						frodospec.error(this.getClass().getName()+
								":getIntermediateStatus:Get ADU(s) failed.",e);
					}// end catch
				}// end if get temperature status
				// SDSU supply voltages
				if(status.getPropertyBoolean("frodospec.get_status.ccd.supply_voltages"))
				{
					try
					{
						frodospec.log(Logger.VERBOSITY_VERY_TERSE,
							      this.getClass().getName()+
							      ":processCommand:Getting voltage ADUs for arm "+
							      FrodoSpecConstants.ARM_STRING_LIST[arm]+".");
						adu = ccdList[arm].getHighVoltageAnalogueADU();
						hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+
							      ".High Voltage Supply ADU",new Integer(adu));
						adu = ccdList[arm].getLowVoltageAnalogueADU();
						hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+
							      ".Low Voltage Supply ADU",new Integer(adu));
						adu = ccdList[arm].getMinusLowVoltageAnalogueADU();
						hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+
							      ".Minus Low Voltage Supply ADU",
							      new Integer(adu));
					}
					catch(CCDLibraryNativeException e)
					{
						frodospec.error(this.getClass().getName()+
								":getIntermediateStatus:Get supply voltage ADU failed.",e);
					}// end catch
				}// end if get supply voltage status
			}// end if mode is suitable for talking to the utility board
			else // the SDSU controller is doing an exposure - we can't get the temperature
			{
				sdsuCommsInstrumentStatus[arm] = GET_STATUS_DONE.VALUE_STATUS_UNKNOWN;
			}
		}// end for on CCD controllers
		// focus stages 
		if(status.getPropertyBoolean("frodospec.get_status.focus_stage.position"))
		{
			for(int arm = FrodoSpecConfig.RED_ARM; arm <= FrodoSpecConfig.BLUE_ARM; arm++)
			{
				FocusStage focusStage = null;
				double position;
				
				focusStage = frodospec.getFocusStage(arm);
				if(focusStage.getEnable())
				{
					try
					{
						position = focusStage.getPosition();
						hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+
							      ".Focus Stage Position",new Double(position));
						focusStageInstrumentStatusString[arm] =GET_STATUS_DONE.VALUE_STATUS_OK;
					}
					catch(Exception e)
					{
						frodospec.error(this.getClass().getName()+
						       ":getIntermediateStatus:Get focus stage position failed.",e);
						focusStageInstrumentStatusString[arm]=GET_STATUS_DONE.VALUE_STATUS_FAIL;
					}
				}// end if enable
				else
					focusStageInstrumentStatusString[arm] = GET_STATUS_DONE.VALUE_STATUS_UNKNOWN;
			}// end for on arm
		}// end if focus_stage
		// PLC
		plc = frodospec.getPLC();
		//  plc fault status
		frodospec.log(Logger.VERBOSITY_VERY_TERSE,
			      this.getClass().getName()+":getIntermediateStatus:Getting PLC fault status.");
		try
		{
			plcFaultStatus = plc.getFaultStatus();
		}
		catch(EIPNativeException e)
		{
			frodospec.error(this.getClass().getName()+
		":getIntermediateStatus:Get PLC fault status failed:Setting internally to 16383 (bits 0..13 set).",e);
			// set bits 0..13 - this should propogate into the instrument status
			plcFaultStatus = 16383;
		}// end catch
		hashTable.put("Plc.Fault.Status",new Integer(plcFaultStatus));
		hashTable.put("Plc.Fault.Status.String",new String(Plc.printBits(plcFaultStatus)));
		// mechanism status
		try
		{
			plcMechanismStatus = plc.getMechanismStatus();
		}
		catch(EIPNativeException e)
		{
			frodospec.error(this.getClass().getName()+
					":getIntermediateStatus:Get PLC mechanism status failed.",e);
			plcMechanismStatus = 0;
		}// end catch
		hashTable.put("Plc.Mechanism.Status",new Integer(plcMechanismStatus));
		hashTable.put("Plc.Mechanism.Status.String",new String(Plc.printBits(plcMechanismStatus)));
		// get grating positions from PLC?
		if(status.getPropertyBoolean("frodospec.get_status.plc.gratings"))
		{
			for(int arm = FrodoSpecConfig.RED_ARM; arm <= FrodoSpecConfig.BLUE_ARM; arm++)
			{
				try
				{
					String gratingPositionString = null;
					
					frodospec.log(Logger.VERBOSITY_VERY_TERSE,
						      this.getClass().getName()+
						":getIntermediateStatus:Getting grating position from PLC for arm "+
						FrodoSpecConstants.ARM_STRING_LIST[arm]+".");
					gratingPositionString = plc.getGratingPositionString(arm);
					hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+
						      ".Grating Position String",gratingPositionString);
					frodospec.log(Logger.VERBOSITY_VERY_TERSE,
						      this.getClass().getName()+
						":getIntermediateStatus:Grating position statusfrom PLC for arm "+
					      FrodoSpecConstants.ARM_STRING_LIST[arm]+" = "+gratingPositionString+".");
				}
				catch(EIPNativeException e)
				{
					frodospec.error(this.getClass().getName()+
						    ":getIntermediateStatus:Get grating position from PLC failed.",e);
				}// end catch
			} // end for on arm
		}// end if get grating positions
		// environment sensors
		if(status.getPropertyBoolean("frodospec.get_status.plc.environment"))
		{
			try
			{
				for(int i = 0; i< Plc.TEMPERATURE_PROBE_COUNT; i++)
				{
					frodospec.log(Logger.VERBOSITY_VERY_TERSE,
						      this.getClass().getName()+
						      ":processCommand:Getting enviromental temperature "+i+".");
					fvalue = plc.getTemperature(i);
					hashTable.put("Environment.Temperature."+i,new Float(fvalue));
					frodospec.log(Logger.VERBOSITY_VERY_TERSE,
						      this.getClass().getName()+
						      ":processCommand:Enviromental temperature "+i+
						      " has value "+fvalue+".");
				}// end for on temperature sensor
				// humidity
				fvalue = plc.getHumidity();
				hashTable.put("Environment.Humidity",new Float(fvalue));
				// instrument temperature
				fvalue = plc.getInstrumentTemperature();
				hashTable.put("Environment.Temperature.Instrument",new Float(fvalue));
				// panel temperature
				fvalue = plc.getPanelTemperature();
				hashTable.put("Environment.Temperature.Panel",new Float(fvalue));
			}
			catch(EIPNativeException e)
			{
				frodospec.error(this.getClass().getName()+
						":getIntermediateStatus:Get enviroment data failed.",e);
			}// end catch
		}// end if get enviroment
		if(status.getPropertyBoolean("frodospec.get_status.plc.mechanism"))
		{
			try
			{
				// air flow
				fvalue = plc.getAirFlow();
				hashTable.put("Air.Flow",new Float(fvalue));
				// air pressure
				fvalue = plc.getAirPressure();
				hashTable.put("Air.Pressure",new Float(fvalue));
				// cooling time
				fvalue = plc.getCoolingTimeOn();
				hashTable.put("Cooling.Time",new Float(fvalue));
				// linear encoders
				for(int arm = FrodoSpecConfig.RED_ARM; arm <= FrodoSpecConfig.BLUE_ARM; arm++)
				{
					fvalue = plc.getLinearEncoderPosition(arm);
					hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+
						      ".Focus.Stage.Linear.Encoder.Position",new Float(fvalue));
				}// end for on arm
			}
			catch(EIPNativeException e)
			{
				frodospec.error(this.getClass().getName()+
						":getIntermediateStatus:Get mechanism data failed.",e);
			}// end catch
		}// end if get enviromental temperature
	// Standard status
		setInstrumentStatus(plcFaultStatus,focusStageInstrumentStatusString);
	}

	/**
	 * Set the entry for detector temperature (for the specifed arm) in the hashtable based upon the 
	 * current temperature.
	 * Reads the folowing config:
	 * <ul>
	 * <li>frodospec.get_status.ccd.<arm>.temperature.warm.warn
	 * <li>frodospec.get_status.ccd.<arm>.temperature.warm.fail
	 * <li>frodospec.get_status.ccd.<arm>.temperature.cold.warn
	 * <li>frodospec.get_status.ccd.<arm>.temperature.cold.fail
	 * </ul>
	 * @param arm Which arm, one of RED_ARM or BLUE_ARM, 0 is an illegal index.
	 * @param currentTemperature The current temperature in degrees C.
	 * @exception NumberFormatException Thrown if the config is not a valid double.
	 * @exception IllegalArgumentException Throwm if arm is out of range.
	 * @see #hashTable
	 * @see #status
	 * @see #armDetectorTemperatureInstrumentStatus
	 * @see FrodoSpecConstants#ARM_STRING_LIST
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#KEYWORD_DETECTOR_TEMPERATURE_INSTRUMENT_STATUS
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#VALUE_STATUS_OK
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#VALUE_STATUS_WARN
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#VALUE_STATUS_FAIL
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	protected void setDetectorTemperatureInstrumentStatus(int arm,double currentTemperature)
		throws NumberFormatException, IllegalArgumentException
	{
		double warmWarnTemperature,warmFailTemperature,coldWarnTemperature,coldFailTemperature;

		if((arm != FrodoSpecConfig.RED_ARM)&&(arm != FrodoSpecConfig.BLUE_ARM))
		{
			throw new IllegalArgumentException(this.getClass().getName()+
						     ":setDetectorTemperatureInstrumentStatus: Illegal arm:"+arm);
		}
		// get config for warn and fail temperatures for this arm
		warmWarnTemperature = status.getPropertyDouble("frodospec.get_status.ccd."+
							       FrodoSpecConstants.ARM_STRING_LIST[arm]+
							       ".temperature.warm.warn");
		warmFailTemperature = status.getPropertyDouble("frodospec.get_status.ccd."+
							       FrodoSpecConstants.ARM_STRING_LIST[arm]+
							       ".temperature.warm.fail");
		coldWarnTemperature = status.getPropertyDouble("frodospec.get_status.ccd."+
							       FrodoSpecConstants.ARM_STRING_LIST[arm]+
							       ".temperature.cold.warn");
		coldFailTemperature = status.getPropertyDouble("frodospec.get_status.ccd."+
							       FrodoSpecConstants.ARM_STRING_LIST[arm]+
							       ".temperature.cold.fail");
		// set status
		if(currentTemperature > warmFailTemperature)
			armDetectorTemperatureInstrumentStatus[arm] = GET_STATUS_DONE.VALUE_STATUS_FAIL;
		else if(currentTemperature > warmWarnTemperature)
			armDetectorTemperatureInstrumentStatus[arm] = GET_STATUS_DONE.VALUE_STATUS_WARN;
		else if(currentTemperature < coldFailTemperature)
			armDetectorTemperatureInstrumentStatus[arm] = GET_STATUS_DONE.VALUE_STATUS_FAIL;
		else if(currentTemperature < coldWarnTemperature)
			armDetectorTemperatureInstrumentStatus[arm] = GET_STATUS_DONE.VALUE_STATUS_WARN;
		else
			armDetectorTemperatureInstrumentStatus[arm] = GET_STATUS_DONE.VALUE_STATUS_OK;
		// set hashtable entry
		hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+"."+
			      GET_STATUS_DONE.KEYWORD_DETECTOR_TEMPERATURE_INSTRUMENT_STATUS,
			      armDetectorTemperatureInstrumentStatus[arm]);
	}

	/**
	 * Sets: 
	 * <ul>
	 * <li>Overall detector temperature status (<i>KEYWORD_DETECTOR_TEMPERATURE_INSTRUMENT_STATUS</i>) based
	 *     on armDetectorTemperatureInstrumentStatus.
	 * <li>Per-arm PLC status in <i>&lt;arm&gt;.KEYWORD_PLC_INSTRUMENT_STATUS</i>, 
	 *     based on subsets of plcFaultStatus.
	 * <li>Overall PLC status in <i>KEYWORD_PLC_INSTRUMENT_STATUS</i>, 
	 *     based on all of plcFaultStatus.
	 * <li>Per-arm Focus Stage status in <i>&lt;arm&gt;.KEYWORD_FOCUS_STAGE_INSTRUMENT_STATUS</i>, 
	 *     based on focusStageInstrumentStatusString[arm].
	 * <li>Overall Focus Stage status in <i>KEYWORD_FOCUS_STAGE_INSTRUMENT_STATUS</i>, 
	 *     based on focusStageInstrumentStatusString.
	 * <li>Overall per-arm status <i>&lt;arm&gt;.KEYWORD_INSTRUMENT_STATUS</i>, based on
	 *     armDetectorTemperatureInstrumentStatus, the per-arm PLC status and per-arm focus stage status.
	 * <li>the overall instrument status keyword <i>KEYWORD_INSTRUMENT_STATUS</i> in the hashtable. 
	 *     This is derived from sub-system keyword values armDetectorTemperatureInstrumentStatus, 
	 *     the overall PLC status and focus stage status.
	 * </ul>
	 * Each is set to the worst of OK/WARN/FAIL. If sub-systems are UNKNOWN, OK is returned.
	 * @param plcFaultStatus The Fault status bits from the PLC. Depending on the fault bits
	 *        instrument status may change.
	 * @param focusStageInstrumentStatusString An array of three strings, the status value strings for each
	 *        arm's focus stage. The first index is VALUE_STATUS_UNKNOWN.
	 * @see #hashTable
	 * @see #status
	 * @see #armDetectorTemperatureInstrumentStatus
	 * @see #instrumentStatus
	 * @see #sdsuCommsInstrumentStatus
	 * @see #KEYWORD_PLC_INSTRUMENT_STATUS
	 * @see #KEYWORD_FOCUS_STAGE_INSTRUMENT_STATUS
	 * @see #KEYWORD_SDSU_COMMS_INSTRUMENT_STATUS
	 * @see Plc#FAULT_STATUS_AIR_PRESSURE_HIGH
	 * @see Plc#FAULT_STATUS_AIR_PRESSURE_LOW
	 * @see Plc#FAULT_STATUS_HUMIDITY_HIGH
	 * @see Plc#FAULT_STATUS_COOLING
	 * @see Plc#FAULT_STATUS_INST_TEMPERATURE_HIGH
	 * @see Plc#FAULT_STATUS_PANEL_TEMPERATURE_HIGH
	 * @see Plc#FAULT_STATUS_GRATING_POSITION_RED_HIGH
	 * @see Plc#FAULT_STATUS_GRATING_POSITION_RED_LOW
	 * @see Plc#FAULT_STATUS_SHUTTER_RED_OPEN
	 * @see Plc#FAULT_STATUS_SHUTTER_RED_CLOSE
	 * @see Plc#FAULT_STATUS_GRATING_POSITION_BLUE_HIGH
	 * @see Plc#FAULT_STATUS_GRATING_POSITION_BLUE_LOW
	 * @see Plc#FAULT_STATUS_SHUTTER_BLUE_OPEN
	 * @see Plc#FAULT_STATUS_SHUTTER_BLUE_CLOSE
	 * @see FrodoSpecConstants#ARM_STRING_LIST
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#KEYWORD_DETECTOR_TEMPERATURE_INSTRUMENT_STATUS
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#KEYWORD_INSTRUMENT_STATUS
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#VALUE_STATUS_OK
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#VALUE_STATUS_WARN
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#VALUE_STATUS_FAIL
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	// diddly rewrite as per-arm-hardware and per hardware status written in separate methods are per detector
	// temperature
	// this method to agregate into per-arm and overall status
	protected void setInstrumentStatus(int plcFaultStatus,String focusStageInstrumentStatusString[])
	{
		String armInstrumentStatus;
		String detectorTemperatureInstrumentStatus;
		String plcArmInstrumentStatus[] = {GET_STATUS_DONE.VALUE_STATUS_OK,GET_STATUS_DONE.VALUE_STATUS_OK,
						   GET_STATUS_DONE.VALUE_STATUS_OK};
		String plcInstrumentStatus;
		String focusStageInstrumentStatus;
		// fault status bits that set the _overall_ instrument status to fail
		int plcFaultStatusFailMask = Plc.FAULT_STATUS_AIR_PRESSURE_LOW;
		// fault status bits that set the _overall_ instrument status to warn
		int plcFaultStatusWarnMask = Plc.FAULT_STATUS_AIR_PRESSURE_HIGH|
			Plc.FAULT_STATUS_HUMIDITY_HIGH|Plc.FAULT_STATUS_COOLING|
			Plc.FAULT_STATUS_INST_TEMPERATURE_HIGH|Plc.FAULT_STATUS_PANEL_TEMPERATURE_HIGH;
		// fault status bits that set the _per arm_ (and therefore also overall) instrument status to warn
		int plcFaultStatusArmWarnMask[] = {0,
			 // red arm
			 Plc.FAULT_STATUS_GRATING_POSITION_RED_HIGH|Plc.FAULT_STATUS_GRATING_POSITION_RED_LOW|
			 Plc.FAULT_STATUS_SHUTTER_RED_OPEN|Plc.FAULT_STATUS_SHUTTER_RED_CLOSE,
			 // blue arm
			 Plc.FAULT_STATUS_GRATING_POSITION_BLUE_HIGH|Plc.FAULT_STATUS_GRATING_POSITION_BLUE_LOW|
			 Plc.FAULT_STATUS_SHUTTER_BLUE_OPEN|Plc.FAULT_STATUS_SHUTTER_BLUE_CLOSE};
		// SDSU comms per-arm status
		for(int arm = FrodoSpecConfig.RED_ARM; arm <= FrodoSpecConfig.BLUE_ARM; arm++)
		{
			hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+"."+
				      KEYWORD_SDSU_COMMS_INSTRUMENT_STATUS,sdsuCommsInstrumentStatus[arm]);
		}
		// no overall SDSU comms status at the moment - how would this be useful?
		// set overall detectorTemperatureInstrumentStatus based on each arm
		detectorTemperatureInstrumentStatus = GET_STATUS_DONE.VALUE_STATUS_OK;
		for(int arm = FrodoSpecConfig.RED_ARM; arm <= FrodoSpecConfig.BLUE_ARM; arm++)
		{
			// only go to warn if we are in OK, if we are already fail keep that
			if(armDetectorTemperatureInstrumentStatus[arm].equals(GET_STATUS_DONE.VALUE_STATUS_WARN)&&
			   detectorTemperatureInstrumentStatus.equals(GET_STATUS_DONE.VALUE_STATUS_OK))
				detectorTemperatureInstrumentStatus = GET_STATUS_DONE.VALUE_STATUS_WARN;
			if(armDetectorTemperatureInstrumentStatus[arm].equals(GET_STATUS_DONE.VALUE_STATUS_FAIL))
				detectorTemperatureInstrumentStatus = GET_STATUS_DONE.VALUE_STATUS_FAIL;
			// UNKNOWN - leave overall as it is
		}
		// set overall detectorTemperatureInstrumentStatus  hashtable entry
		hashTable.put(GET_STATUS_DONE.KEYWORD_DETECTOR_TEMPERATURE_INSTRUMENT_STATUS,
			      detectorTemperatureInstrumentStatus);
		// plc per-arm status
		for(int arm = FrodoSpecConfig.RED_ARM; arm <= FrodoSpecConfig.BLUE_ARM; arm++)
		{
			// initialise to OK
			plcArmInstrumentStatus[arm] = GET_STATUS_DONE.VALUE_STATUS_OK;
			// general plc faults propogate into both arms as they affect both arms
			if((plcFaultStatus & plcFaultStatusWarnMask) > 0)
				plcArmInstrumentStatus[arm] = GET_STATUS_DONE.VALUE_STATUS_WARN;
			// check each arm for plc problems
			if((plcFaultStatus & plcFaultStatusArmWarnMask[arm]) > 0)
				plcArmInstrumentStatus[arm] = GET_STATUS_DONE.VALUE_STATUS_WARN;
			// general plc fail fault
			if((plcFaultStatus & plcFaultStatusFailMask) > 0)
				plcArmInstrumentStatus[arm] = GET_STATUS_DONE.VALUE_STATUS_FAIL;
			// set per-arm status in hashtable
			hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+"."+
				      KEYWORD_PLC_INSTRUMENT_STATUS,plcArmInstrumentStatus[arm]);
		}
		// overall plc status
		plcInstrumentStatus = GET_STATUS_DONE.VALUE_STATUS_OK;
		for(int arm = FrodoSpecConfig.RED_ARM; arm <= FrodoSpecConfig.BLUE_ARM; arm++)
		{
			// both arms use the overall plcFaultStatusWarnMask in their status,
			// so no need to fold that in here
			// only go into warn if we are in OK - if we are already fail keep that
			if(plcArmInstrumentStatus[arm].equals(GET_STATUS_DONE.VALUE_STATUS_WARN)&&
			   plcInstrumentStatus.equals(GET_STATUS_DONE.VALUE_STATUS_OK))
				plcInstrumentStatus = GET_STATUS_DONE.VALUE_STATUS_WARN;
			// both arms use the overall plcFaultStatusFailMask in their status
			// so no need to fold that in here.
			// If either arm is in fail, the whole PLC is in fail.
			if(plcArmInstrumentStatus[arm].equals(GET_STATUS_DONE.VALUE_STATUS_FAIL))
				plcInstrumentStatus = GET_STATUS_DONE.VALUE_STATUS_FAIL;
		}
		// over all PLC status (based on fault bits)
		hashTable.put(KEYWORD_PLC_INSTRUMENT_STATUS,plcInstrumentStatus);
		// per-arm focus stage
		for(int arm = FrodoSpecConfig.RED_ARM; arm <= FrodoSpecConfig.BLUE_ARM; arm++)
		{
			// set per-arm status in hashtable
			hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+"."+
				      KEYWORD_FOCUS_STAGE_INSTRUMENT_STATUS,focusStageInstrumentStatusString[arm]);
		}
		// overall focus stage
		focusStageInstrumentStatus = GET_STATUS_DONE.VALUE_STATUS_OK;
		for(int arm = FrodoSpecConfig.RED_ARM; arm <= FrodoSpecConfig.BLUE_ARM; arm++)
		{
			// only go into warn if we are in OK - if we are already fail keep that
			if(focusStageInstrumentStatusString[arm].equals(GET_STATUS_DONE.VALUE_STATUS_WARN)&&
			   focusStageInstrumentStatus.equals(GET_STATUS_DONE.VALUE_STATUS_OK))
				focusStageInstrumentStatus = GET_STATUS_DONE.VALUE_STATUS_WARN;
			if(focusStageInstrumentStatusString[arm].equals(GET_STATUS_DONE.VALUE_STATUS_FAIL))
				focusStageInstrumentStatus = GET_STATUS_DONE.VALUE_STATUS_FAIL;
		}
		hashTable.put(KEYWORD_FOCUS_STAGE_INSTRUMENT_STATUS,focusStageInstrumentStatus);
		// set overall per-arm status
		for(int arm = FrodoSpecConfig.RED_ARM; arm <= FrodoSpecConfig.BLUE_ARM; arm++)
		{
			// initialise to OK
			armInstrumentStatus = GET_STATUS_DONE.VALUE_STATUS_OK;
			//
			// check for warnings
			//
			// check each arm for temperature problems
			if(armDetectorTemperatureInstrumentStatus[arm].equals(GET_STATUS_DONE.VALUE_STATUS_WARN))
				armInstrumentStatus = GET_STATUS_DONE.VALUE_STATUS_WARN;
			// check for SDSU comms problems
			if(sdsuCommsInstrumentStatus[arm].equals(GET_STATUS_DONE.VALUE_STATUS_WARN))
				armInstrumentStatus = GET_STATUS_DONE.VALUE_STATUS_WARN;
			// check PLC for each arm
			if(plcArmInstrumentStatus[arm].equals(GET_STATUS_DONE.VALUE_STATUS_WARN))
				armInstrumentStatus = GET_STATUS_DONE.VALUE_STATUS_WARN;
			// check focus stage
			if(focusStageInstrumentStatusString[arm].equals(GET_STATUS_DONE.VALUE_STATUS_WARN))
				armInstrumentStatus = GET_STATUS_DONE.VALUE_STATUS_WARN;
			//
			// check fails
			//
			// check for SDSU comms problems
			if(sdsuCommsInstrumentStatus[arm].equals(GET_STATUS_DONE.VALUE_STATUS_FAIL))
				armInstrumentStatus = GET_STATUS_DONE.VALUE_STATUS_FAIL;
			// check each arm for temperature problems
			if(armDetectorTemperatureInstrumentStatus[arm].equals(GET_STATUS_DONE.VALUE_STATUS_FAIL))
				armInstrumentStatus = GET_STATUS_DONE.VALUE_STATUS_FAIL;
			// check PLC for each arm
			if(plcArmInstrumentStatus[arm].equals(GET_STATUS_DONE.VALUE_STATUS_FAIL))
				armInstrumentStatus = GET_STATUS_DONE.VALUE_STATUS_FAIL;
			// check focus stage
			if(focusStageInstrumentStatusString[arm].equals(GET_STATUS_DONE.VALUE_STATUS_FAIL))
				armInstrumentStatus = GET_STATUS_DONE.VALUE_STATUS_FAIL;
			// UNKNOWN - leave overall as it is
			// set per-arm status in hashtable
			hashTable.put(FrodoSpecConstants.ARM_STRING_LIST[arm]+"."+
				      GET_STATUS_DONE.KEYWORD_INSTRUMENT_STATUS,armInstrumentStatus);
		}
		// set overall default to OK
		instrumentStatus = GET_STATUS_DONE.VALUE_STATUS_OK;
		for(int arm = FrodoSpecConfig.RED_ARM; arm <= FrodoSpecConfig.BLUE_ARM; arm++)
		{
			//
			// check for warnings
			//
			// if a sub-status is in warning, overall status is in warning, if overall status
			// is currently OK (i.e. other arm has not already set instrument status to fail).
			if(sdsuCommsInstrumentStatus[arm].equals(GET_STATUS_DONE.VALUE_STATUS_WARN)&&
			   instrumentStatus.equals(GET_STATUS_DONE.VALUE_STATUS_OK))
				instrumentStatus = GET_STATUS_DONE.VALUE_STATUS_WARN;
			if(armDetectorTemperatureInstrumentStatus[arm].equals(GET_STATUS_DONE.VALUE_STATUS_WARN)&&
			   instrumentStatus.equals(GET_STATUS_DONE.VALUE_STATUS_OK))
				instrumentStatus = GET_STATUS_DONE.VALUE_STATUS_WARN;
			if(plcArmInstrumentStatus[arm].equals(GET_STATUS_DONE.VALUE_STATUS_WARN)&&
			   instrumentStatus.equals(GET_STATUS_DONE.VALUE_STATUS_OK))
				instrumentStatus = GET_STATUS_DONE.VALUE_STATUS_WARN;
			if(focusStageInstrumentStatusString[arm].equals(GET_STATUS_DONE.VALUE_STATUS_WARN)&&
			   instrumentStatus.equals(GET_STATUS_DONE.VALUE_STATUS_OK))
				instrumentStatus = GET_STATUS_DONE.VALUE_STATUS_WARN;
			//
			// check fails
			//
			// if a sub-status is in fail, overall status is in fail. This overrides a previous warn
			if(sdsuCommsInstrumentStatus[arm].equals(GET_STATUS_DONE.VALUE_STATUS_FAIL))
				instrumentStatus = GET_STATUS_DONE.VALUE_STATUS_FAIL;
			if(armDetectorTemperatureInstrumentStatus[arm].equals(GET_STATUS_DONE.VALUE_STATUS_FAIL))
				instrumentStatus = GET_STATUS_DONE.VALUE_STATUS_FAIL;
			if(plcArmInstrumentStatus[arm].equals(GET_STATUS_DONE.VALUE_STATUS_FAIL))
				instrumentStatus = GET_STATUS_DONE.VALUE_STATUS_FAIL;
			if(focusStageInstrumentStatusString[arm].equals(GET_STATUS_DONE.VALUE_STATUS_FAIL))
				instrumentStatus = GET_STATUS_DONE.VALUE_STATUS_FAIL;
		}
		// set standard status in hashtable
		hashTable.put(GET_STATUS_DONE.KEYWORD_INSTRUMENT_STATUS,instrumentStatus);
	}

	/**
	 * Method to get misc status, when level FULL has been selected.
	 * The following data is put into the hashTable:
	 * <ul>
	 * <li><b>Log Level</b> The current log level used by FrodoSpec.
	 * <li><b>Disk Usage</b> The results of running a &quot;df -k&quot;, to get the disk usage.
	 * <li><b>Process List</b> The results of running a &quot;ps -e -o pid,pcpu,vsz,ruser,stime,time,args&quot;, 
	 * 	to get the processes running on this machine.
	 * <li><b>Uptime</b> The results of running a &quot;uptime&quot;, 
	 * 	to get system load and time since last reboot.
	 * <li><b>Total Memory, Free Memory</b> The total and free memory in the Java virtual machine.
	 * <li><b>java.version, java.vendor, java.home, java.vm.version, java.vm.vendor, java.class.path</b> 
	 * 	Java virtual machine version, classpath and type.
	 * <li><b>os.name, os.arch, os.version</b> The operating system type/version.
	 * <li><b>user.name, user.home, user.dir</b> Data about the user the process is running as.
	 * <li><b>thread.list</b> A list of threads the FrodoSpec process is running.
	 * </ul>
	 * @see #serverConnectionThread
	 * @see #hashTable
	 * @see ExecuteCommand#run
	 * @see FrodoSpecStatus#getLogLevel
	 */
	private void getFullStatus()
	{
		ExecuteCommand executeCommand = null;
		Runtime runtime = null;
		StringBuffer sb = null;
		Thread threadList[] = null;
		int threadCount;

		// log level
		hashTable.put("Log Level",new Integer(status.getLogLevel()));
		// execute 'df -k' on instrument computer
		executeCommand = new ExecuteCommand("df -k");
		executeCommand.run();
		if(executeCommand.getException() == null)
			hashTable.put("Disk Usage",new String(executeCommand.getOutputString()));
		else
			hashTable.put("Disk Usage",new String(executeCommand.getException().toString()));
		// execute "ps -e -o pid,pcpu,vsz,ruser,stime,time,args" on instrument computer
		executeCommand = new ExecuteCommand("ps -e -o pid,pcpu,vsz,ruser,stime,time,args");
		executeCommand.run();
		if(executeCommand.getException() == null)
			hashTable.put("Process List",new String(executeCommand.getOutputString()));
		else
			hashTable.put("Process List",new String(executeCommand.getException().toString()));
		// execute "uptime" on instrument computer
		executeCommand = new ExecuteCommand("uptime");
		executeCommand.run();
		if(executeCommand.getException() == null)
			hashTable.put("Uptime",new String(executeCommand.getOutputString()));
		else
			hashTable.put("Uptime",new String(executeCommand.getException().toString()));
		// get vm memory situation
		runtime = Runtime.getRuntime();
		hashTable.put("Free Memory",new Long(runtime.freeMemory()));
		hashTable.put("Total Memory",new Long(runtime.totalMemory()));
		// get some java vm information
		hashTable.put("java.version",new String(System.getProperty("java.version")));
		hashTable.put("java.vendor",new String(System.getProperty("java.vendor")));
		hashTable.put("java.home",new String(System.getProperty("java.home")));
		hashTable.put("java.vm.version",new String(System.getProperty("java.vm.version")));
		hashTable.put("java.vm.vendor",new String(System.getProperty("java.vm.vendor")));
		hashTable.put("java.class.path",new String(System.getProperty("java.class.path")));
		hashTable.put("os.name",new String(System.getProperty("os.name")));
		hashTable.put("os.arch",new String(System.getProperty("os.arch")));
		hashTable.put("os.version",new String(System.getProperty("os.version")));
		hashTable.put("user.name",new String(System.getProperty("user.name")));
		hashTable.put("user.home",new String(System.getProperty("user.home")));
		hashTable.put("user.dir",new String(System.getProperty("user.dir")));
		// get a list of threads running in the vm
		threadCount = serverConnectionThread.activeCount();
		threadList = new Thread[threadCount];
		serverConnectionThread.enumerate(threadList);
		sb = new StringBuffer();
		for(int i = 0;i< threadCount;i++)
		{
			if(threadList[i] != null)
			{
				sb.append(threadList[i].getName());
				sb.append("\n");
			}
		}
		hashTable.put("thread.list",sb.toString());
	}
}

//
// $Log: not supported by cvs2svn $
// Revision 1.1  2008/11/20 11:33:35  cjm
// Initial revision
//
//

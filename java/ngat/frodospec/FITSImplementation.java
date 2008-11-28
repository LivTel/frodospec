// FITSImplementation.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/FITSImplementation.java,v 1.3 2008-11-28 11:15:56 cjm Exp $
package ngat.frodospec;

import java.lang.*;
import java.util.Date;
import java.util.List;
import java.util.Vector;

import ngat.message.base.*;
import ngat.message.ISS_INST.*;
import ngat.phase2.*;
import ngat.eip.*;
import ngat.fits.*;
import ngat.frodospec.ccd.*;
import ngat.lamp.*;

/**
 * This class provides the generic implementation of commands that write FITS files. It extends those that
 * use the hardware  libraries as this is needed to generate FITS files.
 * @see HardwareImplementation
 * @author Chris Mottram
 * @version $Revision: 1.3 $
 */
public class FITSImplementation extends HardwareImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: FITSImplementation.java,v 1.3 2008-11-28 11:15:56 cjm Exp $");
	/**
	 * A reference to the FrodoSpecStatus class instance that holds status information for the FrodoSpec.
	 */
	protected FrodoSpecStatus status = null;
	/**
	 * A reference to the FitsFilename class instances used to generate unique FITS filenames.
	 */
	protected FitsFilename frodospecFilenameList[] = {null,null,null};
	/**
	 * A local reference to the FitsHeader objects held in FrodoSpec. 
	 * These are used for writing FITS headers to disk
	 * and setting the values of card images within the headers.
	 */
	protected FitsHeader frodospecFitsHeaderList[] = {null,null,null};
	/**
	 * A local reference to the FitsHeaderDefaults objects held in the FrodoSpec. 
	 * These are used to supply default values, units and comments for FITS header card images.
	 */
	protected FitsHeaderDefaults frodospecFitsHeaderDefaultsList[] = {null,null,null};
	/**
	 * This is a copy of the "OBJECT" FITS Header value retrieved from the RCS by getFitsHeadersFromISS.
	 * This is used to modify the "OBJECT" FITS Header value for ARCs etc....
	 * There is only one saved value rather than one per arm, surely as there is only one fibre
	 * FrodoSpec can only be pointing at one object at a time?
	 * @see #getFitsHeadersFromISS
	 */
	protected String objectName = null;
	/**
	 * Internal constant used when converting temperatures in centigrade (from the CCD controller/CCS
	 * configuration file) to Kelvin (used in FITS file). Used in setFitsHeaders.
	 * @see #setFitsHeaders
	 */
	private final static double CENTIGRADE_TO_KELVIN = 273.15;
	/**
	 * Internal constant used when the order number offset defined in the property
	 * 'frodospec.get_fits.order_number_offset' is not found or is not a valid number.
	 * @see #getFitsHeadersFromISS
	 */
	private final static int DEFAULT_ORDER_NUMBER_OFFSET = 255;

	/**
	 * This method calls the super-classes method, and tries to fill in the reference to the
	 * FITS filename object, the FITS header object and the FITS default value object.
	 * @param command The command to be implemented.
	 * @see #status
	 * @see FrodoSpec#getStatus
	 * @see #frodospecFilenameList
	 * @see FrodoSpec#getFitsFilename
	 * @see #frodospecFitsHeaderList
	 * @see FrodoSpec#getFitsHeader
	 * @see #frodospecFitsHeaderDefaultsList
	 * @see FrodoSpec#getFitsHeaderDefaults
	 */
	public void init(COMMAND command)
	{
		super.init(command);
		if(frodospec != null)
		{
			status = frodospec.getStatus();
			// for each arm (FrodoSpecConfig.RED_ARM=1,FrodoSpecConfig.BLUE_ARM=2)
			for(int i=FrodoSpecConfig.RED_ARM; i<=FrodoSpecConfig.BLUE_ARM; i++)
			{
				frodospecFilenameList[i] = frodospec.getFitsFilename(i);
				frodospecFitsHeaderList[i] = frodospec.getFitsHeader(i);
				frodospecFitsHeaderDefaultsList[i] = frodospec.getFitsHeaderDefaults(i);
			}
		}
	}

	/**
	 * This method is used to calculate how long an implementation of a command is going to take, so that the
	 * client has an idea of how long to wait before it can assume the server has died.
	 * @param command The command to be implemented.
	 * @return The time taken to implement this command, or the time taken before the next acknowledgement
	 * is to be sent.
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		return super.calculateAcknowledgeTime(command);
	}

	/**
	 * This routine performs the generic command implementation.
	 * @param command The command to be implemented.
	 * @return The results of the implementation of this command.
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		return super.processCommand(command);
	}

	/**
	 * This routine tries to move the mirror fold to the stowed location, by issuing a MOVE_FOLD command
	 * to the ISS. The position to move the fold to is specified by the scs property file 
	 * (frodospec.mirror_fold.stow). This is done to move the fold mirror out of the way for ARC calibration etc...
	 * If an error occurs the done objects field's are set accordingly.
	 * @param command The command being implemented that made this call to the ISS. This is used
	 * 	for error logging.
	 * @param done A COMMAND_DONE subclass specific to the command being implemented. If an
	 * 	error occurs the relevant fields are filled in with the error.
	 * @return The routine returns a boolean to indicate whether the operation was completed
	 *  	successfully.
	 * @see FrodoSpecStatus#getPropertyInteger
	 * @see #moveFold(COMMAND,COMMAND_DONE,int)
	 */
	public boolean stowFold(COMMAND command,COMMAND_DONE done)
	{
		int mirrorFoldPosition = 0;

		try
		{
			mirrorFoldPosition = status.getPropertyInteger("frodospec.mirror_fold.stow");
		}
		catch(NumberFormatException e)
		{
			mirrorFoldPosition = 0;
			frodospec.error(this.getClass().getName()+":stowFold:"+
					command.getClass().getName()+":"+e);
			done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+313);
			done.setErrorString("stowFold:"+e);
			done.setSuccessful(false);
			return false;
		}
		return moveFold(command,done,mirrorFoldPosition);
	}

	/**
	 * This routine tries to move the mirror fold to a certain location, by issuing a MOVE_FOLD command
	 * to the ISS. The position to move the fold to is specified by the frodospec property file.
	 * If an error occurs the done objects field's are set accordingly.
	 * @param command The command being implemented that made this call to the ISS. This is used
	 * 	for error logging.
	 * @param done A COMMAND_DONE subclass specific to the command being implemented. If an
	 * 	error occurs the relevant fields are filled in with the error.
	 * @return The routine returns a boolean to indicate whether the operation was completed
	 *  	successfully.
	 * @see FrodoSpecStatus#getPropertyInteger
	 * @see FrodoSpec#sendISSCommand
	 * @see #moveFold(COMMAND,COMMAND_DONE,int)
	 */
	public boolean moveFold(COMMAND command,COMMAND_DONE done)
	{
		INST_TO_ISS_DONE instToISSDone = null;
		MOVE_FOLD moveFold = null;
		int mirrorFoldPosition = 0;

		moveFold = new MOVE_FOLD(command.getId());
		try
		{
			mirrorFoldPosition = status.getPropertyInteger("frodospec.mirror_fold.position");
		}
		catch(NumberFormatException e)
		{
			mirrorFoldPosition = 0;
			frodospec.error(this.getClass().getName()+":moveFold:"+
				command.getClass().getName(),e);
			done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+300);
			done.setErrorString("moveFold:"+e);
			done.setSuccessful(false);
			return false;
		}
		return moveFold(command,done,mirrorFoldPosition);
	}

	/**
	 * This routine tries to move the mirror fold to the specified location, by issuing a MOVE_FOLD command
	 * to the ISS.
	 * If an error occurs the done objects field's are set accordingly.
	 * @param command The command being implemented that made this call to the ISS. This is used
	 * 	for error logging.
	 * @param done A COMMAND_DONE subclass specific to the command being implemented. If an
	 * 	error occurs the relevant fields are filled in with the error.
	 * @param mirrorFoldPosition The position to move the fold mirror to.
	 * @return The routine returns a boolean to indicate whether the operation was completed
	 *  	successfully.
	 * @see FrodoSpec#sendISSCommand
	 */
	public boolean moveFold(COMMAND command,COMMAND_DONE done,int mirrorFoldPosition)
	{
		INST_TO_ISS_DONE instToISSDone = null;
		MOVE_FOLD moveFold = null;

		moveFold = new MOVE_FOLD(command.getId());
		frodospec.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,
			this.getClass().getName()+":moveFold:"+command.getClass().getName()+
			":position = "+mirrorFoldPosition);
		moveFold.setMirror_position(mirrorFoldPosition);
		instToISSDone = frodospec.sendISSCommand(moveFold,serverConnectionThread);
		if(instToISSDone.getSuccessful() == false)
		{
			frodospec.error(this.getClass().getName()+":moveFold:"+
				command.getClass().getName()+":"+instToISSDone.getErrorString());
			done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+301);
			done.setErrorString(instToISSDone.getErrorString());
			done.setSuccessful(false);		
			return false;
		}
		return true;
	}

	/**
	 * This routine tries to start the autoguider, by issuing a AG_START command
	 * to the ISS.
	 * If an error occurs the done objects field's can be set accordingly.
	 * @param command The command being implemented that made this call to the ISS. This is used
	 * 	for error logging.
	 * @param done A COMMAND_DONE subclass specific to the command being implemented. If an
	 * 	error occurs the relevant fields are filled in with the error.
	 * @return The routine returns a boolean to indicate whether the operation was completed
	 *  	successfully.
	 * @see FrodoSpec#sendISSCommand
	 */
	public boolean autoguiderStart(COMMAND command,COMMAND_DONE done)
	{
		INST_TO_ISS_DONE instToISSDone = null;

		instToISSDone = frodospec.sendISSCommand(new AG_START(command.getId()),serverConnectionThread);
		if(instToISSDone.getSuccessful() == false)
		{
			frodospec.error(this.getClass().getName()+":autoguiderStart:"+
				command.getClass().getName()+":"+instToISSDone.getErrorString());
			done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+302);
			done.setErrorString(instToISSDone.getErrorString());
			done.setSuccessful(false);		
			return false;
		}
		return true;
	}

	/**
	 * This routine tries to stop the autoguider, by issuing a AG_STOP command
	 * to the ISS.
	 * If an error occurs the done objects field's is set accordingly.
	 * @param command The command being implemented that made this call to the ISS. This is used
	 * 	for error logging.
	 * @param done A COMMAND_DONE subclass specific to the command being implemented. If an
	 * 	error occurs the relevant fields are filled in with the error.
	 * @param checkAbort Whether sendISSCommand should check the thread's abort flag. This should be
	 * 	true for normal operation, and false when autoguiderStop is being used in response
	 * 	to a previously trapped abort (i.e. an exposure has been aborted).
	 * @return The routine returns a boolean to indicate whether the operation was completed
	 *  	successfully.
	 * @see FrodoSpec#sendISSCommand
	 */
	public boolean autoguiderStop(COMMAND command,COMMAND_DONE done,boolean checkAbort)
	{
		INST_TO_ISS_DONE instToISSDone = null;

		instToISSDone = frodospec.sendISSCommand(new AG_STOP(command.getId()),
							 serverConnectionThread,checkAbort);
		if(instToISSDone.getSuccessful() == false)
		{
			frodospec.error(this.getClass().getName()+":autoguiderStop:"+
				command.getClass().getName()+":"+instToISSDone.getErrorString());
			done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+303);
			done.setErrorString(instToISSDone.getErrorString());
			done.setSuccessful(false);		
			return false;
		}
		return true;
	}

	/**
	 * This routine clears the current set of FITS headers. The FITS headers are held in the main FrodoSpec
	 * object. This is retrieved and the relevant method called.
	 * @param arm Which arm. Either FrodoSpecConfig.RED_ARM or FrodoSpecConfig.BLUE_ARM.
	 * @see #frodospecFitsHeaderList
	 * @see ngat.fits.FitsHeader#clearKeywordValueList
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	public void clearFitsHeaders(int arm)  throws IllegalArgumentException
	{
		if((arm != FrodoSpecConfig.RED_ARM)&&(arm != FrodoSpecConfig.BLUE_ARM))
		{
			throw new IllegalArgumentException(this.getClass().getName()+":clearFitsHeaders:Arm "+arm+
							   " out of range.");
		}
		frodospecFitsHeaderList[arm].clearKeywordValueList();
	}

	/**
	 * This routine sets up the Fits Header objects with some keyword value pairs.
	 * It calls the more complicated method below, assuming exposureCount is 1.
	 * @param command The command being implemented that made this call to the ISS. This is used
	 * 	for error logging.
	 * @param done A COMMAND_DONE subclass specific to the command being implemented. If an
	 * 	error occurs the relevant fields are filled in with the error.
	 * @param arm Which arm to retrieve data for.
	 * @param obsTypeString The type of image taken by the camera. This string should be
	 * 	one of the OBSTYPE_VALUE_* defaults in ngat.fits.FitsHeaderDefaults.
	 * @param exposureTime The exposure time,in milliseconds, to put in the EXPTIME keyword. It
	 * 	is converted into decimal seconds (a double).
	 * @return The routine returns a boolean to indicate whether the operation was completed
	 *  	successfully.
	 * @see #setFitsHeaders(COMMAND,COMMAND_DONE,int,String,int,int)
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	public boolean setFitsHeaders(COMMAND command,COMMAND_DONE done,int arm,String obsTypeString,int exposureTime)
	{
		return setFitsHeaders(command,done,arm,obsTypeString,exposureTime,1);
	}

	/**
	 * This routine sets up the Fits Header objects with some keyword value pairs.
	 * <p>The following mandatory keywords are filled in: SIMPLE,BITPIX,NAXIS,NAXIS1,NAXIS2. Note NAXIS1 and
	 * NAXIS2 are retrieved from libfrodospec_ccd, assuming the library has previously been setup with a 
	 * configuration.</p>
	 * <p> A complete list of keywords is constructed from the FrodoSpec FITS defaults file. Some of the values of
	 * these keywords are overwritten by real data obtained from the camera controller, 
	 * or internal FrodoSpec status.
	 * These are:
	 * OBSTYPE, RUNNUM, EXPNUM, EXPTOTAL, DATE, DATE-OBS, UTSTART, MJD, EXPTIME, 
	 * FILTER1, FILTERI1, FILTER2, FILTERI2, CONFIGID, CONFNAME, 
	 * PRESCAN, POSTSCAN, CCDXIMSI, CCDYIMSI, CCDSCALE, CCDRDOUT,
	 * CCDXBIN, CCDYBIN, CCDSTEMP, CCDATEMP, CCDWMODE, CALBEFOR, CALAFTER,
	 * WAVCENT, WAVDISP, WAVRESOL, WAVSHORT, WAVLONG, COLID, CAMID, FOREID, ENVTEMP1.
	 * Windowing keywords CCDWXOFF, CCDWYOFF, CCDWXSIZ, CCDWYSIZ are not implemented at the moment.
	 * Note the DATE, DATE-OBS, UTSTART and MJD keywords are given the value of the current
	 * system time, this value is updated to the exposure start time when the image has been exposed. </p>
	 * @param command The command being implemented that made this call to the ISS. This is used
	 * 	for error logging.
	 * @param done A COMMAND_DONE subclass specific to the command being implemented. If an
	 * 	error occurs the relevant fields are filled in with the error.
	 * @param arm Which arm to retrieve data for.
	 * @param obsTypeString The type of image taken by the camera. This string should be
	 * 	one of the OBSTYPE_VALUE_* defaults in ngat.fits.FitsHeaderDefaults.
	 * @param exposureTime The exposure time,in milliseconds, to put in the EXPTIME keyword. It
	 * 	is converted into decimal seconds (a double).
	 * @param exposureCount The exposure time,in milliseconds, to put in the EXPTOTAL keyword.
	 * @return The routine returns a boolean to indicate whether the operation was completed
	 *  	successfully.
	 * @see #frodospec
	 * @see #frodospecFitsHeaderList
	 * @see #frodospecFitsHeaderDefaultsList
	 * @see HardwareImplementation#redCCD
	 * @see HardwareImplementation#blueCCD
	 * @see FrodoSpec#getPLC
	 * @see FrodoSpec#getLampUnit
	 * @see FrodoSpec#getFocusStage
	 * @see FocusStage#getPosition
	 * @see ngat.frodospec.ccd.CCDLibrary#getNCols
	 * @see ngat.frodospec.ccd.CCDLibrary#getNRows
	 * @see ngat.frodospec.ccd.CCDLibrary#temperatureGet
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 * @see ngat.frodospec.FrodoSpecConstants#ARM_STRING_LIST
	 * @see ngat.lamp.LTAGLampUnit#getLightLevel
	 * @see ngat.lamp.LTAGLampUnit#isLampOn
	 * @see ngat.frodospec.Plc#getGratingPositionString
	 * @see ngat.frodospec.Plc#getTemperature
	 * @see ngat.frodospec.Plc#getHumidity
	 * @see ngat.frodospec.Plc#getInstrumentTemperature
	 * @see ngat.frodospec.Plc#getPanelTemperature
	 * @see ngat.frodospec.Plc#getAirFlow
	 * @see ngat.frodospec.Plc#getAirPressure
	 * @see ngat.frodospec.Plc#getCoolingTimeOn
	 * @see ngat.frodospec.Plc#getLinearEncoderPosition
	 * @see ngat.frodospec.Plc#getMechanismStatus
	 * @see ngat.frodospec.Plc#getFaultStatus
	 */
	public boolean setFitsHeaders(COMMAND command,COMMAND_DONE done,int arm,String obsTypeString,
				      int exposureTime,int exposureCount)
	{
		CCDLibrary ccd = null;
		Plc plc = null;
		LTAGLampUnit lampUnit = null;
		FocusStage focusStage = null;
		double actualTemperature = 0.0;
		FitsHeaderCardImage cardImage = null;
		Date date = null;
		Vector defaultFitsHeaderList = null;
		double doubleValue = 0.0;
		int xbin,ybin,ivalue;
		int preScan,postScan;
		String currentResolutionString = null;
		String plcAddress = null;
		String lampName = null;
		float fvalue;

		// which arm do we need the data for
		if(arm == FrodoSpecConfig.RED_ARM)
		{
			ccd = redCCD;
		}
		else if(arm == FrodoSpecConfig.BLUE_ARM)
		{
			ccd = blueCCD;
		}
		else
		{
			String s = new String("Command "+command.getClass().getName()+
				":setFitsHeaders:Illegal arm:"+arm);
			frodospec.error(s);
			done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+305);
			done.setErrorString(s);
			done.setSuccessful(false);
			return false;
		}
		// get PLC
		plc = frodospec.getPLC();
		try
		{
			// actual temperature
			actualTemperature = ccd.temperatureGet();
			// currently configured binning
			xbin = ccd.getXBin();
			ybin = ccd.getYBin();
			// get resolution of grating for this arm
			currentResolutionString = plc.getGratingPositionString(arm);
		// load all the FITS header defaults and put them into the frodospecFitsHeaderList object
			defaultFitsHeaderList = frodospecFitsHeaderDefaultsList[arm].getCardImageList();
			frodospecFitsHeaderList[arm].addKeywordValueList(defaultFitsHeaderList,0);
		// NAXIS1
			cardImage = frodospecFitsHeaderList[arm].get("NAXIS1");
			cardImage.setValue(new Integer(ccd.getNCols()));
		// NAXIS2
			cardImage = frodospecFitsHeaderList[arm].get("NAXIS2");
			cardImage.setValue(new Integer(ccd.getNRows()));
		// OBSTYPE
			cardImage = frodospecFitsHeaderList[arm].get("OBSTYPE");
			cardImage.setValue(obsTypeString);
		// The current MULTRUN number and runNumber are used for these keywords at the moment.
		// They are updated in saveFitsHeaders, when the retrieved values are more likely 
		// to be correct.
		// RUNNUM
			cardImage = frodospecFitsHeaderList[arm].get("RUNNUM");
			cardImage.setValue(new Integer(frodospecFilenameList[arm].getMultRunNumber()));
		// EXPNUM
			cardImage = frodospecFitsHeaderList[arm].get("EXPNUM");
			cardImage.setValue(new Integer(frodospecFilenameList[arm].getRunNumber()));
		// EXPTOTAL
			cardImage = frodospecFitsHeaderList[arm].get("EXPTOTAL");
			cardImage.setValue(new Integer(exposureCount));
		// The DATE,DATE-OBS and UTSTART keywords are saved using the current date/time.
		// This is updated when the data is saved if CFITSIO is used.
			date = new Date();
		// DATE
			cardImage = frodospecFitsHeaderList[arm].get("DATE");
			cardImage.setValue(date);
		// DATE-OBS
			cardImage = frodospecFitsHeaderList[arm].get("DATE-OBS");
			cardImage.setValue(date);
		// UTSTART
			cardImage = frodospecFitsHeaderList[arm].get("UTSTART");
			cardImage.setValue(date);
		// MJD
			cardImage = frodospecFitsHeaderList[arm].get("MJD");
			cardImage.setValue(date);
		// EXPTIME
			cardImage = frodospecFitsHeaderList[arm].get("EXPTIME");
			cardImage.setValue(new Double(((double)exposureTime)/1000.0));
		// FILTER1
			cardImage = frodospecFitsHeaderList[arm].get("FILTER1");
			cardImage.setValue(frodospecFitsHeaderDefaultsList[arm].getValue("FILTER1."+
					   FrodoSpecConstants.ARM_STRING_LIST[arm]+"."+currentResolutionString));
		// FILTERI1
			cardImage = frodospecFitsHeaderList[arm].get("FILTERI1");
			cardImage.setValue(frodospecFitsHeaderDefaultsList[arm].getValue("FILTERI1."+
					   FrodoSpecConstants.ARM_STRING_LIST[arm]+"."+currentResolutionString));
		// CONFIGID
			cardImage = frodospecFitsHeaderList[arm].get("CONFIGID");
			cardImage.setValue(new Integer(status.getConfigId(arm)));
		// CONFNAME
			cardImage = frodospecFitsHeaderList[arm].get("CONFNAME");
			cardImage.setValue(status.getConfigName(arm));
		// PRESCAN
			cardImage = frodospecFitsHeaderList[arm].get("PRESCAN");
			preScan = frodospecFitsHeaderDefaultsList[arm].getValueInteger("PRESCAN."+
				    status.getNumberColumns(xbin)+"."+getCCDRDOUTValue()+"."+xbin);
			cardImage.setValue(new Integer(preScan));
		// POSTSCAN
			cardImage = frodospecFitsHeaderList[arm].get("POSTSCAN");
			postScan = frodospecFitsHeaderDefaultsList[arm].getValueInteger("POSTSCAN."+
				    status.getNumberColumns(xbin)+"."+getCCDRDOUTValue()+"."+xbin);
			cardImage.setValue(new Integer(postScan));
		// CCDXIMSI
		// CCDYIMSI
		// CCDSCALE
		// CCDRDOUT
		// CCDXBIN
			cardImage = frodospecFitsHeaderList[arm].get("CCDXBIN");
			cardImage.setValue(new Integer(xbin));
		// CCDYBIN
			cardImage = frodospecFitsHeaderList[arm].get("CCDYBIN");
			cardImage.setValue(new Integer(ybin));
		// CCDSTEMP
			doubleValue = status.getPropertyDouble("frodospec.ccd."+
				    FrodoSpecConstants.ARM_STRING_LIST[arm]+".config.temperature.target");
			cardImage = frodospecFitsHeaderList[arm].get("CCDSTEMP");
			cardImage.setValue(new Integer((int)(doubleValue+CENTIGRADE_TO_KELVIN)));
		// CCDATEMP
			cardImage = frodospecFitsHeaderList[arm].get("CCDATEMP");
			cardImage.setValue(new Integer((int)(actualTemperature+CENTIGRADE_TO_KELVIN)));
		// windowing keywords
		// CCDWMODE
			cardImage = frodospecFitsHeaderList[arm].get("CCDWMODE");
			cardImage.setValue(new Boolean(false));
		// CALBEFOR
			cardImage = frodospecFitsHeaderList[arm].get("CALBEFOR");
			cardImage.setValue(new Boolean(false));// diddly
		// CALAFTER
			cardImage = frodospecFitsHeaderList[arm].get("CALAFTER");
			cardImage.setValue(new Boolean(false));// diddly
		// ROTCENTX
		// Value specified in config file is unbinned without bias offsets added
			cardImage = frodospecFitsHeaderList[arm].get("ROTCENTX");
			cardImage.setValue(new Integer((frodospecFitsHeaderDefaultsList[arm].
							getValueInteger("ROTCENTX")/xbin)+preScan));
		// ROTCENTY
		// Value specified in config file is unbinned 
			cardImage = frodospecFitsHeaderList[arm].get("ROTCENTY");
			cardImage.setValue(new Integer(frodospecFitsHeaderDefaultsList[arm].
						       getValueInteger("ROTCENTY")/ybin));
		// WAVCENT
			cardImage = frodospecFitsHeaderList[arm].get("WAVCENT");
			cardImage.setValue(new Integer(frodospecFitsHeaderDefaultsList[arm].getValueInteger(
				  "WAVCENT."+FrodoSpecConstants.ARM_STRING_LIST[arm]+"."+currentResolutionString)));
		// WAVDISP
			cardImage = frodospecFitsHeaderList[arm].get("WAVDISP");
			cardImage.setValue(new Double(frodospecFitsHeaderDefaultsList[arm].getValueDouble(
				  "WAVDISP."+FrodoSpecConstants.ARM_STRING_LIST[arm]+"."+currentResolutionString)));
		// WAVRESOL
			cardImage = frodospecFitsHeaderList[arm].get("WAVRESOL");
			cardImage.setValue(new Double(frodospecFitsHeaderDefaultsList[arm].getValueDouble(
				  "WAVRESOL."+FrodoSpecConstants.ARM_STRING_LIST[arm]+"."+currentResolutionString)));
		// WAVSHORT
			cardImage = frodospecFitsHeaderList[arm].get("WAVSHORT");
			cardImage.setValue(new Integer(frodospecFitsHeaderDefaultsList[arm].getValueInteger(
				  "WAVSHORT."+FrodoSpecConstants.ARM_STRING_LIST[arm]+"."+currentResolutionString)));
		// WAVLONG
			cardImage = frodospecFitsHeaderList[arm].get("WAVLONG");
			cardImage.setValue(new Integer(frodospecFitsHeaderDefaultsList[arm].getValueInteger(
				  "WAVLONG."+FrodoSpecConstants.ARM_STRING_LIST[arm]+"."+currentResolutionString)));
		// LAMPS
			lampUnit = frodospec.getLampUnit();
			try
			{
		// LAMPFLUX
				cardImage = frodospecFitsHeaderList[arm].get("LAMPFLUX");
				cardImage.setValue(new Integer(lampUnit.getLightLevel()));
			}
			catch(Exception e)
			{
				String s = new String("Command "+command.getClass().getName()+
						      ":Setting LAMPFLUX Fits Headers failed:");
				frodospec.error(s,e);
				// do not return this error but carry on on lamp failure at the moment
				cardImage.setValue(new String("UNKNOWN"));
			}
		// What is the name of LAMP1
			cardImage = frodospecFitsHeaderList[arm].get("LAMP1TYP");
			lampName = (String)(cardImage.getValue());
		// LAMP1SET
			try
			{
				cardImage = frodospecFitsHeaderList[arm].get("LAMP1SET");
				cardImage.setValue(new Boolean(lampUnit.isLampOn(lampName)));
			}
			catch(Exception e)
			{
				String s = new String("Command "+command.getClass().getName()+
						      ":Setting LAMP1SET Fits Headers failed:");
				frodospec.error(s,e);
				// do not return this error but carry on on lamp failure at the moment
				cardImage.setValue(new String("UNKNOWN"));
			}
		// What is the name of LAMP2
			cardImage = frodospecFitsHeaderList[arm].get("LAMP2TYP");
			lampName = (String)(cardImage.getValue());
		// LAMP2SET
			try
			{
				cardImage = frodospecFitsHeaderList[arm].get("LAMP2SET");
				cardImage.setValue(new Boolean(lampUnit.isLampOn(lampName)));
			}
			catch(Exception e)
			{
				String s = new String("Command "+command.getClass().getName()+
						      ":Setting LAMP2SET Fits Headers failed:");
				frodospec.error(s,e);
				// do not return this error but carry on on lamp failure at the moment
				cardImage.setValue(new String("UNKNOWN"));
			}
		// What is the name of LAMP3
			cardImage = frodospecFitsHeaderList[arm].get("LAMP3TYP");
			lampName = (String)(cardImage.getValue());
		// LAMP3SET
			try
			{
				cardImage = frodospecFitsHeaderList[arm].get("LAMP3SET");
				cardImage.setValue(new Boolean(lampUnit.isLampOn(lampName)));
			}
			catch(Exception e)
			{
				String s = new String("Command "+command.getClass().getName()+
						      ":Setting LAMP3SET Fits Headers failed:");
				frodospec.error(s,e);
				// do not return this error but carry on on lamp failure at the moment
				cardImage.setValue(new String("UNKNOWN"));
			}
		// COLID
			cardImage = frodospecFitsHeaderList[arm].get("COLID");
			cardImage.setValue(frodospecFitsHeaderDefaultsList[arm].getValue("COLID."+
					   FrodoSpecConstants.ARM_STRING_LIST[arm]));
		// CAMID
			cardImage = frodospecFitsHeaderList[arm].get("CAMID");
			cardImage.setValue(frodospecFitsHeaderDefaultsList[arm].getValue("CAMID."+
					   FrodoSpecConstants.ARM_STRING_LIST[arm]));
		// FOREID
			cardImage = frodospecFitsHeaderList[arm].get("FOREID");
			cardImage.setValue(frodospecFitsHeaderDefaultsList[arm].getValue("FOREID."+
					   FrodoSpecConstants.ARM_STRING_LIST[arm]));
	       // COLFOC
			cardImage = frodospecFitsHeaderList[arm].get("COLFOC");
			cardImage.setValue(frodospecFitsHeaderDefaultsList[arm].getValue("COLFOC."+
					   FrodoSpecConstants.ARM_STRING_LIST[arm]));
		// CAMFOC
			cardImage = frodospecFitsHeaderList[arm].get("CAMFOC");
			focusStage = frodospec.getFocusStage(arm);
			fvalue = (float)(focusStage.getPosition());
			cardImage.setValue(new Float(fvalue)); 
		// ENVTEMP[N]
			for(int i = 0; i< Plc.TEMPERATURE_PROBE_COUNT; i++)
			{
				fvalue = plc.getTemperature(i);
				cardImage = frodospecFitsHeaderList[arm].get("ENVTEMP"+i);
				cardImage.setValue(new Float(fvalue+CENTIGRADE_TO_KELVIN));
			}
		// ENVHUM0
			fvalue = plc.getHumidity();
			cardImage = frodospecFitsHeaderList[arm].get("ENVHUM0");
			cardImage.setValue(new Float(fvalue));
		// INSTEMP
			fvalue = plc.getInstrumentTemperature();
			cardImage = frodospecFitsHeaderList[arm].get("INSTEMP");
			cardImage.setValue(new Float(fvalue+CENTIGRADE_TO_KELVIN));
		// PANTEMP
			fvalue = plc.getPanelTemperature();
			cardImage = frodospecFitsHeaderList[arm].get("PANTEMP");
			cardImage.setValue(new Float(fvalue+CENTIGRADE_TO_KELVIN));
		// ENVAIRF
			fvalue = plc.getAirFlow();
			cardImage = frodospecFitsHeaderList[arm].get("ENVAIRF");
			cardImage.setValue(new Float(fvalue));
		// ENVAIRP
			fvalue = plc.getAirPressure();
			cardImage = frodospecFitsHeaderList[arm].get("ENVAIRP");
			cardImage.setValue(new Float(fvalue));
		// COOLTIME
			fvalue = plc.getCoolingTimeOn();
			cardImage = frodospecFitsHeaderList[arm].get("COOLTIME");
			cardImage.setValue(new Float(fvalue));
		// FOCLINEN
			fvalue = plc.getLinearEncoderPosition(arm);
			cardImage = frodospecFitsHeaderList[arm].get("FOCLINEN");
			cardImage.setValue(new Float(fvalue));
		// STATMECH
			ivalue = plc.getMechanismStatus();
			cardImage = frodospecFitsHeaderList[arm].get("STATMECH");
			cardImage.setValue(new Integer(ivalue));
		// STATFALT
			ivalue = plc.getFaultStatus();
			cardImage = frodospecFitsHeaderList[arm].get("STATFALT");
			cardImage.setValue(new Integer(ivalue));
		}// end try
		// ngat.fits.FitsHeaderException thrown by frodospecFitsHeaderDefaults.getValue
		// ngat.util.FileUtilitiesNativeException thrown by FrodoSpecStatus.getConfigId
		// NumberFormatException thrown by FrodoSpecStatus.getFilterWheelName/FrodoSpecStatus.getConfigId
		// Exception thrown by FrodoSpecStatus.getConfigId
		// IllegalArgumentException thrown by plcGetResolution.
		// EIPNativeException thrown by plcGetResolution.
		catch(Exception e)
		{
			String s = new String("Command "+command.getClass().getName()+
				":Setting Fits Headers failed:");
			frodospec.error(s,e);
			done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+304);
			done.setErrorString(s+e);
			done.setSuccessful(false);
			return false;
		}
		return true;
	}

	/**
	 * This routine tries to get a set of FITS headers for an exposure, by issuing a GET_FITS command
	 * to the ISS. The results from this command are put into the FrodoSpec's FITS header object.
	 * If an error occurs the done objects field's can be set to record the error.
	 * The order numbers returned from the ISS are incremented by the order number offset
	 * defined in the FrodoSpec 'frodospec.get_fits.order_number_offset' property.
	 * The objectName field is updated with the value of the "OBJECT" FITS header. This is used for
	 * changing the "OBJECT" keyword for ARCs etc...
	 * @param command The command being implemented that made this call to the ISS. This is used
	 * 	for error logging.
	 * @param done A COMMAND_DONE subclass specific to the command being implemented. If an
	 * 	error occurs the relevant fields are filled in with the error.
	 * @param arm Which arm. Either FrodoSpecConfig.RED_ARM or FrodoSpecConfig.BLUE_ARM.
	 * @return The routine returns a boolean to indicate whether the operation was completed
	 *  	successfully.
	 * @exception IllegalArgumentException Thrown if the arm is out of range.
	 * @see FrodoSpec#sendISSCommand
	 * @see FrodoSpec#getStatus
	 * @see FrodoSpecStatus#getPropertyInteger
	 * @see #frodospecFitsHeaderList
	 * @see #DEFAULT_ORDER_NUMBER_OFFSET
	 * @see #frodospec
	 * @see #objectName
	 */
	public boolean getFitsHeadersFromISS(COMMAND command,COMMAND_DONE done,int arm)  throws IllegalArgumentException
	{
		INST_TO_ISS_DONE instToISSDone = null;
		GET_FITS_DONE getFitsDone = null;
		FitsHeaderCardImage objectCardImage = null;
		int orderNumberOffset;

		if((arm != FrodoSpecConfig.RED_ARM)&&(arm != FrodoSpecConfig.BLUE_ARM))
		{
			throw new IllegalArgumentException(this.getClass().getName()+":getFitsHeadersFromISS:Arm "+arm+
							   " out of range.");
		}
		instToISSDone = frodospec.sendISSCommand(new GET_FITS(command.getId()),serverConnectionThread);
		if(instToISSDone.getSuccessful() == false)
		{
			frodospec.error(this.getClass().getName()+":getFitsHeadersFromISS:"+
				command.getClass().getName()+":"+instToISSDone.getErrorString());
			done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+305);
			done.setErrorString(instToISSDone.getErrorString());
			done.setSuccessful(false);
			return false;
		}
	// Get the returned FITS header information into the FitsHeader object.
		getFitsDone = (GET_FITS_DONE)instToISSDone;
	// get the order number offset
		try
		{
			orderNumberOffset = status.getPropertyInteger("frodospec.get_fits.order_number_offset");
		}
		catch(NumberFormatException e)
		{
			orderNumberOffset = DEFAULT_ORDER_NUMBER_OFFSET;
			frodospec.error(this.getClass().getName()+
				":getFitsHeadersFromISS:Getting order number offset failed.",e);
		}
		frodospecFitsHeaderList[arm].addKeywordValueList(getFitsDone.getFitsHeader(),orderNumberOffset);
		// retrieve the OBJECT FITS header card image
		// we will need to modify this in odd ways for ARC obs/calBefore/calAfter
		objectCardImage = frodospecFitsHeaderList[arm].get("OBJECT");
		objectName = (String)(objectCardImage.getValue());
		return true;
	}

	/**
	 * This routine uses the Fits Header object, stored in the frodospec object, to save the headers to disc.
	 * This method also updates the RUNNUM and EXPNUM keywords with the current multRun and runNumber values
	 * in the frodospecFilenameList object, as they must be correct when the file is saved.
	 * @param command The command being implemented that made this call to the ISS. This is used
	 * 	for error logging.
	 * @param done A COMMAND_DONE subclass specific to the command being implemented. If an
	 * 	error occurs the relevant fields are filled in with the error.
	 * @param arm Which arm to retrieve data for.
	 * @param filename The filename to save the headers to.
	 * @return The routine returns a boolean to indicate whether the operation was completed
	 *  	successfully.
	 * @exception IllegalArgumentException Thrown if the arm is out of range.
	 * @see #frodospecFitsHeaderList
	 * @see #frodospecFilenameList
	 * @see ngat.fits.FitsFilename#getMultRunNumber
	 * @see ngat.fits.FitsFilename#getRunNumber
	 */
	public boolean saveFitsHeaders(COMMAND command,COMMAND_DONE done,int arm,String filename) throws IllegalArgumentException
	{
		if((arm != FrodoSpecConfig.RED_ARM)&&(arm != FrodoSpecConfig.BLUE_ARM))
		{
			throw new IllegalArgumentException(this.getClass().getName()+":saveFitsHeaders:Arm "+arm+
							   " out of range.");
		}
		try
		{
			frodospecFitsHeaderList[arm].add("RUNNUM",
							 new Integer(frodospecFilenameList[arm].getMultRunNumber()),
				frodospecFitsHeaderDefaultsList[arm].getComment("RUNNUM"),
				frodospecFitsHeaderDefaultsList[arm].getUnits("RUNNUM"),
				frodospecFitsHeaderDefaultsList[arm].getOrderNumber("RUNNUM"));
			frodospecFitsHeaderList[arm].add("EXPNUM",
							 new Integer(frodospecFilenameList[arm].getRunNumber()),
				frodospecFitsHeaderDefaultsList[arm].getComment("EXPNUM"),
				frodospecFitsHeaderDefaultsList[arm].getUnits("EXPNUM"),
				frodospecFitsHeaderDefaultsList[arm].getOrderNumber("EXPNUM"));
		}
		// FitsHeaderException thrown by frodospecFitsHeaderDefaults.getValue
		// IllegalAccessException thrown by frodospecFitsHeaderDefaults.getValue
		// InvocationTargetException thrown by frodospecFitsHeaderDefaults.getValue
		// NoSuchMethodException thrown by frodospecFitsHeaderDefaults.getValue
		// InstantiationException thrown by frodospecFitsHeaderDefaults.getValue
		// ClassNotFoundException thrown by frodospecFitsHeaderDefaults.getValue
		catch(Exception e)
		{
			String s = new String("Command "+command.getClass().getName()+
				":Setting Fits Headers in saveFitsHeaders failed:"+e);
			frodospec.error(s,e);
			done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+306);
			done.setErrorString(s);
			done.setSuccessful(false);
			return false;
		}
		try
		{
			frodospecFitsHeaderList[arm].writeFitsHeader(filename);
		}
		catch(FitsHeaderException e)
		{
			String s = new String("Command "+command.getClass().getName()+
					":Saving Fits Headers failed for file:"+filename+":"+e);
			frodospec.error(s,e);
			done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+307);
			done.setErrorString(s);
			done.setSuccessful(false);
			return false;
		}
		return true;
	}

	/**
	 * This routine uses the Fits Header object, stored in the frodospec object, to save the headers to disc.
	 * This method also updates the RUNNUM and EXPNUM keywords with the current multRun and runNumber values
	 * in the ccsFilename object, as they must be correct when the file is saved. 
	 * A list of windows are defined from the setup's window flags, and a set of headers
	 * are saved to a window specific filename for each window defined.
	 * It changess the CCDWXOFF, CCDWYOFF, CCDWXSIZ and CCDWYSIZ keywords for each window defined. 
	 * @param command The command being implemented that made this call to the ISS. This is used
	 * 	for error logging.
	 * @param done A COMMAND_DONE subclass specific to the command being implemented. If an
	 * 	error occurs the relevant fields are filled in with the error.
	 * @param arm Which arm to retrieve data for.
	 * @param filenameList An instance of a list. The filename's saved containing FITS headers are added
	 *        to the list.
	 * @return The routine returns a boolean to indicate whether the operation was completed
	 *  	successfully.
	 * @exception IllegalArgumentException Thrown if the arm is out of range.
	 * @see #frodospecFitsHeaderList
	 * @see #frodospecFilenameList
	 * @see ngat.fits.FitsFilename#getMultRunNumber
	 * @see ngat.fits.FitsFilename#getRunNumber
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	public boolean saveFitsHeaders(COMMAND command,COMMAND_DONE done,int arm,List filenameList) throws IllegalArgumentException
	{
		FitsHeaderCardImage cardImage = null;
		CCDLibrarySetupWindow window = null;
		CCDLibrary ccd = null;
		List windowIndexList = null;
		String filename = null;
		int windowIndex,windowFlags,ncols,nrows,xbin,ybin;

		if((arm != FrodoSpecConfig.RED_ARM)&&(arm != FrodoSpecConfig.BLUE_ARM))
		{
			throw new IllegalArgumentException(this.getClass().getName()+":saveFitsHeaders:Arm "+arm+
							   " out of range.");
		}
		if(arm == FrodoSpecConfig.RED_ARM)
			ccd = redCCD;
		else if(arm == FrodoSpecConfig.BLUE_ARM)
			ccd = blueCCD;
		windowFlags = ccd.setupGetWindowFlags();
		windowIndexList = new Vector();
		if(windowFlags > 0)
		{
			// if the relevant bit is set, add an Integer with the appopriate
			// window index to the windowIndexList. The window index is one less than
			// the window number.
			if((windowFlags&CCDLibrary.SETUP_WINDOW_ONE) > 0)
			{
				windowIndexList.add(new Integer(0));
			}
			if((windowFlags&CCDLibrary.SETUP_WINDOW_TWO) > 0)
			{
				windowIndexList.add(new Integer(1));
			}
			if((windowFlags&CCDLibrary.SETUP_WINDOW_THREE) > 0)
			{
				windowIndexList.add(new Integer(2));
			}
			if((windowFlags&CCDLibrary.SETUP_WINDOW_FOUR) > 0)
			{
				windowIndexList.add(new Integer(3));
			}
		}// end if windowFlags > 0
		else
		{
			windowIndexList.add(new Integer(-1));
		}
		for(int i = 0; i < windowIndexList.size(); i++)
		{
			try
			{
				windowIndex = ((Integer)windowIndexList.get(i)).intValue();
				if(windowIndex > -1)
				{
					window = ccd.setupGetWindow(windowIndex);
					// window number is 1 more than index
					frodospecFilenameList[arm].setWindowNumber(windowIndex+1);
					ncols = ccd.getWindowWidth(windowIndex);
					nrows = ccd.getWindowHeight(windowIndex);
					// only change PRESCAN and POSTSCAN if windowed
				        // PRESCAN
					cardImage = frodospecFitsHeaderList[arm].get("PRESCAN");
					cardImage.setValue(new Integer(0));
				        // POSTSCAN
					cardImage = frodospecFitsHeaderList[arm].get("POSTSCAN");
				        //diddly see ccd_setup.c : SETUP_WINDOW_BIAS_WIDTH
					cardImage.setValue(new Integer(53));
				}
				else
				{
					// get current binning for later
					xbin = ccd.getXBin();
					ybin = ccd.getYBin();
					frodospec.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_MISC,
						      this.getClass().getName()+
						":saveFitsHeaders:Default window X size = "+
						frodospecFitsHeaderDefaultsList[arm].getValueInteger("CCDWXSIZ")+" / "+xbin+" = "+
						(frodospecFitsHeaderDefaultsList[arm].getValueInteger("CCDWXSIZ")/xbin)+".");
					frodospec.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_MISC,this.getClass().getName()+
						":saveFitsHeaders:Default window Y size = "+
						frodospecFitsHeaderDefaultsList[arm].getValueInteger("CCDWYSIZ")+" / "+ybin+" = "+
						(frodospecFitsHeaderDefaultsList[arm].getValueInteger("CCDWYSIZ")/ybin)+".");
					window = new CCDLibrarySetupWindow(0,0,
							  frodospecFitsHeaderDefaultsList[arm].getValueInteger("CCDWXSIZ")/xbin,
							  frodospecFitsHeaderDefaultsList[arm].getValueInteger("CCDWYSIZ")/ybin);
					frodospec.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_MISC,this.getClass().getName()+
						":saveFitsHeaders:Using default window : "+window+".");
					frodospecFilenameList[arm].setWindowNumber(1);
					ncols = ccd.getNCols();
					nrows = ccd.getNRows();
				}
				filename = frodospecFilenameList[arm].getFilename();
				// NAXIS1
				cardImage = frodospecFitsHeaderList[arm].get("NAXIS1");
				cardImage.setValue(new Integer(ncols));
				// NAXIS2
				cardImage = frodospecFitsHeaderList[arm].get("NAXIS2");
				cardImage.setValue(new Integer(nrows));
				// RUNNUM/EXPNUM
				frodospecFitsHeaderList[arm].add("RUNNUM",
							new Integer(frodospecFilenameList[arm].getMultRunNumber()),
						  frodospecFitsHeaderDefaultsList[arm].getComment("RUNNUM"),
						  frodospecFitsHeaderDefaultsList[arm].getUnits("RUNNUM"),
						  frodospecFitsHeaderDefaultsList[arm].getOrderNumber("RUNNUM"));
				frodospecFitsHeaderList[arm].add("EXPNUM",
							new Integer(frodospecFilenameList[arm].getRunNumber()),
						  frodospecFitsHeaderDefaultsList[arm].getComment("EXPNUM"),
						  frodospecFitsHeaderDefaultsList[arm].getUnits("EXPNUM"),
						  frodospecFitsHeaderDefaultsList[arm].getOrderNumber("EXPNUM"));
				// CCDWXOFF
				cardImage = frodospecFitsHeaderList[arm].get("CCDWXOFF");
				cardImage.setValue(new Integer(window.getXStart()));
				// CCDWYOFF
				cardImage = frodospecFitsHeaderList[arm].get("CCDWYOFF");
				cardImage.setValue(new Integer(window.getYStart()));
				// CCDWXSIZ
				cardImage = frodospecFitsHeaderList[arm].get("CCDWXSIZ");
				cardImage.setValue(new Integer(window.getXEnd()-window.getXStart()));
				// CCDWYSIZ
				cardImage = frodospecFitsHeaderList[arm].get("CCDWYSIZ");
				cardImage.setValue(new Integer(window.getYEnd()-window.getYStart()));
			}//end try
			// CCDLibraryNativeException thrown by CCDSetupGetWindow
			// FitsHeaderException thrown by ccsFitsHeaderDefaults.getValue
			// IllegalAccessException thrown by ccsFitsHeaderDefaults.getValue
			// InvocationTargetException thrown by ccsFitsHeaderDefaults.getValue
			// NoSuchMethodException thrown by ccsFitsHeaderDefaults.getValue
			// InstantiationException thrown by ccsFitsHeaderDefaults.getValue
			// ClassNotFoundException thrown by ccsFitsHeaderDefaults.getValue
			catch(Exception e)
			{
				String s = new String("Command "+command.getClass().getName()+
						      ":Setting Fits Headers in saveFitsHeaders failed:"+e);
				frodospec.error(s,e);
				done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+311);
				done.setErrorString(s);
				done.setSuccessful(false);
				return false;
			}
			try
			{
				frodospecFitsHeaderList[arm].writeFitsHeader(filename);
				filenameList.add(filename);
			}
			catch(FitsHeaderException e)
			{
				String s = new String("Command "+command.getClass().getName()+
						      ":Saving Fits Headers failed for file:"+filename+":"+e);
				frodospec.error(s,e);
				done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+312);
				done.setErrorString(s);
				done.setSuccessful(false);
				return false;
			}
		}// end for
		return true;
	}

	/**
	 * This method is called by command implementors that assume the CCD camera was configured to
	 * operate in non-windowed mode. The method checks CCDLibrary's setup window flags are zero.
	 * If they are non-zero, the done object is filled in with a suitable error message.
	 * @param arm Which arm to retrieve data for.
	 * @param done An instance of command done, the error string/number are set if the window flags are non-zero.
	 * @return The method returns true if the CCD is setup to be non-windowed, false if it is setup to be windowed.
	 * @see #redCCD
	 * @see #blueCCD
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 * @see ngat.frodospec.ccd.CCDLibrary#setupGetWindowFlags
	 */
	public boolean checkNonWindowedSetup(int arm,COMMAND_DONE done)
	{
		CCDLibrary ccd = null;

		if(arm == FrodoSpecConfig.RED_ARM)
			ccd = redCCD;
		else if(arm == FrodoSpecConfig.BLUE_ARM)
			ccd = blueCCD;
		if(ccd.setupGetWindowFlags() > 0)
		{
			String s = new String(":Configured for Windowed Readout:Expecting non-windowed.");
			frodospec.error(s);
			done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+310);
			done.setErrorString(s);
			done.setSuccessful(false);
			return false;
		}
		return true;
	}

	/**
	 * Method to get an integer representing a SDSU output Amplifier,that can be passed into the setupDimensions
	 * method of ngat.ccd.CCDLibrary. 
	 * This implementation should agree with the eqivalent getDeInterlaceSetting method.
	 * @return An integer, representing a valid value to pass into CCDSetupDimensions to set the specified
	 *         amplifier.
	 * @exception NullPointerException Thrown if the property name, or it's value, are null.
	 * @exception CCDLibraryFormatException Thrown if the property's value, which is passed into
	 *            dspAmplifierFromString, does not contain a valid amplifier.
	 * @see #getAmplifier(java.lang.String)
	 */
	public int getAmplifier() throws NullPointerException,CCDLibraryFormatException
	{
		int amplifier;

		amplifier = getAmplifier("frodospec.config.amplifier");
		return amplifier;
	}

	/**
	 * Method to get an integer represeting a SDSU output Amplifier, that can be passed into the setupDimensions
	 * method of ngat.ccd.CCDLibrary. The amplifier to use is retrieved from the specified property.
	 * @param propertyName A string, of the property keyword, the value of which is used to specify the
	 *        amplifier.
	 * @return An integer, representing a valid value to pass into setupDimensions to set the specified
	 *         amplifier.
	 * @exception NullPointerException Thrown if the property name, or it's value, are null.
	 * @exception CCDLibraryFormatException Thrown if the property's value, which is passed into
	 *            CCDDSPAmplifierFromString, does not contain a valid amplifier.
	 * @see #status
	 * @see #blueCCD
	 * @see FrodoSpecStatus#getProperty
	 * @see ngat.frodospec.ccd.CCDLibrary#dspAmplifierFromString
	 */
	public int getAmplifier(String propertyName) throws NullPointerException,CCDLibraryFormatException
	{
		String propertyValue = null;

		if(propertyName == null)
		{
			throw new NullPointerException(this.getClass().getName()+
						       ":getAmplifier:Property Name was null.");
		}
		propertyValue = status.getProperty(propertyName);
		if(propertyValue == null)
		{
			throw new NullPointerException(this.getClass().getName()+
						       ":getAmplifier:Property Value of keyword "+propertyName+
						       " was null.");
		}
		// NB does not matter which arm we call this for.
		return blueCCD.dspAmplifierFromString(propertyValue);
	}

	/**
	 * Method to get an integer representing a SDSU output De-Interlace setting,
	 * that can be passed into the setupDimensions method of ngat.ccd.CCDLibrary. 
	 * The chosen property name is passed to getDeInterlaceSetting to get the
	 * equivalent de-interlace setting.
	 * This implementation should agree with the eqivalent getAmplifier method.
	 * @return An integer, representing a valid value to pass into setupDimensions to set the specified
	 *         de-interlace setting.
	 * @exception NullPointerException Thrown if getDeInterlaceSetting fails.
	 * @exception CCDLibraryFormatException Thrown if getDeInterlaceSetting fails.
	 * @see #getDeInterlaceSetting(java.lang.String)
	 */
	public int getDeInterlaceSetting() throws NullPointerException,CCDLibraryFormatException
	{
		int deInterlaceSetting;

		deInterlaceSetting = getDeInterlaceSetting("frodospec.config.amplifier");
		return deInterlaceSetting;
	}

	/**
	 * Method to get an integer represeting a SDSU de-interlace setting,
	 * that can be passed into the setupDimensions method of ngat.ccd.CCDLibrary. 
	 * The amplifier to use is retrieved from the specified property, and the de-interlace setting determined 
	 * from this.
	 * @param amplifierPropertyName A string, of the property keyword, the value of which is used to specify the
	 *        amplifier.
	 * @return An integer, representing a valid value to pass into setupDimensions to set the specified
	 *         de-interlace setting.
	 * @exception NullPointerException Thrown if the property name, or it's value, are null.
	 * @exception IllegalArgumentException Thrown if the amplifier was not recognised by this method.
	 * @exception CCDLibraryFormatException Thrown if the derived de-interlace string, which is passed into
	 *            CCDDSPDeinterlaceFromString, does not contain a valid de-interlace setting.
	 * @see #blueCCD
	 * @see #getAmplifier
	 * @see ngat.frodospec.ccd.CCDLibrary#dspDeinterlaceFromString
	 */
	public int getDeInterlaceSetting(String amplifierPropertyName) throws NullPointerException,
	                                 IllegalArgumentException,CCDLibraryFormatException
	{
		String deInterlaceString = null;
		int amplifier,deInterlaceSetting;

		amplifier = getAmplifier(amplifierPropertyName);
		// convert Amplifier to De-Interlace Setting string
		switch(amplifier)
		{
			case CCDLibrary.DSP_AMPLIFIER_LEFT:
				deInterlaceString = "DSP_DEINTERLACE_SINGLE";
				break;
			case CCDLibrary.DSP_AMPLIFIER_RIGHT:
				deInterlaceString = "DSP_DEINTERLACE_FLIP";
				break;
			case CCDLibrary.DSP_AMPLIFIER_BOTH:
				deInterlaceString = "DSP_DEINTERLACE_SPLIT_SERIAL";
				break;
			default:
				throw new IllegalArgumentException(this.getClass().getName()+
						       ":getDeInterlaceSetting:Amplifier String of keyword "+
						       amplifierPropertyName+" was illegal value "+amplifier+".");
		}
		// convert de-interlace string into value to pass to libccd.
		// Does not matter which instance of CCDLibrary we use for the conversion
		deInterlaceSetting = blueCCD.dspDeinterlaceFromString(deInterlaceString);
		return deInterlaceSetting;
	}

	/**
	 * This method retrieves the current Amplifier configuration used to configure the CCD controller.
	 * This determines which readout(s) the CCD uses. The numeric setting is then converted into a 
	 * valid string as specified by the LT FITS standard.
	 * @return A String is returned, either 'LEFT', 'RIGHT', or 'DUAL'. If the amplifier cannot be
	 * 	determined an exception is thrown.
	 * @exception IllegalArgumentException Thrown if the amplifier string cannot be determined.
	 * @see ngat.frodospec.ccd.CCDLibrary#getAmplifier
	 * @see ngat.frodospec.ccd.CCDLibrary#DSP_AMPLIFIER_LEFT
	 * @see ngat.frodospec.ccd.CCDLibrary#DSP_AMPLIFIER_RIGHT
	 * @see ngat.frodospec.ccd.CCDLibrary#DSP_AMPLIFIER_BOTH
	 * @see #blueCCD
	 * @see #redCCD
	 */
	private String getCCDRDOUTValue() throws IllegalArgumentException
	{
		String amplifierString = null;
		int amplifier;

		//get amplifier from libccd cached setting.
		amplifier = blueCCD.getAmplifier();
		// diddly method does not exist
		// diddly arm parameter
		switch(amplifier)
		{
			case CCDLibrary.DSP_AMPLIFIER_LEFT:
				amplifierString = "LEFT";
				break;
			case CCDLibrary.DSP_AMPLIFIER_RIGHT:
				amplifierString = "RIGHT";
				break;
			case CCDLibrary.DSP_AMPLIFIER_BOTH:
				amplifierString = "DUAL";
				break;
			default:
				throw new IllegalArgumentException("getCCDRDOUTValue:amplifier:"+amplifier+
									" not known.");
		}
		return amplifierString;
	}

	/**
	 * Get the configured exposure length for the specified lamp from the configuration file.
	 * @param lampString A space separated list of lamps to use.
	 * @param arm Which arm we are using.
	 * @param resolution Which resolution grating we are using.
	 * @return The exposure length in milliseconds.
	 * @exception ScsFilterNativeException Thrown by getPositionWavelength.
	 * @see #status
	 * @see FrodoSpecStatus#getPropertyInteger
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 * @see ngat.phase2.FrodoSpecConfig#RESOLUTION_HIGH
	 * @see ngat.phase2.FrodoSpecConfig#RESOLUTION_LOW
	 * @see FrodoSpecConstants#ARM_STRING_LIST
	 * @see FrodoSpecConstants#RESOLUTION_STRING_LIST
	 */
	protected int getArcExposureLength(String lampString,int arm,int resolution) throws IllegalArgumentException
	{
		int exposureLength;
		String keywordString = null;

		if((arm != FrodoSpecConfig.RED_ARM)&&(arm != FrodoSpecConfig.BLUE_ARM))
		{
			throw new IllegalArgumentException(this.getClass().getName()+
							   ":getArcExposureLength:Illegal arm:"+arm);
		}
		if((resolution != FrodoSpecConfig.RESOLUTION_HIGH)&&(resolution != FrodoSpecConfig.RESOLUTION_LOW))
		{
			throw new IllegalArgumentException(this.getClass().getName()+
							   ":getArcExposureLength:Illegal resolution:"+resolution);
		}

		keywordString = new String("frodospec.arc."+FrodoSpecConstants.ARM_STRING_LIST[arm]+"."+
					   FrodoSpecConstants.RESOLUTION_STRING_LIST[resolution]+"."+lampString+
					   ".exposure_length");
		exposureLength = status.getPropertyInteger(keywordString);
		return exposureLength;
	}

	/**
	 * Method to turn off A&G lamps using the LampController (and therefore lampUnit).
	 * @param arm Which arm is the command running on.
	 * @param command The command that requires the lamp turning off.
	 * @param done  The COMMAND_DONE that requires the lamp turning off. On failure, the error
	 *            is filled in as appropriate.
	 * @return The method returns true on success and false on failure. If false, the error is set in the
	 *         done object.
	 * @see #frodospec
	 * @see ngat.frodospec.FrodoSpec#getLampController
	 * @see ngat.frodospec.LampController#clearLampLock
	 */
	public boolean turnLampsOff(int arm,COMMAND command,COMMAND_DONE done)
	{
		try
		{
			frodospec.getLampController().clearLampLock(arm);
		}
		catch(Exception e)
		{
			frodospec.error(this.getClass().getName()+":turnLampsOff:"+command+
					":Switching off lamps failed:",e);
			done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+314);
			done.setErrorString("Switching off lamps failed:"+e.toString());
			done.setSuccessful(false);
			return false;
		}
		return true;
	}
}

//
// $Log: not supported by cvs2svn $
// Revision 1.2  2008/11/24 14:57:23  cjm
// Added objectName field and associated code to extraxt from returned ISS headers.
// The OBJECT FITS header is then modified before saving for ARCs and Darks.
//
// Revision 1.1  2008/11/20 11:33:35  cjm
// Initial revision
//
//

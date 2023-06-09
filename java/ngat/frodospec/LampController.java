// LampController.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/LampController.java,v 1.10 2011-01-12 11:50:03 cjm Exp $
package ngat.frodospec;

import java.lang.*;
import java.io.*;

import ngat.lamp.*;
import ngat.message.base.ACK;
import ngat.net.*;
import ngat.util.logging.*;

/**
 * We need the ability to coordinate lamp operations between the two arms of FrodoSpec.
 * <ul>
 * <li>One or more arms may be doing MULTRUN exposures, which <b>require</b> all lamps to be off
 *     (so only starlight and not lamp light (from the A&G box) is travelling up the fibres.
 * <li>We can only be doing an ARC, or calibration before/after ARC (as part of a MULTRUN), if we are not doing
 *     a MULTRUN.
 * <li>Both arms can be doing an ARC, but <b>only</b> if the same lampset is in use.
 * </ul>
 * This class attempts to coordinate this activity.
 * @author Chris Mottram
 * @version $Revision: 1.10 $
 */
public class LampController
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: LampController.java,v 1.10 2011-01-12 11:50:03 cjm Exp $");
	/**
	 * Constant used when we require no lamp to be used.
	 */
	protected final static String NO_LAMP = "NO_LAMP";
	/**
	 * Default length of time to wait on inUseLock, in milliseconds.
	 * @see #inUseLock
	 */
	protected final static int DEFAULT_WAIT_LENGTH = 10000;
	/**
	 * The logger to use.
	 */
	protected Logger logger = null;	
	/**
	 * An instance of LTAGLampUnit to control/interface with the LT A&G Box calibration lamps.
	 * @see ngat.lamp.LTAGLampUnit
	 */
	private LTAGLampUnit lampUnit = null;
	/**
	 * The lock object to synchronise on before modifying inUseLamps / inUseCount.
	 * @see #inUseLamps
	 * @see #inUseCount
	 */
	private Object inUseLock = null;
	/**
	 * Which lamps are in use.
	 */
	private String inUseLamps = null;
	/**
	 * How many threads are using the lamps.
	 */
	private int inUseCount = 0;
	/**
	 * Length of time to wait between waits on the inUseLock. In milliseconds.
	 * @see #inUseLock
	 * @see #DEFAULT_WAIT_LENGTH
	 */
	private int waitLength = DEFAULT_WAIT_LENGTH;

	/**
	 * Constructor. Creates inUseLock instance. Gets the logger instance from the LogManager.
	 * @see #inUseLock
	 * @see #logger
	 */
	public LampController()
	{
		super();
		inUseLock = new Object();
		// set logger
		logger = LogManager.getLogger("log");
	}

	/**
	 * Set the LT A&G box lamp unit we are controlling with this class.
	 * @param o The lamp unit.
	 * @see #lampUnit
	 */
	public void setLampUnit(LTAGLampUnit o)
	{
		lampUnit = o;
	}

	/**
	 * Set the lamp lock to NO_LAMP (i.e. we want to do a MULTRUN). And actually turn all the lamps off.
	 * Stow the calibration mirror.
	 * @param clazz The class string used for generating log records from this operation.
	 * @param source The source string used for generating log records from this operation.
	 * @param arm The arm to set the lock for.
	 * @param serverConnectionThread The servers connection thread. Used to send ACKs whilst waiting for the
	 *                               light (or lack thereof) to become available.
	 * @exception IOException Thrown if sending the ACK back over the server connection thread fails.
	 * @exception Exception Thrown if a problem with the lamp unit occurs.
	 * @see #inUseLock
	 * @see #inUseCount
	 * @see #inUseLamps
	 * @see #lampUnit
	 * @see #waitLength
	 * @see ngat.lamp.LTAGLampUnit#turnAllLampsOff
	 * @see ngat.lamp.LTAGLampUnit#stowMirror
	 */
	public void setNoLampLock(String clazz,String source,int arm,
				  FrodoSpecTCPServerConnectionThread serverConnectionThread) 
		throws IOException, Exception
	{
		ACK ack = null;

		logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,this.getClass().getName()+
			   ":setNoLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+") started.");
		// acquire lock
		synchronized(inUseLock)
		{
			logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,this.getClass().getName()+
				   ":setNoLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+"): Entered lock.");
			// if no lamp locked out, acquire NO_LAMP
			if(inUseCount == 0)
			{
				if(lampUnit != null)
				{
					// actually turn all lamps off (this even works if lock was on NO_LAMP!)
					logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,
						   this.getClass().getName()+
						   ":setNoLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+
						   ") turning off all lamps.");
					lampUnit.turnAllLampsOff(clazz,source);
					// actually stow mirror
					logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,
						   this.getClass().getName()+
						   ":setNoLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+
						   ") stowing calibration mirror.");
					lampUnit.stowMirror(clazz,source);
				}
				// update inUse Data
				// Do this after turning off lamps/stowing mirror, so if those operations fail
				// the lamp controller does not get stuck in a locked state
				inUseLamps = NO_LAMP;
				inUseCount++;
				logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,this.getClass().getName()+
					   ":setNoLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+
					   "): inUseCount was zero: Acquired NO_LAMP lock.");
			}
			else
			{
				// if current lamp in use is NO_LAMP, just inc in use count
				if(inUseLamps.equals(NO_LAMP))
				{
					if(lampUnit != null)
					{
						// actually turn all lamps off(this even works if lock was on NO_LAMP!)
						// if inUseLamps / inUseCount is > 0 these calls arn't
						// strictly necessary, the lamps should already be off/ mirror stowed
						logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,
							   this.getClass().getName()+
							   ":setNoLampLock(arm="+
							   FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ") turning off all lamps.");
						lampUnit.turnAllLampsOff(clazz,source);
						// actually stow mirror
						logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,
							   this.getClass().getName()+
							   ":setNoLampLock(arm="+
							   FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ") stowing calibration mirror.");
						lampUnit.stowMirror(clazz,source);
					}
					// update inUse Data
					// Do this after turning off lamps/stowing mirror, so if those operations fail
					// the lamp controller does not get stuck in a locked state
					inUseCount++;
					logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,
						   this.getClass().getName()+
						  ":setNoLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+
						   "): inUseLamp was NO_LAMP: inUseCount now:"+inUseCount);
				}
				else
				{
					logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,
						   this.getClass().getName()+
						   ":setNoLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+
						   "): lamp lock in use ("+inUseLamps+","+inUseCount+
						   "):waiting to acquire lamp.");
					// wait until lamp in use is NO_LAMP or inUseCount is zero
					// use short circuit evaluation here, if lock is released whilst this
					// thread is in wait inUseLamps can become null, but then inUseCount
					// _must_ be zero
					while((inUseCount > 0) && (inUseLamps.equals(NO_LAMP) == false))
					{
						logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,
							   this.getClass().getName()+
							   ":setNoLampLock(arm="+
							   FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   "): Sending ACK:"+waitLength);
						// send ACK to command client to stop timeout
						ack = new ACK(this.getClass().getName()+":setNoLampLock(arm="+
							      FrodoSpecConstants.ARM_STRING_LIST[arm]+"):inUseLamps="+
							      inUseLamps+":inUseCount="+inUseCount);
						ack.setTimeToComplete((waitLength * 2));
						if(serverConnectionThread != null)
							serverConnectionThread.sendAcknowledge(ack);
						logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,
							   this.getClass().getName()+
							   ":setNoLampLock(arm="+
							   FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   "): Entering wait on lock:"+waitLength);
						// wait releases lock on inUseLock
						try
						{
							inUseLock.wait(waitLength);
						}
						catch(InterruptedException e)
						{
						}
						// lock is now back on for inUseLock
					}// end while
					// so NO_LAMP in use and inc count
					// actually turn all lamps off (this even works if lock was on NO_LAMP!)
					if(lampUnit != null)
					{
						logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,
							   this.getClass().getName()+
							   ":setNoLampLock(arm="+
							   FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ") turning off all lamps.");
						lampUnit.turnAllLampsOff(clazz,source);
						// actually stow mirror
						logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,
							   this.getClass().getName()+
							   ":setNoLampLock(arm="+
							   FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ") stowing calibration mirror.");
						lampUnit.stowMirror(clazz,source);
					}
					// update inUse Data
					// Do this after turning off lamps/stowing mirror, so if those operations fail
					// the lamp controller does not get stuck in a locked state
					inUseLamps = NO_LAMP;
					inUseCount++;
					logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,
						   this.getClass().getName()+
						  ":setNoLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+
						   "): acquired NO_LAMP: inUseCount now:"+inUseCount);
				}// endif wrong lamp is in use
			}// end if in use count > 0
		}// end synchronized on inUseLock
	}

	/**
	 * Set the lamp lock to lampsString (i.e. we want to do an ARC of some sort).
	 * Then turn all the lamps off, and then turn on the specified lamps. Then move the calibration mirror inline.
	 * @param clazz The class string used for generating log records from this operation.
	 * @param source The source string used for generating log records from this operation.
	 * @param arm The arm to set the lock for.
	 * @param lampsString Which set of lamps we want to use.
	 * @param serverConnectionThread The servers connection thread. Used to send ACKs whilst waiting for the
	 *                               light to become available.
	 * @exception IOException Thrown if sending the ACK back over the server connection thread fails.
	 * @exception Exception Thrown if a problem with the lamp unit occurs.
	 * @see #inUseLock
	 * @see #inUseCount
	 * @see #inUseLamps
	 * @see #lampUnit
	 * @see ngat.lamp.LTAGLampUnit#turnAllLampsOff
	 * @see ngat.lamp.LTAGLampUnit#turnLampsOn
	 * @see ngat.lamp.LTAGLampUnit#moveMirrorInline
	 */
	public void setLampLock(String clazz,String source,int arm,String lampsString,
				FrodoSpecTCPServerConnectionThread serverConnectionThread)
		throws IOException, Exception
	{
		ACK ack = null;

		logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,this.getClass().getName()+
		       ":setLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+",lamps="+lampsString+") started.");
		// acquire lock
		synchronized(inUseLock)
		{
			// if no lamp locked out, acquire lampsString
			if(inUseCount == 0)
			{
				if(lampUnit != null)
				{
					// actually turn lamp on
					logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,
						   this.getClass().getName()+
						   ":setLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+
						   ") turning lamps on:"+lampsString);
					// we should turn all lamps off before turning the ones we want back on
					// this resets all the lamp demand bits
					lampUnit.turnAllLampsOff(clazz,source);
					lampUnit.turnLampsOn(clazz,source,lampsString);
					logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,
						   this.getClass().getName()+
						   ":setLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+
						   ") moving mirror inline.");
					lampUnit.moveMirrorInline(clazz,source);
				}
				// update inUse Data
				// Do this after turning on lamps/moving mirror inline, so if those operations fail
				// the lamp controller does not get stuck in a locked state
				inUseLamps = lampsString;
				inUseCount++;
				logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,this.getClass().getName()+
					   ":setLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+
					   ",lamps="+lampsString+"): inUseCount was zero: Acquired lock for:"+
					   lampsString);
			}
			else
			{
				// if current lamp in use is lampsString, just inc in use count
				if(inUseLamps.equals(lampsString))
				{
					if(lampUnit != null)
					{
						// actually turn lamp on
						// if inUseLamps / inUseCount is > 0 these calls arn't
						// strictly necessary, the lamps should already be on/ mirror inline
						logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,
							   this.getClass().getName()+
							   ":setLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ") turning lamps on:"+lampsString);
						// we should turn all lamps off before turning the ones we want back on
						// this resets all the lamp demand bits
						lampUnit.turnAllLampsOff(clazz,source);
						lampUnit.turnLampsOn(clazz,source,lampsString);
						logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,
							   this.getClass().getName()+
							   ":setLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ") moving mirror inline.");
						lampUnit.moveMirrorInline(clazz,source);
					}
					// update inUse Data
					// Do this after turning on lamps/moving mirror inline, 
					// so if those operations fail
					// the lamp controller does not get stuck in a locked state
					inUseCount++;
					logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,
						   this.getClass().getName()+":setLampLock(arm="+
						   FrodoSpecConstants.ARM_STRING_LIST[arm]+",lamps="+lampsString+
						   "): inUseLamp was "+lampsString+": inUseCount now:"+inUseCount);
				}
				else
				{
					// wait until lamp in use is lampsString or inUseCount is zero
					// use short circuit evaluation here, if lock is released whilst this
					// thread is in wait inUseLamps can become null, but then inUseCount
					// _must_ be zero
					while((inUseCount > 0) && (inUseLamps.equals(lampsString) == false))
					{
						logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,
							   this.getClass().getName()+
							   ":setLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ",lamps="+lampsString+"): Sending ACK:"+waitLength);
						// send ACK to command client to stop timeout
						// don't save ack in server connection thread, so we can retrieve
						// the original ack time at the end of this loop.
						ack = new ACK(this.getClass().getName()+":setLampLock(arm="+
							      FrodoSpecConstants.ARM_STRING_LIST[arm]+",lamps="+
							      lampsString+"):inUseLamps="+
							      inUseLamps+":inUseCount="+inUseCount);
						ack.setTimeToComplete((waitLength * 2));
						if(serverConnectionThread != null)
							serverConnectionThread.sendAcknowledge(ack,false);
						logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,
							   this.getClass().getName()+
							   ":setLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ",lamps="+lampsString+"): Entering wait on lock:"+
							   waitLength);
						// wait releases lock on inUseLock
						try
						{
							inUseLock.wait(waitLength);
						}
						catch(InterruptedException e)
						{
						}
						// lock is now back on for inUseLock
					}// end while
					if(lampUnit != null)
					{
						// actually turn lamp on
						logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,
							   this.getClass().getName()+
							   ":setLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ") turning lamps on:"+lampsString);
						// we should turn all lamps off before turning the ones we want back on
						// this resets all the lamp demand bits
						lampUnit.turnAllLampsOff(clazz,source);
						lampUnit.turnLampsOn(lampsString);
						logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,
							   this.getClass().getName()+
							   ":setLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ") moving mirror inline.");
						lampUnit.moveMirrorInline(clazz,source);
					}
					// update inUse Data
					// Do this after turning on lamps/moving mirror inline, 
					// so if those operations fail
					// the lamp controller does not get stuck in a locked state
					inUseLamps = lampsString;
					inUseCount++;
					logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,
						   this.getClass().getName()+":setLampLock(arm="+
						   FrodoSpecConstants.ARM_STRING_LIST[arm]+",lamps="+lampsString+
						   "): acquired "+lampsString+
						   ": inUseCount now:"+inUseCount);
				}// endif wrong lamp is in use
			}// end if in use count > 0
		}// end synchronized on inUseLock
		// reset acknowledge time to last ack time set before wait ACKs
		if(serverConnectionThread != null)
		{
			ack = new ACK(this.getClass().getName()+":setLampLock(arm="+
				      FrodoSpecConstants.ARM_STRING_LIST[arm]+",lamps="+
				      lampsString+"):inUseLamps="+
				      inUseLamps+":inUseCount="+inUseCount);
			ack.setTimeToComplete(serverConnectionThread.getAcknowledgeTime());
			serverConnectionThread.sendAcknowledge(ack,true);
		}
	}

	/**
	 * Clear the previously specified lock. Then turn all the lamps off. Then stow the calibration mirror.
	 * @param clazz The class string used for generating log records from this operation.
	 * @param source The source string used for generating log records from this operation.
	 * @exception Exception Thrown if a problem with the lamp unit occurs.
	 * @see #inUseLock
	 * @see #inUseCount
	 * @see #inUseLamps
	 * @see #lampUnit
	 * @see ngat.lamp.LTAGLampUnit#turnAllLampsOff
	 * @see ngat.lamp.LTAGLampUnit#stowMirror
	 */
	public void clearLampLock(String clazz,String source,int arm) throws Exception
	{
		logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,this.getClass().getName()+
		       ":clearLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+") started.");
		// acquire lock
		synchronized(inUseLock)
		{
			inUseCount--;
			logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,this.getClass().getName()+
				   ":clearLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+
				   "): Acquired lamp lock:Decremented inUseCount ="+inUseCount);
			if(inUseCount == 0)
			{
				logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,this.getClass().getName()+
					   ":clearLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+
					   ") turning lamps off:"+inUseLamps);
				try
				{
					// actually turn all lamps off (this even works if lock was on NO_LAMP!)
					if(lampUnit != null)
					{
						lampUnit.turnAllLampsOff(clazz,source);
						// make sure calibration mirror is stowed
						lampUnit.stowMirror(clazz,source);
					}
				}
				// even if the stow mirror or turn all lamps off fails,
				// we want to release the inUseLamps and Lock, otherwise
				// the lamps get stuck in the locked position until the next reboot.
				finally
				{
					inUseLamps = null;
					inUseLock.notify();
				}
			}
		}
		logger.log(Logger.VERBOSITY_INTERMEDIATE,clazz,source,this.getClass().getName()+
		    ":clearLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+") finished.");
	}

	/**
	 * Method to get a string representing the controller status.
	 * @return A string of the form: "In Use Lamps:"+inUseLamps+" In Use Count:"+inUseCount.
	 * @see #inUseLock
	 * @see #inUseLamps
	 * @see #inUseCount
	 */
	public String getLampControllerStatus()
	{
		String s = null;

		synchronized(inUseLock)
		{
			s = new String("In Use Lamps:"+inUseLamps+" In Use Count:"+inUseCount);
		}
		return s;
	}

	/**
	 * Method to get the fault status from the lamp unit.
	 * Note this can be called from a thread other than the one controlling the lights, as the
	 * ngat.lamp.PLCConnection class in synchronised.
	 * @param clazz The class string used for generating log records from this operation.
	 * @param source The source string used for generating log records from this operation.
	 * @return The method returns true if the underlying lamp controller has a fault, false if it does not
	 *         have a fault.
	 * @exception Exception Thrown if there is a comms problem with the lamp unit.
	 * @see #lampUnit
	 * @see ngat.lamp.LTAGLampUnit#getFaultStatus
	 */
	public boolean getFaultStatus(String clazz,String source) throws Exception
	{
		return lampUnit.getFaultStatus(clazz,source);
	}
}

//
// $Log: not supported by cvs2svn $
// Revision 1.9  2010/12/13 11:18:45  cjm
// Modified clearLampLock to clear inUseLamps/inUseLock.notify even if the call to
// turnAllLampsOff or stowMirror fails. This stops the software getting stuck with a lock in place.
// Of course, the lamp may be left on/mirror be left in position incorrectly, but I think the
// stowMirror/turnAllLampsOff will be retried on subsequent commands.
//
// Revision 1.8  2010/04/07 15:11:53  cjm
// The ACKs now sent whilst waiting for a lamp to become available are not saved in the
// server connection thread.
// The original ACK length is then sent back to the client after the lamp has been successfully acquired.
//
// Revision 1.7  2010/03/15 16:46:10  cjm
// Now stows and move in line the calibration mirror.
// Added getFaultStatus method.
//
// Revision 1.6  2009/08/20 12:10:05  cjm
// Added extra arm logging.
// setNoLampLock now has while loop short circuit evaluation the right way around,
// hopefully stopping NullPointerExceptions.
// Added if null tests on serverConnectionThread and lampUnit, so lam controller
// can be tested from TestLampController without an actual lamp or connection thread running.
//
// Revision 1.5  2009/05/06 10:12:00  cjm
// Better documentation.
//
// Revision 1.4  2009/02/05 11:38:59  cjm
// Swapped Bitwise for Absolute logging levels.
//
// Revision 1.3  2008/11/24 15:51:04  cjm
// Fixed logging errors.
//
// Revision 1.2  2008/11/20 17:30:58  cjm
// Fixed setLampLock while loop error.
//
// Revision 1.1  2008/11/20 11:33:35  cjm
// Initial revision
//
//

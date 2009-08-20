// LampController.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/LampController.java,v 1.6 2009-08-20 12:10:05 cjm Exp $
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
 * @version $Revision: 1.6 $
 */
public class LampController
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: LampController.java,v 1.6 2009-08-20 12:10:05 cjm Exp $");
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
	 */
	public void setNoLampLock(int arm,FrodoSpecTCPServerConnectionThread serverConnectionThread) 
		throws IOException, Exception
	{
		ACK ack = null;

		logger.log(Logger.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":setNoLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+") started.");
		// acquire lock
		synchronized(inUseLock)
		{
			logger.log(Logger.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
				   ":setNoLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+"): Entered lock.");
			// if no lamp locked out, acquire NO_LAMP
			if(inUseCount == 0)
			{
				inUseLamps = NO_LAMP;
				inUseCount++;
				logger.log(Logger.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
					   ":setNoLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+
					   "): inUseCount was zero: Acquired NO_LAMP lock.");
			}
			else
			{
				// if current lamp in use is NO_LAMP, just inc in use count
				if(inUseLamps.equals(NO_LAMP))
				{
					inUseCount++;
					logger.log(Logger.VERBOSITY_INTERMEDIATE,
						   this.getClass().getName()+
						  ":setNoLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+
						   "): inUseLamp was NO_LAMP: inUseCount now:"+inUseCount);
				}
				else
				{
					logger.log(Logger.VERBOSITY_INTERMEDIATE,
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
						logger.log(Logger.VERBOSITY_INTERMEDIATE,
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
						logger.log(Logger.VERBOSITY_INTERMEDIATE,
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
					inUseLamps = NO_LAMP;
					inUseCount++;
					logger.log(Logger.VERBOSITY_INTERMEDIATE,
						   this.getClass().getName()+
						  ":setNoLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+
						   "): acquired NO_LAMP: inUseCount now:"+inUseCount);
				}// endif wrong lamp is in use
			}// end if in use count > 0
		}// end synchronized on inUseLock
		// actually turn all lamps off (this even works if lock was on NO_LAMP!)
		logger.log(Logger.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":setNoLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+") turning off all lamps.");
		if(lampUnit != null)
			lampUnit.turnAllLampsOff();
	}

	/**
	 * Set the lamp lock to lampsString (i.e. we want to do an ARC of some sort).
	 * Then turn all the lamps off, and then turn on the specified lamps.
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
	 */
	public void setLampLock(int arm,String lampsString,FrodoSpecTCPServerConnectionThread serverConnectionThread)
		throws IOException, Exception
	{
		ACK ack = null;

		logger.log(Logger.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
		       ":setLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+",lamps="+lampsString+") started.");
		// acquire lock
		synchronized(inUseLock)
		{
			// if no lamp locked out, acquire lampsString
			if(inUseCount == 0)
			{
				inUseLamps = lampsString;
				inUseCount++;
				logger.log(Logger.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
					   ":setLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+
					   ",lamps="+lampsString+"): inUseCount was zero: Acquired lock for:"+
					   lampsString);
			}
			else
			{
				// if current lamp in use is lampsString, just inc in use count
				if(inUseLamps.equals(lampsString))
				{
					inUseCount++;
					logger.log(Logger.VERBOSITY_INTERMEDIATE,
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
						logger.log(Logger.VERBOSITY_INTERMEDIATE,
							   this.getClass().getName()+
							   ":setLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+
							   ",lamps="+lampsString+"): Sending ACK:"+waitLength);
						// send ACK to command client to stop timeout
						ack = new ACK(this.getClass().getName()+":setLampLock(arm="+
							      FrodoSpecConstants.ARM_STRING_LIST[arm]+",lamps="+
							      lampsString+"):inUseLamps="+
							      inUseLamps+":inUseCount="+inUseCount);
						ack.setTimeToComplete((waitLength * 2));
						if(serverConnectionThread != null)
							serverConnectionThread.sendAcknowledge(ack);
						logger.log(Logger.VERBOSITY_INTERMEDIATE,
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
					// set lampsString in use and inc count
					inUseLamps = lampsString;
					inUseCount++;
					logger.log(Logger.VERBOSITY_INTERMEDIATE,
						   this.getClass().getName()+":setLampLock(arm="+
						   FrodoSpecConstants.ARM_STRING_LIST[arm]+",lamps="+lampsString+
						   "): acquired "+lampsString+
						   ": inUseCount now:"+inUseCount);
				}// endif wrong lamp is in use
			}// end if in use count > 0
		}// end synchronized on inUseLock
		// actually turn lamp on
		logger.log(Logger.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
		    ":setLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+") turning lamps on:"+lampsString);
		// we should turn all lamps off before turning the ones we want back on
		// this resets all the lamp demand bits
		if(lampUnit != null)
		{
			lampUnit.turnAllLampsOff();
			lampUnit.turnLampsOn(lampsString);
		}
	}

	/**
	 * Clear the previously specified lock. Then turn all the lamps off.
	 * @exception Exception Thrown if a problem with the lamp unit occurs.
	 * @see #inUseLock
	 * @see #inUseCount
	 * @see #inUseLamps
	 * @see #lampUnit
	 * @see ngat.lamp.LTAGLampUnit#turnAllLampsOff
	 */
	public void clearLampLock(int arm) throws Exception
	{
		logger.log(Logger.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
		       ":clearLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+") started.");
		// acquire lock
		synchronized(inUseLock)
		{
			inUseCount--;
			logger.log(Logger.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
				   ":clearLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+
				   "): Acquired lamp lock:Decremented inUseCount ="+inUseCount);
			if(inUseCount == 0)
			{
				logger.log(Logger.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
					   ":clearLampLock(arm="+FrodoSpecConstants.ARM_STRING_LIST[arm]+
					   ") turning lamps off:"+inUseLamps);
				// actually turn all lamps off (this even works if lock was on NO_LAMP!)
				if(lampUnit != null)
					lampUnit.turnAllLampsOff();
				inUseLamps = null;
				inUseLock.notify();
			}
		}
		logger.log(Logger.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
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
}

//
// $Log: not supported by cvs2svn $
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

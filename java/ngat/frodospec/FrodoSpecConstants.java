// FrodoSpecConstants.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/FrodoSpecConstants.java,v 1.1 2008-11-20 11:33:35 cjm Exp $
package ngat.frodospec;

import java.lang.*;
import java.io.*;

/**
 * This class holds some constant values for the FrodoSpec program. 
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class FrodoSpecConstants
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: FrodoSpecConstants.java,v 1.1 2008-11-20 11:33:35 cjm Exp $");

	/**
	 * Error code. No error.
	 */
	public final static int FRODOSPEC_ERROR_CODE_NO_ERROR 			= 0;
	/**
	 * The base Error number, for all FrodoSpec error codes. 
	 * See http://ltdevsrv.livjm.ac.uk/~dev/errorcodes.html for details.
	 */
	public final static int FRODOSPEC_ERROR_CODE_BASE 			= 800000;

	/**
	 * Logging level. Don't do any logging.
	 */
	public final static int FRODOSPEC_LOG_LEVEL_NONE 			= 0;
	/**
	 * Logging level. Log Commands messages received/sent.
	 * Note Java layer allocated bits 0..7 for logging.
	 */
	public final static int FRODOSPEC_LOG_LEVEL_COMMANDS 			= (1<<0);
	/**
	 * Logging level. Log Commands message replies received/sent.
	 * Note Java layer allocated bits 0..7 for logging.
	 */
	public final static int FRODOSPEC_LOG_LEVEL_REPLIES 			= (1<<1);
	/**
	 * Logging level. Extra TELFOCUS logging, on intermediate/accuracy (chi-squared) values
	 * for the quadratic fitting process.
	 * Note Java layer allocated bits 0..7 for logging.
	 */
	public final static int FRODOSPEC_LOG_LEVEL_TELFOCUS 			= (1<<2);
	/**
	 * Logging level. Extra DAY_CALIBRATE logging.
	 * Note Java layer allocated bits 0..7 for logging.
	 */
	public final static int FRODOSPEC_LOG_LEVEL_DAY_CALIBRATE 		= (1<<3);
	/**
	 * Logging level. Extra TWILIGHT_CALIBRATE logging.
	 * Note Java layer allocated bits 0..7 for logging.
	 */
	public final static int FRODOSPEC_LOG_LEVEL_TWILIGHT_CALIBRATE 		= (1<<4);
	/**
	 * Logging level. Extra MULTRUN logging.
	 * Note Java layer allocated bits 0..7 for logging.
	 */
	public final static int FRODOSPEC_LOG_LEVEL_MULTRUN 		        = (1<<5);
	/**
	 * Logging level. Extra RUNAT logging.
	 * Note Java layer allocated bits 0..7 for logging.
	 */
	public final static int FRODOSPEC_LOG_LEVEL_RUNAT 		        = (1<<6);
	/**
	 * Logging level. Misc logging.
	 * Note Java layer allocated bits 0..7 for logging.
	 */
	public final static int FRODOSPEC_LOG_LEVEL_MISC 		        = (1<<7);
	/**
	 * Logging level. PLC logging.
	 *  <b>Note Java layer allocated bits 0..7 for logging. BUT this is bit 8!</b>
	 */
	public final static int FRODOSPEC_LOG_LEVEL_PLC 		        = (1<<8);
	/**
	 * Logging level. Log if any logging is turned on.
	 * Note Java layer allocated bits 0..7 for logging.
	 * Note this only turns on all Java layer logging, not C library logging.
	 */
	public final static int FRODOSPEC_LOG_LEVEL_ALL 			= (FRODOSPEC_LOG_LEVEL_COMMANDS|
		FRODOSPEC_LOG_LEVEL_REPLIES|FRODOSPEC_LOG_LEVEL_TELFOCUS|FRODOSPEC_LOG_LEVEL_DAY_CALIBRATE|
		FRODOSPEC_LOG_LEVEL_TWILIGHT_CALIBRATE|FRODOSPEC_LOG_LEVEL_MULTRUN|FRODOSPEC_LOG_LEVEL_RUNAT|
		FRODOSPEC_LOG_LEVEL_MISC|FRODOSPEC_LOG_LEVEL_PLC);
	/**
	 * Logging level used by the error logger. We want to log all errors,
	 * hence this value should be used for all errors.
	 */
	public final static int FRODOSPEC_LOG_LEVEL_ERROR			= 1;

	/**
	 * Default thread priority level. This is for the server thread. Currently this has the highest priority,
	 * so that new connections are always immediately accepted.
	 * This number is the default for the <b>frodospec.thread.priority.server</b> property, if it does not exist.
	 */
	public final static int FRODOSPEC_DEFAULT_THREAD_PRIORITY_SERVER		= Thread.NORM_PRIORITY+2;
	/**
	 * Default thread priority level. 
	 * This is for server connection threads dealing with sub-classes of the INTERRUPT
	 * class. Currently these have a higher priority than other server connection threads,
	 * so that INTERRUPT commands are always responded to even when another command is being dealt with.
	 * This number is the default for the <b>frodospec.thread.priority.interrupt</b> property, if it does not exist.
	 */
	public final static int FRODOSPEC_DEFAULT_THREAD_PRIORITY_INTERRUPT	= Thread.NORM_PRIORITY+1;
	/**
	 * Default thread priority level. This is for most server connection threads. 
	 * Currently this has a normal priority.
	 * This number is the default for the <b>frodospec.thread.priority.normal</b> property, if it does not exist.
	 */
	public final static int FRODOSPEC_DEFAULT_THREAD_PRIORITY_NORMAL		= Thread.NORM_PRIORITY;
	/**
	 * Default thread priority level. This is for the Telescope Image Transfer server/client threads. 
	 * Currently this has the lowest priority, so that the camera control is not interrupted by image
	 * transfer requests.
	 * This number is the default for the <b>frodospec.thread.priority.tit</b> property, if it does not exist.
	 */
	public final static int FRODOSPEC_DEFAULT_THREAD_PRIORITY_TIT		= Thread.MIN_PRIORITY;
	/**
	 * The number of arms on FrodoSpec.
	 */
	public final static int FRODOSPEC_ARM_COUNT = 2;
	/**
	 * Mapping from arm numbers to strings. The first index is "none", so there are three elements in the list,
	 * as RED_ARM is configured as 1 and BLUE_ARM is configured as 2.
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	public final static String ARM_STRING_LIST[] = {"none","red","blue"};
	/**
	 * Mapping from resolution numbers to strings.The first index is "none", 
	 * so there are three elements in the list,
	 * as RESOLUTION_LOW is configured as 1 and RESOLUTION_HIGH is configured as 2.
	 * @see ngat.phase2.FrodoSpecConfig#RESOLUTION_HIGH
	 * @see ngat.phase2.FrodoSpecConfig#RESOLUTION_LOW
	 */
	public final static String RESOLUTION_STRING_LIST[] = {"none","low","high"};
}
//
// $Log: not supported by cvs2svn $
//

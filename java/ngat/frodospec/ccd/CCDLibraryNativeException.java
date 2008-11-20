// CCDLibraryNativeException.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/ccd/CCDLibraryNativeException.java,v 1.1 2008-11-20 11:34:28 cjm Exp $
package ngat.frodospec.ccd;

/**
 * This class extends Exception. Objects of this class are thrown when the underlying C code in CCDLibrary produces an
 * error. The JNI interface itself can also generate these exceptions.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class CCDLibraryNativeException extends Exception
{
	/**
	 * Revision Control System id string, showing the version of the Class
	 */
	public final static String RCSID = new String("$Id: CCDLibraryNativeException.java,v 1.1 2008-11-20 11:34:28 cjm Exp $");

	/**
	 * Constructor for the exception.
	 * @param errorString The error string.
	 */
	public CCDLibraryNativeException(String errorString)
	{
		super(errorString);
	}

	/**
	 * Constructor for the exception. Used from C JNI interface.
	 * @param errorString The error string.
	 * @param libfrodospec_ccd The libccd instance that caused this excecption.
	 */
	public CCDLibraryNativeException(String errorString,CCDLibrary libfrodospec_ccd)
	{
		super(errorString);
	}
}

//
// $Log: not supported by cvs2svn $
//

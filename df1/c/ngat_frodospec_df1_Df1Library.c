/* ngat_frodospec_df1_Df1Library.c
** implementation of Java Class ngat.frodospec.df1.Df1Library native interfaces
** $Header: /home/cjm/cvs/frodospec/df1/c/ngat_frodospec_df1_Df1Library.c,v 1.1 2023-03-21 14:34:10 cjm Exp $
*/
/**
 * ngat_frodospec_df1_Df1Library.c is the 'glue' between libfrodospec_df1, 
 * the C library used to control the PLC, and Df1Library.java, 
 * a Java Class to drive the controller. Df1Library specifically
 * contains all the native C routines corresponding to native methods in Java.
 * @author Chris Mottram LJMU
 * @version $Revision: 1.1 $
 */
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes
 * for time.
 */
#define _POSIX_SOURCE 1
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes
 * for time.
 */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jni.h>
#include <time.h>
#include "df1_general.h"
#include "df1_interface.h"
#include "df1_read_write.h"

/* hash definitions */

/**
 * Hash define for the size of the array holding Df1Library Instance (jobject) maps to Df1_Interface_Handle_T.
 * Set to 5.
 */
#define HANDLE_MAP_SIZE         (5)

/* internal structures */
/**
 * Structure holding mapping between Df1Library Instances (jobject) to Df1_Interface_Handle_T.
 * This means each Df1Library object talks to one PLC.
 * <dl>
 * <dt>Df1Library_Instance_Handle</dt> <dd>jobject reference for the Df1Library instance.</dd>
 * <dt>Interface_Handle</dt> <dd>Pointer to the Df1_Interface_Handle_T for that Df1Library instance.</dd>
 * </dl>
 */
struct Handle_Map_Struct
{
	jobject Df1Library_Instance_Handle;
	Df1_Interface_Handle_T* Interface_Handle;
};

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id: ngat_frodospec_df1_Df1Library.c,v 1.1 2023-03-21 14:34:10 cjm Exp $";

/**
 * Copy of the java virtual machine pointer, used for logging back up to the Java layer from C.
 */
static JavaVM *java_vm = NULL;
/**
 * Cached global reference to the "ngat.frodospec.df1.Df1Library" logger, 
 * used to log back to the Java layer from C routines.
 */
static jobject logger = NULL;
/**
 * Cached reference to the "ngat.util.logging.Logger" class's log(int level,String message) method.
 * Used to log C layer log messages, in conjunction with the logger's object reference logger.
 * @see #logger
 */
static jmethodID log_method_id = NULL;
/**
 * Internal list of maps between Df1Library jobject's (i.e. Df1Library references), and
 * Df1_Interface_Handle_T handles (which control which /dev/astropci port we talk to).
 * @see #Handle_Map_Struct
 * @see #HANDLE_MAP_SIZE
 */
static struct Handle_Map_Struct Handle_Map_List[HANDLE_MAP_SIZE] = 
{
	{NULL,NULL},
	{NULL,NULL},
	{NULL,NULL},
	{NULL,NULL},
	{NULL,NULL}
};

/* internal routines */
static void Df1Library_Throw_Exception(JNIEnv *env,jobject obj,char *function_name);
static void Df1Library_Throw_Exception_String(JNIEnv *env,jobject obj,char *function_name,char *error_string);
static void Df1Library_Log_Handler(int level,char *string);
static int Df1Library_Handle_Map_Add(JNIEnv *env,jobject instance,Df1_Interface_Handle_T* interface_handle);
static int Df1Library_Handle_Map_Delete(JNIEnv *env,jobject instance);
static int Df1Library_Handle_Map_Find(JNIEnv *env,jobject instance,Df1_Interface_Handle_T** interface_handle);

/* ------------------------------------------------------------------------------
** 		External routines
** ------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------
** 		Df1Library C layer initialisation
** ------------------------------------------------------------------------------ */
/**
 * This routine gets called when the native library is loaded. We use this routine
 * to get a copy of the JavaVM pointer of the JVM we are running in. This is used to
 * get the correct per-thread JNIEnv context pointer in Df1Library_Log_Handler.
 * @see #java_vm
 * @see #Df1Library_Log_Handler
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
	java_vm = vm;
	return JNI_VERSION_1_2;
}

/**
 * Class:     ngat_frodospec_df1_Df1Library<br>
 * Method:    initialiseLoggerReference<br>
 * Signature: (Lngat/util/logging/Logger;)V<br>
 * Java Native Interface implementation Df1Library's initialiseLoggerReference.
 * This takes the supplied logger object reference and stores it in the logger variable as a global reference.
 * The log method ID is also retrieved and stored.
 * The libfrodospec_df1's log handler is set to the JNI routine Df1Library_Log_Handler.
 * The libfrodospec_df1's log filter function is set bitwise.
 * @param l The Df1Library's "ngat.frodospec.df1.Df1Library" logger.
 * @see #Df1Library_Log_Handler
 * @see #logger
 * @see #log_method_id
 * @see df1_general.html#Df1_Log_Filter_Level_Bitwise
 * @see df1_general.html#Df1_Set_Log_Handler_Function
 * @see df1_general.html#Df1_Set_Log_Filter_Function
 */
JNIEXPORT void JNICALL Java_ngat_frodospec_df1_Df1Library_initialiseLoggerReference(JNIEnv *env,jobject obj,
										    jobject l)
{
	jclass cls = NULL;

/* save logger instance */
	logger = (*env)->NewGlobalRef(env,l);
/* get the ngat.util.logging.Logger class */
	cls = (*env)->FindClass(env,"ngat/util/logging/Logger");
	/* if the class is null, one of the following exceptions occured:
	** ClassFormatError,ClassCircularityError,NoClassDefFoundError,OutOfMemoryError */
	if(cls == NULL)
		return;
/* get relevant method id to call */
/* log(int level,java/lang/String message) */
	log_method_id = (*env)->GetMethodID(env,cls,"log","(ILjava/lang/String;)V");
	if(log_method_id == NULL)
	{
		/* One of the following exceptions has been thrown:
		** NoSuchMethodError, ExceptionInInitializerError, OutOfMemoryError */
		return;
	}
	/* Make the C layer log back to the Java logger, using Df1Library_Log_Handler JNI routine.  */
	Df1_Set_Log_Handler_Function(Df1Library_Log_Handler);
	/* Make the filtering bitwise, as expected by the C layer */
	Df1_Set_Log_Filter_Function(Df1_Log_Filter_Level_Bitwise);
}

/**
 * Class:     ngat_frodospec_df1_Df1Library<br>
 * Method:    finaliseLoggerReference<br>
 * Signature: ()V<br>
 * This native method is called from Df1Library's finaliser method. It removes the global reference to
 * logger.
 * @see #logger
 */
JNIEXPORT void JNICALL Java_ngat_frodospec_df1_Df1Library_finaliseLoggerReference(JNIEnv *env, jobject obj)
{
	(*env)->DeleteGlobalRef(env,logger);
}

/* ------------------------------------------------------------------------------
** 		df1_general.c
** ------------------------------------------------------------------------------ */
/**
 * Class:     ngat_frodospec_df1_Df1Library<br>
 * Method:    Df1_Set_Log_Filter_Level<br>
 * Signature: (I)V<br>
 * @see df1_general.html#Df1_Set_Log_Filter_Level
 */
JNIEXPORT void JNICALL Java_ngat_frodospec_df1_Df1Library_Df1_1Set_1Log_1Filter_1Level(JNIEnv *env,jobject obj,
										       jint level)
{
	Df1_Set_Log_Filter_Level(level);
}

/* ------------------------------------------------------------------------------
** 		df1_interface.c
** ------------------------------------------------------------------------------ */
/**
 * Class:     ngat_frodospec_df1_Df1Library<br>
 * Method:    Df1_Interface_Handle_Create<br>
 * Signature: ()V<br>
 * @see df1_interface.html#Df1_Interface_Handle_T
 * @see df1_interface.html#Df1_Interface_Handle_Create
 * @see #Df1Library_Throw_Exception
 * @see #Df1Library_Handle_Map_Add
 */
JNIEXPORT void JNICALL Java_ngat_frodospec_df1_Df1Library_Df1_1Interface_1Handle_1Create(JNIEnv *env, jobject obj)
{
	Df1_Interface_Handle_T *handle = NULL;
	int retval;

	retval = Df1_Interface_Handle_Create(&handle);
	/* if an error occured throw an exception. */
	if(retval == FALSE)
	{
		Df1Library_Throw_Exception(env,obj,"Df1_Interface_Handle_Create");
		return;
	}
	/* map this (obj) to handle */
	retval = Df1Library_Handle_Map_Add(env,obj,handle);
	if(retval == FALSE)
	{
		/* An error should have been thrown by Df1Library_Handle_Map_Add */
		return;
	}
}

/**
 * Class:     ngat_frodospec_df1_Df1Library<br>
 * Method:    Df1_Interface_Handle_Destroy<br>
 * Signature: ()V<br>
 * @see df1_interface.html#Df1_Interface_Handle_T
 * @see df1_interface.html#Df1_Interface_Handle_Destroy
 * @see #Df1Library_Throw_Exception
 * @see #Df1Library_Handle_Map_Find
 * @see #Df1Library_Handle_Map_Delete
 */
JNIEXPORT void JNICALL Java_ngat_frodospec_df1_Df1Library_Df1_1Interface_1Handle_1Destroy(JNIEnv *env, jobject obj)
{
	Df1_Interface_Handle_T *handle = NULL;
	int retval;

	/* get interface handle from Df1Library instance map */
	if(!Df1Library_Handle_Map_Find(env,obj,&handle))
		return; /* Df1Library_Handle_Map_Find throws an exception on failure */
	retval = Df1_Interface_Handle_Destroy(&handle);
	/* if an error occured throw an exception. */
	if(retval == FALSE)
	{
		Df1Library_Throw_Exception(env,obj,"Df1_Interface_Handle_Destroy");
		return;
	}
	/* remove mapping from Df1Library instance to interface handle */
	retval = Df1Library_Handle_Map_Delete(env,obj);
	if(retval == FALSE)
	{
		/* Df1Library_Handle_Map_Delete should have thrown an error if it fails */
		return;
	}
}

/**
 * Class:     ngat_frodospec_df1_Df1Library<br>
 * Method:    Df1_Interface_Open<br>
 * Signature: (ILjava/lang/String;I)V<br>
 * @see df1_interface.html#DF1_INTERFACE_DEVICE_ID
 * @see df1_interface.html#Df1_Interface_Handle_T
 * @see df1_interface.html#Df1_Interface_Open
 * @see #Df1Library_Throw_Exception
 * @see #Df1Library_Handle_Map_Find
 */
JNIEXPORT void JNICALL Java_ngat_frodospec_df1_Df1Library_Df1_1Interface_1Open(JNIEnv *env, jobject obj, 
					    jint device_id, jstring device_name_jstring, jint port_number)
{
	Df1_Interface_Handle_T *handle = NULL;
	const char *device_name = NULL;
	int retval;

	/* get interface handle from Df1Library instance map */
	if(!Df1Library_Handle_Map_Find(env,obj,&handle))
		return; /* Df1Library_Handle_Map_Find throws an exception on failure */
	/* Get the device name from a java string to a c null terminated string
	** If the java String is null the device_name should be null as well */
	if(device_name_jstring != NULL)
		device_name = (*env)->GetStringUTFChars(env,device_name_jstring,0);
	retval = Df1_Interface_Open((enum DF1_INTERFACE_DEVICE_ID)device_id,(char*)device_name,
				    (int)port_number,handle);
	/* If we created the device_name string we need to free the memory it uses */
	if(device_name_jstring != NULL)
		(*env)->ReleaseStringUTFChars(env,device_name_jstring,device_name);
	/* if an error occured throw an exception. */
	if(retval == FALSE)
	{
		Df1Library_Throw_Exception(env,obj,"Df1_Interface_Open");
		return;
	}
}

/**
 * Class:     ngat_frodospec_df1_Df1Library<br>
 * Method:    Df1_Interface_Close<br>
 * Signature: ()V<br>
 * @see #Df1Library_Throw_Exception
 * @see #Df1Library_Handle_Map_Find
 * @see df1_interface.html#Df1_Interface_Handle_T
 * @see df1_interface.html#Df1_Interface_Close
 */
JNIEXPORT void JNICALL Java_ngat_frodospec_df1_Df1Library_Df1_1Interface_1Close(JNIEnv *env, jobject obj)
{
	Df1_Interface_Handle_T *handle = NULL;
	int retval;

	/* get interface handle from Df1Library instance map */
	if(!Df1Library_Handle_Map_Find(env,obj,&handle))
		return; /* Df1Library_Handle_Map_Find throws an exception on failure */
	retval = Df1_Interface_Close(handle);
	/* if an error occured throw an exception. */
	if(retval == FALSE)
	{
		Df1Library_Throw_Exception(env,obj,"Df1_Interface_Close");
		return;
	}
}

/* ------------------------------------------------------------------------------
** 		df1_read_write.c
** ------------------------------------------------------------------------------ */
/**
 * Class:     ngat_frodospec_df1_Df1Library<br>
 * Method:    Df1_Write_Boolean<br>
 * Signature: (Ljava/lang/String;Z)V<br>
 * @see #Df1Library_Throw_Exception
 * @see #Df1Library_Handle_Map_Find
 * @see df1_interface.html#Df1_Interface_Handle_T
 * @see df1_read_write.html#Df1_Write_Boolean
 * @see df1.html#SLC
 */
JNIEXPORT void JNICALL Java_ngat_frodospec_df1_Df1Library_Df1_1Write_1Boolean(JNIEnv *env, jobject obj, 
						            jstring plc_address_jstring, jboolean value)
{
	Df1_Interface_Handle_T *handle = NULL;
	const char *plc_address_c = NULL;
	int retval;

	/* get interface handle from Df1Library instance map */
	if(!Df1Library_Handle_Map_Find(env,obj,&handle))
		return; /* Df1Library_Handle_Map_Find throws an exception on failure */
	/* Get the PLC address from a java string to a c null terminated string
	** If the java String is null the plc_address_c should be null as well */
	if(plc_address_jstring != NULL)
		plc_address_c = (*env)->GetStringUTFChars(env,plc_address_jstring,0);
	retval = Df1_Write_Boolean(handle,SLC,(char *)plc_address_c,(int)value);
	/* If we created the plc_address_c string we need to free the memory it uses */
	if(plc_address_jstring != NULL)
		(*env)->ReleaseStringUTFChars(env,plc_address_jstring,plc_address_c);
	/* if an error occured throw an exception. */
	if(retval == FALSE)
		Df1Library_Throw_Exception(env,obj,"Df1_Write_Boolean");
}

/**
 * Class:     ngat_frodospec_df1_Df1Library<br>
 * Method:    Df1_Read_Boolean<br>
 * Signature: (Ljava/lang/String;)Z<br>
 * @see #Df1Library_Throw_Exception
 * @see #Df1Library_Handle_Map_Find
 * @see df1_interface.html#Df1_Interface_Handle_T
 * @see df1_read_write.html#Df1_Read_Boolean
 * @see df1.html#SLC
 */
JNIEXPORT jboolean JNICALL Java_ngat_frodospec_df1_Df1Library_Df1_1Read_1Boolean(JNIEnv *env, jobject obj,
										 jstring plc_address_jstring)
{
	Df1_Interface_Handle_T *handle = NULL;
	const char *plc_address_c = NULL;
	int retval,value;

	/* get interface handle from Df1Library instance map */
	if(!Df1Library_Handle_Map_Find(env,obj,&handle))
		return (jboolean)FALSE; /* Df1Library_Handle_Map_Find throws an exception on failure */
	/* Get the PLC address from a java string to a c null terminated string
	** If the java String is null the plc_address_c should be null as well */
	if(plc_address_jstring != NULL)
		plc_address_c = (*env)->GetStringUTFChars(env,plc_address_jstring,0);
	retval = Df1_Read_Boolean(handle,SLC,(char *)plc_address_c,&value);
	/* If we created the plc_address_c string we need to free the memory it uses */
	if(plc_address_jstring != NULL)
		(*env)->ReleaseStringUTFChars(env,plc_address_jstring,plc_address_c);
	/* if an error occured throw an exception. */
	if(retval == FALSE)
		Df1Library_Throw_Exception(env,obj,"Df1_Read_Boolean");
	return (jboolean)value;
}

/**
 * Class:     ngat_frodospec_df1_Df1Library<br>
 * Method:    Df1_Write_Integer<br>
 * Signature: (Ljava/lang/String;S)V<br>
 * @see #Df1Library_Throw_Exception
 * @see #Df1Library_Handle_Map_Find
 * @see df1_interface.html#Df1_Interface_Handle_T
 * @see df1_read_write.html#Df1_Write_Integer
 * @see df1.html#SLC
 */
JNIEXPORT void JNICALL Java_ngat_frodospec_df1_Df1Library_Df1_1Write_1Integer(JNIEnv *env, jobject obj,
							      jstring plc_address_jstring, jshort value)
{
	Df1_Interface_Handle_T *handle = NULL;
	const char *plc_address_c = NULL;
	int retval;

	/* get interface handle from Df1Library instance map */
	if(!Df1Library_Handle_Map_Find(env,obj,&handle))
		return; /* Df1Library_Handle_Map_Find throws an exception on failure */
	/* Get the PLC address from a java string to a c null terminated string
	** If the java String is null the plc_address_c should be null as well */
	if(plc_address_jstring != NULL)
		plc_address_c = (*env)->GetStringUTFChars(env,plc_address_jstring,0);
	retval = Df1_Write_Integer(handle,SLC,(char *)plc_address_c,(word)value);
	/* If we created the plc_address_c string we need to free the memory it uses */
	if(plc_address_jstring != NULL)
		(*env)->ReleaseStringUTFChars(env,plc_address_jstring,plc_address_c);
	/* if an error occured throw an exception. */
	if(retval == FALSE)
		Df1Library_Throw_Exception(env,obj,"Df1_Write_Integer");
}

/**
 * Class:     ngat_frodospec_df1_Df1Library<br>
 * Method:    Df1_Read_Integer<br>
 * Signature: (Ljava/lang/String;)S<br>
 * @see #Df1Library_Throw_Exception
 * @see #Df1Library_Handle_Map_Find
 * @see df1_interface.html#Df1_Interface_Handle_T
 * @see df1_read_write.html#Df1_Read_Integer
 * @see df1.html#SLC
 */
JNIEXPORT jshort JNICALL Java_ngat_frodospec_df1_Df1Library_Df1_1Read_1Integer(JNIEnv *env, jobject obj,
									       jstring plc_address_jstring)
{
	Df1_Interface_Handle_T *handle = NULL;
	const char *plc_address_c = NULL;
	int retval;
	word value;

	/* get interface handle from Df1Library instance map */
	if(!Df1Library_Handle_Map_Find(env,obj,&handle))
		return 0; /* Df1Library_Handle_Map_Find throws an exception on failure */
	/* Get the PLC address from a java string to a c null terminated string
	** If the java String is null the plc_address_c should be null as well */
	if(plc_address_jstring != NULL)
		plc_address_c = (*env)->GetStringUTFChars(env,plc_address_jstring,0);
	retval = Df1_Read_Integer(handle,SLC,(char *)plc_address_c,&value);
	/* If we created the plc_address_c string we need to free the memory it uses */
	if(plc_address_jstring != NULL)
		(*env)->ReleaseStringUTFChars(env,plc_address_jstring,plc_address_c);
	/* if an error occured throw an exception. */
	if(retval == FALSE)
		Df1Library_Throw_Exception(env,obj,"Df1_Read_Integer");
	return (jshort)value;
}

/**
 * Class:     ngat_frodospec_df1_Df1Library<br>
 * Method:    Df1_Write_Float<br>
 * Signature: (Ljava/lang/String;F)V<br>
 * @see #Df1Library_Throw_Exception
 * @see #Df1Library_Handle_Map_Find
 * @see df1_interface.html#Df1_Interface_Handle_T
 * @see df1_read_write.html#Df1_Write_Float
 * @see df1.html#SLC
 */
JNIEXPORT void JNICALL Java_ngat_frodospec_df1_Df1Library_Df1_1Write_1Float(JNIEnv *env, jobject obj, 
									    jstring plc_address_jstring, jfloat value)
{
	Df1_Interface_Handle_T *handle = NULL;
	const char *plc_address_c = NULL;
	int retval;

	/* get interface handle from Df1Library instance map */
	if(!Df1Library_Handle_Map_Find(env,obj,&handle))
		return; /* Df1Library_Handle_Map_Find throws an exception on failure */
	/* Get the PLC address from a java string to a c null terminated string
	** If the java String is null the plc_address_c should be null as well */
	if(plc_address_jstring != NULL)
		plc_address_c = (*env)->GetStringUTFChars(env,plc_address_jstring,0);
	retval = Df1_Write_Float(handle,SLC,(char *)plc_address_c,(float)value);
	/* If we created the plc_address_c string we need to free the memory it uses */
	if(plc_address_jstring != NULL)
		(*env)->ReleaseStringUTFChars(env,plc_address_jstring,plc_address_c);
	/* if an error occured throw an exception. */
	if(retval == FALSE)
		Df1Library_Throw_Exception(env,obj,"Df1_Write_Float");
}

/**
 * Class:     ngat_frodospec_df1_Df1Library<br>
 * Method:    Df1_Read_Float<br>
 * Signature: (Ljava/lang/String;)F<br>
 * @see #Df1Library_Throw_Exception
 * @see #Df1Library_Handle_Map_Find
 * @see df1_interface.html#Df1_Interface_Handle_T
 * @see df1_read_write.html#Df1_Read_Float
 * @see df1.html#SLC
 */
JNIEXPORT jfloat JNICALL Java_ngat_frodospec_df1_Df1Library_Df1_1Read_1Float(JNIEnv *env, jobject obj, 
									     jstring plc_address_jstring)
{
	Df1_Interface_Handle_T *handle = NULL;
	const char *plc_address_c = NULL;
	int retval;
	float value;

	/* get interface handle from Df1Library instance map */
	if(!Df1Library_Handle_Map_Find(env,obj,&handle))
		return 0; /* Df1Library_Handle_Map_Find throws an exception on failure */
	/* Get the PLC address from a java string to a c null terminated string
	** If the java String is null the plc_address_c should be null as well */
	if(plc_address_jstring != NULL)
		plc_address_c = (*env)->GetStringUTFChars(env,plc_address_jstring,0);
	retval = Df1_Read_Float(handle,SLC,(char *)plc_address_c,&value);
	/* If we created the plc_address_c string we need to free the memory it uses */
	if(plc_address_jstring != NULL)
		(*env)->ReleaseStringUTFChars(env,plc_address_jstring,plc_address_c);
	/* if an error occured throw an exception. */
	if(retval == FALSE)
		Df1Library_Throw_Exception(env,obj,"Df1_Read_Float");
	return (jfloat)value;
}

/* ------------------------------------------------------------------------------
** 		Internal routines
** ------------------------------------------------------------------------------ */
/**
 * This routine throws an exception. The error generated is from the error codes in libfrodospec_df1, 
 * it assumes another routine has generated an error and this routine packs this error into an exception to return
 * to the Java code, using Df1Library_Throw_Exception_String. The total length of the error string should
 * not be longer than DF1_ERROR_LENGTH. A new line is added to the start of the error string,
 * so that the error string returned from libfrodospec_df1 is formatted properly.
 * @param env The JNI environment pointer.
 * @param function_name The name of the function in which this exception is being generated for.
 * @param obj The instance of Df1Library that threw the error.
 * @see df1_general.html#Df1_Error_To_String
 * @see #Df1Library_Throw_Exception_String
 * @see #DF1_ERROR_LENGTH
 */
static void Df1Library_Throw_Exception(JNIEnv *env,jobject obj,char *function_name)
{
	char error_string[DF1_ERROR_LENGTH];

	strcpy(error_string,"\n");
	Df1_Error_To_String(error_string+strlen(error_string));
	Df1Library_Throw_Exception_String(env,obj,function_name,error_string);
}

/**
 * This routine throws an exception of class ngat/frodospec/df1/Df1LibraryNativeException. 
 * This is used to report all libfrodospec_df1 error messages back to the Java layer.
 * @param env The JNI environment pointer.
 * @param obj The instance of Df1Library that threw the error.
 * @param function_name The name of the function in which this exception is being generated for.
 * @param error_string The string to pass to the constructor of the exception.
 */
static void Df1Library_Throw_Exception_String(JNIEnv *env,jobject obj,char *function_name,char *error_string)
{
	jclass exception_class = NULL;
	jobject exception_instance = NULL;
	jstring error_jstring = NULL;
	jmethodID mid;
	int retval;

	exception_class = (*env)->FindClass(env,"ngat/frodospec/df1/Df1LibraryNativeException");
	if(exception_class != NULL)
	{
	/* get Df1LibraryNativeException constructor */
		mid = (*env)->GetMethodID(env,exception_class,"<init>",
					  "(Ljava/lang/String;Lngat/frodospec/df1/Df1Library;)V");
		if(mid == 0)
		{
			/* One of the following exceptions has been thrown:
			** NoSuchMethodError, ExceptionInInitializerError, OutOfMemoryError */
			fprintf(stderr,"Df1Library_Throw_Exception_String:GetMethodID failed:%s:%s\n",function_name,
				error_string);
			return;
		}
	/* convert error_string to JString */
		error_jstring = (*env)->NewStringUTF(env,error_string);
	/* call constructor */
		exception_instance = (*env)->NewObject(env,exception_class,mid,error_jstring,obj);
		if(exception_instance == NULL)
		{
			/* One of the following exceptions has been thrown:
			** InstantiationException, OutOfMemoryError */
			fprintf(stderr,"Df1Library_Throw_Exception_String:NewObject failed %s:%s\n",
				function_name,error_string);
			return;
		}
	/* throw instance */
		retval = (*env)->Throw(env,(jthrowable)exception_instance);
		if(retval !=0)
		{
			fprintf(stderr,"Df1Library_Throw_Exception_String:Throw failed %d:%s:%s\n",retval,
				function_name,error_string);
		}
	}
	else
	{
		fprintf(stderr,"Df1Library_Throw_Exception_String:FindClass failed:%s:%s\n",function_name,
			error_string);
	}
}

/**
 * libfrodospec_df1 Log Handler for the Java layer interface. 
 * This calls the ngat.frodospec.df1.Df1Library logger's 
 * log(int level,String message) method with the parameters supplied to this routine.
 * If the logger instance is NULL, or the log_method_id is NULL the call is not made.
 * Otherwise, A java.lang.String instance is constructed from the string parameter,
 * and the JNI CallVoidMEthod routine called to call log().
 * @param level The log level of the message.
 * @param string The message to log.
 * @see #java_vm
 * @see #logger
 * @see #log_method_id
 */
static void Df1Library_Log_Handler(int level,char *string)
{
	JNIEnv *env = NULL;
	jstring java_string = NULL;

	if(logger == NULL)
	{
		fprintf(stderr,"Df1Library_Log_Handler:logger was NULL (%d,%s).\n",level,string);
		return;
	}
	if(log_method_id == NULL)
	{
		fprintf(stderr,"Df1Library_Log_Handler:log_method_id was NULL (%d,%s).\n",level,string);
		return;
	}
	if(java_vm == NULL)
	{
		fprintf(stderr,"Df1Library_Log_Handler:java_vm was NULL (%d,%s).\n",level,string);
		return;
	}
/* get java env for this thread */
	(*java_vm)->AttachCurrentThread(java_vm,(void**)&env,NULL);
	if(env == NULL)
	{
		fprintf(stderr,"Df1Library_Log_Handler:env was NULL (%d,%s).\n",level,string);
		return;
	}
	if(string == NULL)
	{
		fprintf(stderr,"Df1Library_Log_Handler:string (%d) was NULL.\n",level);
		return;
	}
/* convert C to Java String */
	java_string = (*env)->NewStringUTF(env,string);
/* call log method on logger instance */
	(*env)->CallVoidMethod(env,logger,log_method_id,(jint)level,java_string);
}

/**
 * Routine to add a mapping from the Df1Library instance instance to the opened Df1 Interface Handle
 * interface_handle, in the Handle_Map_List.
 * @param instance The Df1Library instance.
 * @param interface_handle The interface handle.
 * @return The routine returns TRUE if the map is added (or updated), FALSE if there was no room left
 *         in the mapping list. 
 *         Df1Library_Throw_Exception_String is used to throw a Java exception if the routine returns FALSE.
 * @see #HANDLE_MAP_SIZE
 * @see #Handle_Map_List
 * @see #Df1Library_Throw_Exception_String
 */
static int Df1Library_Handle_Map_Add(JNIEnv *env,jobject instance,Df1_Interface_Handle_T* interface_handle)
{
	int i,done;
	jobject global_instance = NULL;

	/* does the map already exist? */
	i = 0;
	done = FALSE;
	while((i < HANDLE_MAP_SIZE)&&(done == FALSE))
	{
		if((*env)->IsSameObject(env,Handle_Map_List[i].Df1Library_Instance_Handle,instance))
			done = TRUE;
		else
			i++;
	}
	if(done == TRUE)/* found an existing interface handle for this Df1Library instance */
	{
		/* update handle */
		Handle_Map_List[i].Interface_Handle = interface_handle;
	}
	else
	{
		/* look for a blank index to put the map */
		i = 0;
		done = FALSE;
		while((i < HANDLE_MAP_SIZE)&&(done == FALSE))
		{
			if(Handle_Map_List[i].Df1Library_Instance_Handle == NULL)
				done = TRUE;
			else
				i++;
		}
		if(done == FALSE)
		{
			Df1Library_Throw_Exception_String(env,instance,"Df1Library_Handle_Map_Add",
							  "No empty slots in handle map.");
			return FALSE;
		}
		/* index i is free, add handle map here */
		global_instance = (*env)->NewGlobalRef(env,instance);
		if(global_instance == NULL)
		{
			Df1Library_Throw_Exception_String(env,instance,"Df1Library_Handle_Map_Add",
							  "Failed to create Global reference of instance.");
			return FALSE;
		}
		fprintf(stdout,"Df1Library_Handle_Map_Add:Adding instance %p with handle %p at map index %d.\n",
			(void*)global_instance,(void*)interface_handle,i);
		Handle_Map_List[i].Df1Library_Instance_Handle = global_instance;
		Handle_Map_List[i].Interface_Handle = interface_handle;
	}
	return TRUE;
}

/**
 * Routine to delete a mapping from the Df1Library instance instance to the opened Df1 Interface Handle
 * interface_handle, in the Handle_Map_List.
 * @param instance The Df1Library instance to remove from the list.
 * @return The routine returns TRUE if the map is deleted (or updated), FALSE if the mapping could not be found
 *         in the mapping list.
 *         Df1Library_Throw_Exception_String is used to throw a Java exception if the routine returns FALSE.
 * @see #HANDLE_MAP_SIZE
 * @see #Handle_Map_List
 * @see #Df1Library_Throw_Exception_String
 */
static int Df1Library_Handle_Map_Delete(JNIEnv *env,jobject instance)
{
	int i,done;

  	/* does the map already exist? */
	i = 0;
	done = FALSE;
	while((i < HANDLE_MAP_SIZE)&&(done == FALSE))
	{
		if((*env)->IsSameObject(env,Handle_Map_List[i].Df1Library_Instance_Handle,instance))
			done = TRUE;
		else
			i++;
	}
	if(done == FALSE)
	{
		Df1Library_Throw_Exception_String(env,instance,"Df1Library_Handle_Map_Delete",
						  "Failed to find Df1Library instance in handle map.");
		return FALSE;
	}
	/* found an existing interface handle for this Df1Library instance at index i */
	/* delete this map at index i */
	fprintf(stdout,"Df1Library_Handle_Map_Delete:Deleting instance %p with handle %p at map index %d.\n",
		(void*)Handle_Map_List[i].Df1Library_Instance_Handle,(void*)Handle_Map_List[i].Interface_Handle,i);
	(*env)->DeleteGlobalRef(env,Handle_Map_List[i].Df1Library_Instance_Handle);
	Handle_Map_List[i].Df1Library_Instance_Handle = NULL;
	Handle_Map_List[i].Interface_Handle = NULL;
	return TRUE;
}

/**
 * Routine to find a mapping from the Df1Library instance instance to the opened Df1 Interface Handle
 * interface_handle, in the Handle_Map_List.
 * @param instance The Df1Library instance.
 * @param interface_handle The address of an interface handle, to fill with the interface handle for
 *        this Df1Library instance, if one is successfully found.
 * @return The routine returns TRUE if the mapping is found and returned,, FALSE if there was no mapping
 *         for this Df1Library instance, or the interface_handle pointer was NULL.
 *         Df1Library_Throw_Exception_String is used to throw a Java exception if the routine returns FALSE.
 * @see #HANDLE_MAP_SIZE
 * @see #Handle_Map_List
 * @see #Df1Library_Throw_Exception_String
 */
static int Df1Library_Handle_Map_Find(JNIEnv *env,jobject instance,Df1_Interface_Handle_T** interface_handle)
{
	int i,done;

	if(interface_handle == NULL)
	{
		Df1Library_Throw_Exception_String(env,instance,"Df1Library_Handle_Map_Find",
						  "interface handle was NULL.");
		return FALSE;
	}
	i = 0;
	done = FALSE;
	while((i < HANDLE_MAP_SIZE)&&(done == FALSE))
	{
		if((*env)->IsSameObject(env,Handle_Map_List[i].Df1Library_Instance_Handle,instance))
			done = TRUE;
		else
			i++;
	}
	if(done == FALSE)
	{
		fprintf(stdout,"Df1Library_Handle_Map_Find:Failed to find instance %p.\n",(void*)instance);
		Df1Library_Throw_Exception_String(env,instance,"Df1Library_Handle_Map_Find",
						  "Df1Library instance handle was not found.");
		return FALSE;
	}
	(*interface_handle) = Handle_Map_List[i].Interface_Handle;
	return TRUE;
}
/*
** $Log: not supported by cvs2svn $
*/

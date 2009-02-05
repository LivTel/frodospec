/* ngat_frodospec_newmark_Newmark.c
** implementation of Java Class ngat.frodospec.newmark.Newmark native interfaces
** $Header: /home/cjm/cvs/frodospec/newmark_motion_controller/c/ngat_frodospec_newmark_Newmark.c,v 1.2 2009-02-05 11:41:03 cjm Exp $
*/
/**
 * ngat_frodospec_newmark_Newmark.c is the 'glue' between libfrodospec_newmark, 
 * the C library used to control Newmark slide, and Newmark.java, 
 * a Java Class to drive the motion controller. Newmark specifically
 * contains all the native C routines corresponding to native methods in Java.
 * @author Chris Mottram LJMU
 * @version $Revision: 1.2 $
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
#include "newmark_general.h"
#include "newmark_command.h"

/**
 * Special external declaration of ArcomESS_Handle_Map_Find.
 * This routine is implemented in libarcom_ess, as part of it's JNI interface code (ngat_serial_arcomess_ArcomESS.c).
 * Unfortunately, the header file is machine generated from Java and so can't contain this function declaration.
 * This declaration is used in Newmark_Handle_Map_Add to internally go from the passed in ArcomESS instance
 * to the libarcom_ess Arcom_ESS_Interface_Handle_T mapping (using libarcom_ess's internal JNI Handle_Map).
 */
extern int ArcomESS_Handle_Map_Find(JNIEnv *env,jobject instance,Arcom_ESS_Interface_Handle_T** interface_handle);

/* hash definitions */

/**
 * Hash define for the size of the array holding Df1Library Instance (jobject) maps to Arcom_ESS_Interface_Handle_T.
 * Set to 5.
 */
#define HANDLE_MAP_SIZE         (5)

/* internal structures */
/**
 * Structure holding mapping between Newmark Instances (jobject) to 
 * the corresponding ArcomESS instance (jobject) and it's associated Arcom_ESS_Interface_Handle_T.
 * This means each Newmark object talks to one motion controller via one ArcomESS connection.
 * <dl>
 * <dt>Newmark_Instance_Handle</dt> <dd>jobject reference for the Newmark instance.</dd>
 * <dt>ArcomESS_Instance_Handle</dt> <dd>jobject reference for the ArcomESS instance.</dd>
 * <dt>Interface_Handle</dt> <dd>Pointer to the Arcom_ESS_Interface_Handle_T.</dd>
 * </dl>
 */
struct Handle_Map_Struct
{
	jobject Newmark_Instance_Handle;
	jobject ArcomESS_Instance_Handle;
	Arcom_ESS_Interface_Handle_T* Interface_Handle;
};

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id: ngat_frodospec_newmark_Newmark.c,v 1.2 2009-02-05 11:41:03 cjm Exp $";

/**
 * Copy of the java virtual machine pointer, used for logging back up to the Java layer from C.
 */
static JavaVM *java_vm = NULL;
/**
 * Cached global reference to the "ngat.frodospec.newmark.Newmark" logger, 
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
 * Internal list of maps between Newmark jobject's (i.e. Newmark references), ArcomESS jobject's, and
 * Arcom_ESS_Interface_Handle_T handles.
 * @see #Handle_Map_Struct
 * @see #HANDLE_MAP_SIZE
 */
static struct Handle_Map_Struct Handle_Map_List[HANDLE_MAP_SIZE] = 
{
	{NULL,NULL,NULL},
	{NULL,NULL,NULL},
	{NULL,NULL,NULL},
	{NULL,NULL,NULL},
	{NULL,NULL,NULL}
};

/* internal routines */
static void Newmark_Throw_Exception(JNIEnv *env,jobject obj,char *function_name);
static void Newmark_Throw_Exception_String(JNIEnv *env,jobject obj,char *function_name,char *error_string);
static void Newmark_Log_Handler(int level,char *string);
static int Newmark_Handle_Map_Add(JNIEnv *env,jobject newmark_instance,jobject arcom_ess_instance);
static int Newmark_Handle_Map_Delete(JNIEnv *env,jobject instance);
static int Newmark_Handle_Map_Find(JNIEnv *env,jobject instance,Arcom_ESS_Interface_Handle_T** interface_handle);

/* ------------------------------------------------------------------------------
** 		External routines
** ------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------
** 		Df1Library C layer initialisation
** ------------------------------------------------------------------------------ */
/**
 * This routine gets called when the native library is loaded. We use this routine
 * to get a copy of the JavaVM pointer of the JVM we are running in. This is used to
 * get the correct per-thread JNIEnv context pointer in Newmark_Log_Handler.
 * @see #java_vm
 * @see #Newmark_Log_Handler
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
	java_vm = vm;
	return JNI_VERSION_1_2;
}

/**
 * Class:     ngat_frodospec_newmark_Newmark<br>
 * Method:    initialiseLoggerReference<br>
 * Signature: (Lngat/util/logging/Logger;)V<br>
 * Java Native Interface implementation Newmark's initialiseLoggerReference.
 * This takes the supplied logger object reference and stores it in the logger variable as a global reference.
 * The log method ID is also retrieved and stored.
 * The libfrodospec_newmark's log handler is set to the JNI routine Newmark_Log_Handler.
 * The libfrodospec_newmark's log filter function is set absolute.
 * @param l The Newmark's "ngat.frodospec.newmark.Newmark" logger.
 * @see #Newmark_Log_Handler
 * @see #logger
 * @see #log_method_id
 * @see newmark_general.html#Newmark_Log_Filter_Level_Absolute
 * @see newmark_general.html#Newmark_Set_Log_Handler_Function
 * @see newmark_general.html#Newmark_Set_Log_Filter_Function
 */
JNIEXPORT void JNICALL Java_ngat_frodospec_newmark_Newmark_initialiseLoggerReference(JNIEnv *env,jobject obj,
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
	/* Make the C layer log back to the Java logger, using Newmark_Log_Handler JNI routine.  */
	Newmark_Set_Log_Handler_Function(Newmark_Log_Handler);
	/* Make the filtering absolute, as expected by the C layer */
	Newmark_Set_Log_Filter_Function(Newmark_Log_Filter_Level_Absolute);
}

/**
 * Class:     ngat_frodospec_newmark_Newmark<br>
 * Method:    finaliseLoggerReference<br>
 * Signature: ()V<br>
 * This native method is called from Newmark's finaliser method. It removes the global reference to
 * logger.
 * @see #logger
 */
JNIEXPORT void JNICALL Java_ngat_frodospec_newmark_Newmark_finaliseLoggerReference(JNIEnv *env, jobject obj)
{
	(*env)->DeleteGlobalRef(env,logger);
}

/**
 * Class:     ngat_frodospec_newmark_Newmark<br>
 * Method:    initialiseHandle<br>
 * Signature: (Lngat/serial/arcomess/ArcomESS;)V<br>
 */
JNIEXPORT void JNICALL Java_ngat_frodospec_newmark_Newmark_initialiseHandle(JNIEnv *env,jobject obj,jobject arcom_ess)
{
	int retval;

	/* map this (obj) to handle */
	retval = Newmark_Handle_Map_Add(env,obj,arcom_ess);
	if(retval == FALSE)
	{
		/* An error should have been thrown by Newmark_Handle_Map_Add */
		return;
	}
}

/**
 * Class:     ngat_frodospec_newmark_Newmark<br>
 * Method:    finaliseHandle<br>
 * Signature: ()V<br>
 */
JNIEXPORT void JNICALL Java_ngat_frodospec_newmark_Newmark_finaliseHandle(JNIEnv *env,jobject obj)
{
	int retval;

	/* remove mapping from Newmark instance to interface handle */
	retval = Newmark_Handle_Map_Delete(env,obj);
	if(retval == FALSE)
	{
		/* Newmark_Handle_Map_Delete should have thrown an error if it fails */
		return;
	}
}


/* ------------------------------------------------------------------------------
** 		newmark_general.c
** ------------------------------------------------------------------------------ */

/**
 * Class:     ngat_frodospec_newmark_Newmark<br>
 * Method:    Newmark_Set_Log_Filter_Level<br>
 * Signature: (I)V<br>
 * @see newmark_general.html#Newmark_Set_Log_Filter_Level
 */
JNIEXPORT void JNICALL Java_ngat_frodospec_newmark_Newmark_Newmark_1Set_1Log_1Filter_1Level(JNIEnv *env,jobject obj,
										       jint level)
{
	Newmark_Set_Log_Filter_Level(level);
}

/* ------------------------------------------------------------------------------
** 		newmark_command.c
** ------------------------------------------------------------------------------ */
/**
 * Class:     ngat_frodospec_newmark_Newmark<br>
 * Method:    Newmark_Command_Home<br>
 * Signature: ()V<br>
 */
JNIEXPORT void JNICALL Java_ngat_frodospec_newmark_Newmark_Newmark_1Command_1Home(JNIEnv *env,jobject obj)
{
	Arcom_ESS_Interface_Handle_T *handle = NULL;
	int retval;

	/* get interface handle from Newmark instance map */
	if(!Newmark_Handle_Map_Find(env,obj,&handle))
		return; /* Newmark_Handle_Map_Find throws an exception on failure */
	retval = Newmark_Command_Home(handle);
	/* if an error occured throw an exception. */
	if(retval == FALSE)
		Newmark_Throw_Exception(env,obj,"Newmark_Command_Home");
}

/**
 * Class:     ngat_frodospec_newmark_Newmark<br>
 * Method:    Newmark_Command_Move<br>
 * Signature: (D)V<br>
 */
JNIEXPORT void JNICALL Java_ngat_frodospec_newmark_Newmark_Newmark_1Command_1Move(JNIEnv *env,jobject obj,
										  jdouble position)
{
	Arcom_ESS_Interface_Handle_T *handle = NULL;
	int retval;

	/* get interface handle from Newmark instance map */
	if(!Newmark_Handle_Map_Find(env,obj,&handle))
		return; /* Newmark_Handle_Map_Find throws an exception on failure */
	retval = Newmark_Command_Move(handle,(double)position);
	/* if an error occured throw an exception. */
	if(retval == FALSE)
		Newmark_Throw_Exception(env,obj,"Newmark_Command_Move");
}

/**
 * Class:     ngat_frodospec_newmark_Newmark<br>
 * Method:    Newmark_Command_Abort_Move<br>
 * Signature: ()V<br>
 */
JNIEXPORT void JNICALL Java_ngat_frodospec_newmark_Newmark_Newmark_1Command_1Abort_1Move(JNIEnv *env,jobject obj)
{
	Arcom_ESS_Interface_Handle_T *handle = NULL;
	int retval;

	/* get interface handle from Newmark instance map */
	if(!Newmark_Handle_Map_Find(env,obj,&handle))
		return; /* Newmark_Handle_Map_Find throws an exception on failure */
	retval = Newmark_Command_Abort_Move(handle);
	/* if an error occured throw an exception. */
	if(retval == FALSE)
		Newmark_Throw_Exception(env,obj,"Newmark_Command_Abort_Move");
}

/**
 * Class:     ngat_frodospec_newmark_Newmark<br>
 * Method:    Newmark_Command_Position_Get<br>
 * Signature: ()D<br>
 */
JNIEXPORT jdouble JNICALL Java_ngat_frodospec_newmark_Newmark_Newmark_1Command_1Position_1Get(JNIEnv *env,jobject obj)
{
	Arcom_ESS_Interface_Handle_T *handle = NULL;
	double position;
	int retval;

	/* get interface handle from Newmark instance map */
	if(!Newmark_Handle_Map_Find(env,obj,&handle))
		return 0.0; /* Newmark_Handle_Map_Find throws an exception on failure */
	retval = Newmark_Command_Position_Get(handle,&position);
	/* if an error occured throw an exception. */
	if(retval == FALSE)
	{
		Newmark_Throw_Exception(env,obj,"Newmark_Command_Position_Get");
		return 0.0;
	}
	return (jdouble)position;
}

/**
 * Class:     ngat_frodospec_newmark_Newmark<br>
 * Method:    Newmark_Command_Position_Tolerance_Set<br>
 * Signature: (D)V<br>
 */
JNIEXPORT void JNICALL Java_ngat_frodospec_newmark_Newmark_Newmark_1Command_1Position_1Tolerance_1Set(JNIEnv *env,
										  jobject obj, jdouble mm)
{
	int retval;

	retval = Newmark_Command_Position_Tolerance_Set(mm);
	/* if an error occured throw an exception. */
	if(retval == FALSE)
	{
		Newmark_Throw_Exception(env,obj,"Newmark_Command_Position_Tolerance_Set");
		return;
	}
}


/* ------------------------------------------------------------------------------
** 		Internal routines
** ------------------------------------------------------------------------------ */
/**
 * This routine throws an exception. The error generated is from the error codes in libfrodospec_newmark, 
 * it assumes another routine has generated an error and this routine packs this error into an exception to return
 * to the Java code, using Newmark_Throw_Exception_String. The total length of the error string should
 * not be longer than NEWMARK_ERROR_LENGTH. A new line is added to the start of the error string,
 * so that the error string returned from libfrodospec_df1 is formatted properly.
 * @param env The JNI environment pointer.
 * @param function_name The name of the function in which this exception is being generated for.
 * @param obj The instance of Newmark that threw the error.
 * @see newmark_general.html#Newmark_Error_To_String
 * @see #Newmark_Throw_Exception_String
 * @see #NEWMARK_ERROR_LENGTH
 */
static void Newmark_Throw_Exception(JNIEnv *env,jobject obj,char *function_name)
{
	char error_string[NEWMARK_ERROR_LENGTH];

	strcpy(error_string,"\n");
	Newmark_Error_To_String(error_string+strlen(error_string));
	Newmark_Throw_Exception_String(env,obj,function_name,error_string);
}

/**
 * This routine throws an exception of class ngat/frodospec/newmark/NewmarkNativeException. 
 * This is used to report all libfrodospec_newmark error messages back to the Java layer.
 * @param env The JNI environment pointer.
 * @param obj The instance of Newmark that threw the error.
 * @param function_name The name of the function in which this exception is being generated for.
 * @param error_string The string to pass to the constructor of the exception.
 */
static void Newmark_Throw_Exception_String(JNIEnv *env,jobject obj,char *function_name,char *error_string)
{
	jclass exception_class = NULL;
	jobject exception_instance = NULL;
	jstring error_jstring = NULL;
	jmethodID mid;
	int retval;

	exception_class = (*env)->FindClass(env,"ngat/frodospec/newmark/NewmarkNativeException");
	if(exception_class != NULL)
	{
	/* get Df1LibraryNativeException constructor */
		mid = (*env)->GetMethodID(env,exception_class,"<init>",
					  "(Ljava/lang/String;Lngat/frodospec/newmark/Newmark;)V");
		if(mid == 0)
		{
			/* One of the following exceptions has been thrown:
			** NoSuchMethodError, ExceptionInInitializerError, OutOfMemoryError */
			fprintf(stderr,"Newmark_Throw_Exception_String:GetMethodID failed:%s:%s\n",function_name,
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
			fprintf(stderr,"Newmark_Throw_Exception_String:NewObject failed %s:%s\n",
				function_name,error_string);
			return;
		}
	/* throw instance */
		retval = (*env)->Throw(env,(jthrowable)exception_instance);
		if(retval !=0)
		{
			fprintf(stderr,"Newmark_Throw_Exception_String:Throw failed %d:%s:%s\n",retval,
				function_name,error_string);
		}
	}
	else
	{
		fprintf(stderr,"Newmark_Throw_Exception_String:FindClass failed:%s:%s\n",function_name,
			error_string);
	}
}

/**
 * libfrodospec_newmark Log Handler for the Java layer interface. 
 * This calls the ngat.frodospec.newmark.Newmark logger's 
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
static void Newmark_Log_Handler(int level,char *string)
{
	JNIEnv *env = NULL;
	jstring java_string = NULL;

	if(logger == NULL)
	{
		fprintf(stderr,"Newmark_Log_Handler:logger was NULL (%d,%s).\n",level,string);
		return;
	}
	if(log_method_id == NULL)
	{
		fprintf(stderr,"Newmark_Log_Handler:log_method_id was NULL (%d,%s).\n",level,string);
		return;
	}
	if(java_vm == NULL)
	{
		fprintf(stderr,"Newmark_Log_Handler:java_vm was NULL (%d,%s).\n",level,string);
		return;
	}
/* get java env for this thread */
	(*java_vm)->AttachCurrentThread(java_vm,(void**)&env,NULL);
	if(env == NULL)
	{
		fprintf(stderr,"Newmark_Log_Handler:env was NULL (%d,%s).\n",level,string);
		return;
	}
	if(string == NULL)
	{
		fprintf(stderr,"Newmark_Log_Handler:string (%d) was NULL.\n",level);
		return;
	}
/* convert C to Java String */
	java_string = (*env)->NewStringUTF(env,string);
/* call log method on logger instance */
	(*env)->CallVoidMethod(env,logger,log_method_id,(jint)level,java_string);
}

/**
 * Routine to add a mapping from the Newmark instance instance to the opened ArcomESS Interface Handle
 * interface_handle, in the Handle_Map_List.
 * @param newmark_instance The Newmark Java object instance.
 * @param arcom_ess_instance The ArcomESS Java object instance.
 * @return The routine returns TRUE if the map is added (or updated), FALSE if there was no room left
 *         in the mapping list. 
 *         Newmark_Throw_Exception_String is used to throw a Java exception if the routine returns FALSE.
 * @see #HANDLE_MAP_SIZE
 * @see #Handle_Map_List
 * @see #Newmark_Throw_Exception_String
 */
static int Newmark_Handle_Map_Add(JNIEnv *env,jobject newmark_instance,jobject arcom_ess_instance)
{
	Arcom_ESS_Interface_Handle_T* arcom_ess_interface_handle = NULL;
	int i,done;
	jobject global_newmark_instance = NULL;
	jobject global_arcom_ess_instance = NULL;

	/* get Arcom_ESS_Interface_Handle from arcom_ess_instance */
	if(!ArcomESS_Handle_Map_Find(env,arcom_ess_instance,&arcom_ess_interface_handle))
	{
		Newmark_Throw_Exception_String(env,newmark_instance,"Newmark_Handle_Map_Add",
						  "ArcomESS_Handle_Map_Find failed to find handle.");
		return FALSE;
	}
	/* does the map already exist? */
	i = 0;
	done = FALSE;
	while((i < HANDLE_MAP_SIZE)&&(done == FALSE))
	{
		if((*env)->IsSameObject(env,Handle_Map_List[i].Newmark_Instance_Handle,newmark_instance))
			done = TRUE;
		else
			i++;
	}
	if(done == TRUE)/* found an existing interface handle for this Df1Library instance */
	{
		/* update handle */
		Handle_Map_List[i].ArcomESS_Instance_Handle = arcom_ess_instance;
		Handle_Map_List[i].Interface_Handle = arcom_ess_interface_handle;
	}
	else
	{
		/* look for a blank index to put the map */
		i = 0;
		done = FALSE;
		while((i < HANDLE_MAP_SIZE)&&(done == FALSE))
		{
			if(Handle_Map_List[i].Newmark_Instance_Handle == NULL)
				done = TRUE;
			else
				i++;
		}
		if(done == FALSE)
		{
			Newmark_Throw_Exception_String(env,newmark_instance,"Newmark_Handle_Map_Add",
							  "No empty slots in handle map.");
			return FALSE;
		}
		/* index i is free, add handle map here */
		global_newmark_instance = (*env)->NewGlobalRef(env,newmark_instance);
		if(global_newmark_instance == NULL)
		{
			Newmark_Throw_Exception_String(env,newmark_instance,"Newmark_Handle_Map_Add",
						       "Failed to create Global reference of newmark_instance.");
			return FALSE;
		}
		global_arcom_ess_instance = (*env)->NewGlobalRef(env,arcom_ess_instance);
		if(global_arcom_ess_instance == NULL)
		{
			Newmark_Throw_Exception_String(env,newmark_instance,"Newmark_Handle_Map_Add",
						       "Failed to create Global reference of arcom_ess_instance.");
			return FALSE;
		}
		/*
		fprintf(stdout,"Df1Library_Handle_Map_Add:""Adding newmark_instance %p, arcom_ess_instance %p with handle %p at map index %d.\n",
			(void*)global_newmark_instance,(void*)global_arcom_ess_instance,(void*)arcom_ess_interface_handle,i);
		*/
		Handle_Map_List[i].Newmark_Instance_Handle = global_newmark_instance;
		Handle_Map_List[i].ArcomESS_Instance_Handle = global_arcom_ess_instance;
		Handle_Map_List[i].Interface_Handle = arcom_ess_interface_handle;
	}
	return TRUE;
}

/**
 * Routine to delete a mapping from the Newmark instance instance to the opened Arcom ESS Interface Handle
 * interface_handle, in the Handle_Map_List.
 * @param newmark_instance The Newmark instance to remove from the list.
 * @return The routine returns TRUE if the map is deleted (or updated), FALSE if the mapping could not be found
 *         in the mapping list.
 *         Newmark_Throw_Exception_String is used to throw a Java exception if the routine returns FALSE.
 * @see #HANDLE_MAP_SIZE
 * @see #Handle_Map_List
 * @see #Newmark_Throw_Exception_String
 */
static int Newmark_Handle_Map_Delete(JNIEnv *env,jobject newmark_instance)
{
	int i,done;

  	/* does the map already exist? */
	i = 0;
	done = FALSE;
	while((i < HANDLE_MAP_SIZE)&&(done == FALSE))
	{
		if((*env)->IsSameObject(env,Handle_Map_List[i].Newmark_Instance_Handle,newmark_instance))
			done = TRUE;
		else
			i++;
	}
	if(done == FALSE)
	{
		Newmark_Throw_Exception_String(env,newmark_instance,"Newmark_Handle_Map_Delete",
						  "Failed to find Newmark instance in handle map.");
		return FALSE;
	}
	/* found an existing interface handle for this Newmark instance at index i */
	/* delete this map at index i */
	/*
	fprintf(stdout,"Df1Library_Handle_Map_Delete:Deleting newmark instance %p, arcom_ess_instance %pwith handle %p at map index %d.\n",
		(void*)Handle_Map_List[i].Df1Library_Instance_Handle,(void*)Handle_Map_List[i].ArcomESS_Instance_Handle(void*)Handle_Map_List[i].Interface_Handle,i);
	*/
	(*env)->DeleteGlobalRef(env,Handle_Map_List[i].Newmark_Instance_Handle);
	(*env)->DeleteGlobalRef(env,Handle_Map_List[i].ArcomESS_Instance_Handle);
	Handle_Map_List[i].Newmark_Instance_Handle = NULL;
	Handle_Map_List[i].ArcomESS_Instance_Handle = NULL;
	Handle_Map_List[i].Interface_Handle = NULL;
	return TRUE;
}

/**
 * Routine to find a mapping from the Newmark instance instance to the opened Arcom ESS Interface Handle
 * interface_handle, in the Handle_Map_List.
 * @param newmark_instance The Newmark instance.
 * @param interface_handle The address of an interface handle, to fill with the interface handle for
 *        this Newmark instance, if one is successfully found.
 * @return The routine returns TRUE if the mapping is found and returned,, FALSE if there was no mapping
 *         for this Newmark instance, or the interface_handle pointer was NULL.
 *         Newmark_Throw_Exception_String is used to throw a Java exception if the routine returns FALSE.
 * @see #HANDLE_MAP_SIZE
 * @see #Handle_Map_List
 * @see #Newmark_Throw_Exception_String
 */
static int Newmark_Handle_Map_Find(JNIEnv *env,jobject newmark_instance,
				   Arcom_ESS_Interface_Handle_T** interface_handle)
{
	int i,done;

	if(interface_handle == NULL)
	{
		Newmark_Throw_Exception_String(env,newmark_instance,
					       "Newmark_Handle_Map_Find","interface handle was NULL.");
		return FALSE;
	}
	i = 0;
	done = FALSE;
	while((i < HANDLE_MAP_SIZE)&&(done == FALSE))
	{
		if((*env)->IsSameObject(env,Handle_Map_List[i].Newmark_Instance_Handle,newmark_instance))
			done = TRUE;
		else
			i++;
	}
	if(done == FALSE)
	{
		/*fprintf(stdout,"Newmark_Handle_Map_Find:Failed to find instance %p.\n",(void*)instance);*/
		Newmark_Throw_Exception_String(env,newmark_instance,"Newmark_Handle_Map_Find",
						  "Newmark instance handle was not found.");
		return FALSE;
	}
	(*interface_handle) = Handle_Map_List[i].Interface_Handle;
	return TRUE;
}
/*
** $Log: not supported by cvs2svn $
** Revision 1.1  2008/11/20 11:35:45  cjm
** Initial revision
**
*/

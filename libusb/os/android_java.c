/*
* Android interop for libusb
* Copyright (c) 2012 Roman Donchenko <roman.donchenko@itseez.com>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <stdbool.h>
#include <jni.h>

#include "android_java.h"
#include "libusbi.h"

static JavaVM *java_vm = NULL;

static jclass java_lang_Throwable = NULL;
static jmethodID java_lang_Throwable_toString;

static jclass android_hardware_usb_UsbDeviceConnection = NULL;
static jmethodID android_hardware_usb_UsbDeviceConnection_getFileDescriptor;
static jmethodID android_hardware_usb_UsbDeviceConnection_close;

static jclass org_libusb_UsbHelper = NULL;
static jmethodID org_libusb_UsbHelper_openDevice;

static void log_exception(JNIEnv * env, jthrowable exception, struct libusb_context *ctx, const char * function)
{
	jstring exc_string = (*env)->CallObjectMethod(env, exception, java_lang_Throwable_toString);

	if ((*env)->ExceptionCheck(env)) {
		(*env)->ExceptionClear(env);
		usbi_log(ctx, LOG_LEVEL_ERROR, function, "a Java exception occurred, but toString() failed");
	} else if (!exc_string) {
		usbi_log(ctx, LOG_LEVEL_ERROR, function, "a Java exception occurred, but toString() is null");				
	} else {
		const char * exc_string_chars = (*env)->GetStringUTFChars(env, exc_string, NULL);
		usbi_log(ctx, LOG_LEVEL_ERROR, function, "a Java exception occurred: %s", exc_string_chars);				
		(*env)->ReleaseStringUTFChars(env, exc_string, exc_string_chars);
	}
}

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void * reserved)
{
	java_vm = vm;
	usbi_dbg("loaded into JVM");

	JNIEnv * env;
	void * void_env;
	(*java_vm)->GetEnv(java_vm, &void_env, JNI_VERSION_1_6);

	env = void_env;

	jclass clazz;

	clazz = (*env)->FindClass(env, "java/lang/Throwable");
	if (!clazz) (*env)->FatalError(env, "Couldn't find java.lang.Throwable!");
	java_lang_Throwable = (*env)->NewGlobalRef(env, clazz);

	java_lang_Throwable_toString = (*env)->GetMethodID(env, java_lang_Throwable,
		"toString", "()Ljava/lang/String;");
	if (!java_lang_Throwable_toString) (*env)->FatalError(env, "Couldn't find java.lang.Throwable.toString()!");

	clazz = (*env)->FindClass(env, "android/hardware/usb/UsbDeviceConnection");
	if (!clazz) goto cleanup;
	android_hardware_usb_UsbDeviceConnection = (*env)->NewGlobalRef(env, clazz);

	android_hardware_usb_UsbDeviceConnection_getFileDescriptor = (*env)->GetMethodID(env, android_hardware_usb_UsbDeviceConnection,
		"getFileDescriptor", "()I");
	if (!android_hardware_usb_UsbDeviceConnection_getFileDescriptor) goto cleanup;

	android_hardware_usb_UsbDeviceConnection_close = (*env)->GetMethodID(env, android_hardware_usb_UsbDeviceConnection,
		"close", "()V");
	if (!android_hardware_usb_UsbDeviceConnection_close) goto cleanup;

	clazz = (*env)->FindClass(env, "org/libusb/UsbHelper");
	if (!clazz) goto cleanup;
	org_libusb_UsbHelper = (*env)->NewGlobalRef(env, clazz);

	org_libusb_UsbHelper_openDevice = (*env)->GetStaticMethodID(env, org_libusb_UsbHelper,
		"openDevice", "(Ljava/lang/String;)Landroid/hardware/usb/UsbDeviceConnection;");
	if (!org_libusb_UsbHelper_openDevice) goto cleanup;

	usbi_dbg("found all Java classes and IDs");

cleanup:;

	jthrowable exception;
	if ((exception = (*env)->ExceptionOccurred(env))) {
		(*env)->ExceptionClear(env);
		log_exception(env, exception, NULL, __func__);
		java_vm = NULL;
	}

	return JNI_VERSION_1_6;
}

JNIEXPORT void JNI_OnUnload(JavaVM* vm, void* reserved)
{
	JNIEnv * env;
	void * void_env;
	(*java_vm)->GetEnv(java_vm, &void_env, JNI_VERSION_1_6);

	env = void_env;

	if (java_lang_Throwable)
		(*env)->DeleteGlobalRef(env, java_lang_Throwable);
	if (android_hardware_usb_UsbDeviceConnection)
		(*env)->DeleteGlobalRef(env, android_hardware_usb_UsbDeviceConnection);
	if (org_libusb_UsbHelper)
		(*env)->DeleteGlobalRef(env, org_libusb_UsbHelper);

	usbi_dbg("unloaded from JVM");
}

void android_java_open(struct libusb_context * ctx, const char * path, int * fd, jobject * connection)
{
	*fd = -1;
	*connection = NULL;
	bool success = false;

	if (!java_vm) return;

	JNIEnv * env;
	void * void_env;
	bool had_to_attach = false;
	jint status = (*java_vm)->GetEnv(java_vm, &void_env, JNI_VERSION_1_6);

	if (status == JNI_EDETACHED) {
		(*java_vm)->AttachCurrentThread(java_vm, &env, NULL);
		had_to_attach = true;
	} else {
		env = void_env;
	}

	jstring path_string = (*env)->NewStringUTF(env, path);

	jobject connection_local = (*env)->CallStaticObjectMethod(env, org_libusb_UsbHelper, org_libusb_UsbHelper_openDevice, path_string);
	if ((*env)->ExceptionCheck(env)) goto cleanup;

	if (!connection_local) {
		usbi_err(ctx, "couldn't open device via Java API");
		goto cleanup;
	}

	*connection = (*env)->NewGlobalRef(env, connection_local);

	*fd = (*env)->CallIntMethod(env, *connection, android_hardware_usb_UsbDeviceConnection_getFileDescriptor);
	if ((*env)->ExceptionCheck(env)) goto cleanup;

	success = true;

cleanup:

	if (!success) {
		jthrowable exception;
		if ((exception = (*env)->ExceptionOccurred(env))) {
			(*env)->ExceptionClear(env);
			log_exception(env, exception, ctx, __func__);
		}

		*fd = -1;
		if (*connection) {
			(*env)->DeleteGlobalRef(env, connection);
			*connection = NULL;
		}
	}

	if (had_to_attach) {
		(*java_vm)->DetachCurrentThread(java_vm);
	}
}

void android_java_close(struct libusb_context * ctx, jobject connection)
{
	if (!java_vm) return;

	JNIEnv * env;
	void * void_env;
	bool had_to_attach = false;
	jint status = (*java_vm)->GetEnv(java_vm, &void_env, JNI_VERSION_1_6);

	if (status == JNI_EDETACHED) {
		(*java_vm)->AttachCurrentThread(java_vm, &env, NULL);
		had_to_attach = true;
	} else {
		env = void_env;
	}

	(*env)->CallVoidMethod(env, connection, android_hardware_usb_UsbDeviceConnection_close);

	jthrowable exception;
	if ((exception = (*env)->ExceptionOccurred(env))) {
		(*env)->ExceptionClear(env);
		log_exception(env, exception, NULL, __func__);
	}

	(*env)->DeleteGlobalRef(env, connection);

	if (had_to_attach) {
		(*java_vm)->DetachCurrentThread(java_vm);
	}
}


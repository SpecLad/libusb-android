#ifndef ANDROID_JAVA_H
#define ANDROID_JAVA_H

#include <jni.h>

#include "libusbi.h"

void android_java_open(struct libusb_context * ctx, const char * path, int * fd, jobject * connection);
void android_java_close(struct libusb_context * ctx, jobject connection);

#endif


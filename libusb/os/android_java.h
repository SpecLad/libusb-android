/*
* Android interop for libusb (declarations)
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

#ifndef ANDROID_JAVA_H
#define ANDROID_JAVA_H

#include <jni.h>

#include "libusbi.h"

void android_java_open(struct libusb_context * ctx, const char * path, int * fd, jobject * connection);
void android_java_close(struct libusb_context * ctx, jobject connection);

#endif


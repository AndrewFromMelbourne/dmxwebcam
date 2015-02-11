//-------------------------------------------------------------------------
//
// The MIT License (MIT)
//
// Copyright (c) 2015 Andrew Duncan
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//-------------------------------------------------------------------------

#ifndef YUV420_IMAGE_H
#define YUV420_IMAGE_H

//-------------------------------------------------------------------------

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "bcm_host.h"
    
//-------------------------------------------------------------------------

typedef struct
{
    int32_t width;
    int32_t height;
    int32_t pitch;
    int32_t alignedHeight;
    uint16_t bitsPerPixel;
    uint32_t size;
    uint8_t *buffer;
}
YUV420_IMAGE_T;

//-------------------------------------------------------------------------

bool
initYUV420Image(
    YUV420_IMAGE_T *image,
    int32_t width,
    int32_t height);

void
clearYUV420Image(
    YUV420_IMAGE_T *image);

void
yuyvToYUV420Image(
    uint8_t *yuyv,
    int32_t width,
    int32_t height,
    YUV420_IMAGE_T *image);

void
destroyYUV420Image(
    YUV420_IMAGE_T *image);

//-------------------------------------------------------------------------

#endif

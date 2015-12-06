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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#include "bcm_host.h"
#pragma GCC diagnostic pop

#include "yuv420Image.h"

//-------------------------------------------------------------------------

#ifndef ALIGN_TO_16
#define ALIGN_TO_16(x)  ((x + 15) & ~15)
#endif

#ifndef ALIGN_TO_32
#define ALIGN_TO_32(x)  ((x + 31) & ~31)
#endif

//-------------------------------------------------------------------------

bool
initYUV420Image(
    YUV420_IMAGE_T *image,
    int32_t width,
    int32_t height)
{
    image->bitsPerPixel = 12;
    image->width = width;
    image->height = height;
    image->pitch = ALIGN_TO_32(width);
    image->alignedHeight = ALIGN_TO_16(height);
    image->size = (3 * image->pitch * image->alignedHeight) / 2;

    image->buffer = calloc(1, image->size);

    if (image->buffer == NULL)
    {
        return false;
    }

    return true;
}

//-------------------------------------------------------------------------

void
clearYUV420ImageIndexed(
    YUV420_IMAGE_T *image,
    int8_t index)
{
    memset(image->buffer, 0, image->size);
}

//-------------------------------------------------------------------------
void
yuyvToYUV420Image(
    uint8_t *yuyv,
    int32_t width,
    int32_t height,
    YUV420_IMAGE_T *image)
{
    int32_t halfPitch = image->pitch / 2;
    int32_t uOffset = image->pitch * image->alignedHeight;
    int32_t vOffset = uOffset + halfPitch * (image->alignedHeight / 2);

    int32_t j;
    for (j = 0 ; j < height ; j++)
    {
        int32_t halfJ = j / 2;

        int32_t i;
        for (i = 0 ; i < width ; i += 2)
        {
            size_t offset = 2 * (i + (j * width));

            uint8_t y1 = yuyv[offset];
            uint8_t y2 = yuyv[offset + 2];

            int32_t yIndex = j * image->pitch + i;

            image->buffer[yIndex] = y1;
            image->buffer[yIndex + 1] = y2;

            if ((j % 2) == 0)
            {
                int32_t uvIndex = halfJ * halfPitch + (i / 2);

                image->buffer[uOffset + uvIndex] = yuyv[offset + 1];
                image->buffer[vOffset + uvIndex] = yuyv[offset + 3];
            }
        }
    }
}

//-------------------------------------------------------------------------

void
destroyYUV420Image(
    YUV420_IMAGE_T *image)
{
    if (image->buffer)
    {
        free(image->buffer);
    }

    image->width = 0;
    image->height = 0;
    image->pitch = 0;
    image->alignedHeight = 0;
    image->bitsPerPixel = 0;
    image->size = 0;
    image->buffer = NULL;
}


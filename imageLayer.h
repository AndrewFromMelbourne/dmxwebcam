//-------------------------------------------------------------------------
//
// The MIT License (MIT)
//
// Copyright (c) 2020 Andrew Duncan
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

#ifndef IMAGE_LAYER_H
#define IMAGE_LAYER_H

#include "image.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#include "bcm_host.h"
#pragma GCC diagnostic pop

//-------------------------------------------------------------------------

typedef struct
{
    IMAGE_T image;
    VC_RECT_T bmpRect;
    VC_RECT_T srcRect;
    VC_RECT_T dstRect;
    int32_t layer;
    DISPMANX_RESOURCE_HANDLE_T frontResource;
    DISPMANX_RESOURCE_HANDLE_T backResource;
    DISPMANX_ELEMENT_HANDLE_T element;
} IMAGE_LAYER_T;

//-------------------------------------------------------------------------

bool
initImageLayer(
    IMAGE_LAYER_T *il,
    int32_t width,
    int32_t height,
    VC_IMAGE_TYPE_T type);

VC_IMAGE_TYPE_T
imageLayerType(
    IMAGE_LAYER_T *il);

void
createResourceImageLayer(
    IMAGE_LAYER_T *il,
    int32_t layer);

void
addElementImageLayerOffset(
    IMAGE_LAYER_T *il,
    int32_t xOffset,
    int32_t yOffset,
    DISPMANX_DISPLAY_HANDLE_T display,
    DISPMANX_UPDATE_HANDLE_T update);

void
addElementImageLayerCentered(
    IMAGE_LAYER_T *il,
    DISPMANX_MODEINFO_T *info,
    DISPMANX_DISPLAY_HANDLE_T display,
    DISPMANX_UPDATE_HANDLE_T update);

void
addElementImageLayerFullScreen(
    IMAGE_LAYER_T *il,
    DISPMANX_MODEINFO_T *info,
    DISPMANX_DISPLAY_HANDLE_T display,
    DISPMANX_UPDATE_HANDLE_T update);

void
addElementImageLayerStretch(
    IMAGE_LAYER_T *il,
    DISPMANX_MODEINFO_T *info,
    DISPMANX_DISPLAY_HANDLE_T display,
    DISPMANX_UPDATE_HANDLE_T update);

void
addElementImageLayer(
    IMAGE_LAYER_T *il,
    DISPMANX_DISPLAY_HANDLE_T display,
    DISPMANX_UPDATE_HANDLE_T update);

void
yuyvToYUV420ImageLayer(
    uint8_t *yuyv,
    int32_t width,
    int32_t height,
    IMAGE_LAYER_T *il);

bool
jpegToRGB888ImageLayer(
    uint8_t *jpeg,
    size_t length,
    int32_t width,
    int32_t height,
    IMAGE_LAYER_T *il);

void
changeSourceImageLayer(
    IMAGE_LAYER_T *il,
    DISPMANX_UPDATE_HANDLE_T update);

void
changeSourceAndUpdateImageLayer(
    IMAGE_LAYER_T *il);

void destroyImageLayer(IMAGE_LAYER_T *il);

//-------------------------------------------------------------------------

#endif

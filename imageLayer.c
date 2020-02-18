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

#include <assert.h>
#include <stdbool.h>

#include "element_change.h"
#include "image.h"
#include "imageLayer.h"

//-------------------------------------------------------------------------

#ifndef ALIGN_TO_16
#define ALIGN_TO_16(x)  ((x + 15) & ~15)
#endif

//-------------------------------------------------------------------------

bool
initImageLayer(
    IMAGE_LAYER_T *il,
    int32_t width,
    int32_t height,
    VC_IMAGE_TYPE_T type)
{
    return initImage(&(il->image), type, width, height);
}

//-------------------------------------------------------------------------

VC_IMAGE_TYPE_T
imageLayerType(
    IMAGE_LAYER_T *il)
{
    return il->image.type;
}

//-------------------------------------------------------------------------

void
createResourceImageLayer(
    IMAGE_LAYER_T *il,
    int32_t layer)
{
    uint32_t vc_image_ptr;

    il->layer = layer;

    il->frontResource =
        vc_dispmanx_resource_create(
            il->image.type,
            il->image.width | (il->image.pitch << 16),
            il->image.height | (il->image.alignedHeight << 16),
            &vc_image_ptr);
    assert(il->frontResource != 0);

    il->backResource =
        vc_dispmanx_resource_create(
            il->image.type,
            il->image.width | (il->image.pitch << 16),
            il->image.height | (il->image.alignedHeight << 16),
            &vc_image_ptr);
    assert(il->backResource != 0);

    //---------------------------------------------------------------------

    switch (il->image.type)
    {
    case VC_IMAGE_RGB888:


        vc_dispmanx_rect_set(&(il->bmpRect),
                             0,
                             0,
                             il->image.width,
                             il->image.height);

        break;

    case VC_IMAGE_YUV420:

        vc_dispmanx_rect_set(&(il->bmpRect),
                             0,
                             0,
                             il->image.width,
                             (3 * il->image.alignedHeight) / 2);

        break;

    default:

        break;
    }

    vc_dispmanx_resource_write_data(il->frontResource,
                                    il->image.type,
                                    il->image.pitch,
                                    il->image.buffer,
                                    &(il->bmpRect));
}

//-------------------------------------------------------------------------

void
addElementImageLayerOffset(
    IMAGE_LAYER_T *il,
    int32_t xOffset,
    int32_t yOffset,
    DISPMANX_DISPLAY_HANDLE_T display,
    DISPMANX_UPDATE_HANDLE_T update)
{
    vc_dispmanx_rect_set(&(il->srcRect),
                         0 << 16,
                         0 << 16,
                         il->image.width << 16,
                         il->image.height << 16);

    vc_dispmanx_rect_set(&(il->dstRect),
                         xOffset,
                         yOffset,
                         il->image.width,
                         il->image.height);

    addElementImageLayer(il, display, update);
}

//-------------------------------------------------------------------------

void
addElementImageLayerCentered(
    IMAGE_LAYER_T *il,
    DISPMANX_MODEINFO_T *info,
    DISPMANX_DISPLAY_HANDLE_T display,
    DISPMANX_UPDATE_HANDLE_T update)
{
    vc_dispmanx_rect_set(&(il->srcRect),
                         0 << 16,
                         0 << 16,
                         il->image.width << 16,
                         il->image.height << 16);

    vc_dispmanx_rect_set(&(il->dstRect),
                         (info->width - il->image.width) / 2,
                         (info->height - il->image.height) / 2,
                         il->image.width,
                         il->image.height);

    addElementImageLayer(il, display, update);
}

//-------------------------------------------------------------------------

void
addElementImageLayerFullScreen(
    IMAGE_LAYER_T *il,
    DISPMANX_MODEINFO_T *info,
    DISPMANX_DISPLAY_HANDLE_T display,
    DISPMANX_UPDATE_HANDLE_T update)
{
    int32_t dstHeight;
    int32_t dstWidth;

    if (info->width > info->height)
    {
        dstHeight = info->height;
        dstWidth = (info->height * il->image.width) / il->image.height;
    }
    else
    {
        dstWidth = info->width;
        dstHeight = (info->width * il->image.height) / il->image.width;
    }

    vc_dispmanx_rect_set(&(il->srcRect),
                         0 << 16,
                         0 << 16,
                         il->image.width << 16,
                         il->image.height << 16);

    vc_dispmanx_rect_set(&(il->dstRect),
                         (info->width - dstWidth) / 2,
                         (info->height - dstHeight) / 2,
                         dstWidth,
                         dstHeight);

    addElementImageLayer(il, display, update);
}

//-------------------------------------------------------------------------

void
addElementImageLayerStretch(
    IMAGE_LAYER_T *il,
    DISPMANX_MODEINFO_T *info,
    DISPMANX_DISPLAY_HANDLE_T display,
    DISPMANX_UPDATE_HANDLE_T update)
{
    vc_dispmanx_rect_set(&(il->srcRect),
                         0 << 16,
                         0 << 16,
                         il->image.width << 16,
                         il->image.height << 16);

    vc_dispmanx_rect_set(&(il->dstRect),
                         0,
                         0,
                         info->width,
                         info->height);

    addElementImageLayer(il, display, update);
}

//-------------------------------------------------------------------------

void
addElementImageLayer(
    IMAGE_LAYER_T *il,
    DISPMANX_DISPLAY_HANDLE_T display,
    DISPMANX_UPDATE_HANDLE_T update)
{
    VC_DISPMANX_ALPHA_T alpha =
    {
        DISPMANX_FLAGS_ALPHA_FROM_SOURCE, 
        255, /*alpha 0->255*/
        0
    };

    //---------------------------------------------------------------------

    il->element =
        vc_dispmanx_element_add(update,
                                display,
                                il->layer,
                                &(il->dstRect),
                                il->frontResource,
                                &(il->srcRect),
                                DISPMANX_PROTECTION_NONE,
                                &alpha,
                                NULL, // clamp
                                DISPMANX_NO_ROTATE);
    assert(il->element != 0);
}

//-------------------------------------------------------------------------

void
changeSourceImageLayer(
    IMAGE_LAYER_T *il,
    DISPMANX_UPDATE_HANDLE_T update)
{
    vc_dispmanx_resource_write_data(il->backResource,
                                    il->image.type,
                                    il->image.pitch,
                                    il->image.buffer,
                                    &(il->bmpRect));

    vc_dispmanx_element_change_source(update,
                                      il->element,
                                      il->backResource);

    DISPMANX_RESOURCE_HANDLE_T tmp = il->frontResource;
    il->frontResource = il->backResource;
    il->backResource = tmp;
}

//-------------------------------------------------------------------------

void
yuyvToYUV420ImageLayer(
    uint8_t *yuyv,
    int32_t width,
    int32_t height,
    IMAGE_LAYER_T *il)
{
    yuyvToYUV420Image(yuyv, width, height, &(il->image));
}

//-------------------------------------------------------------------------

void
jpegToRGB888ImageLayer(
    uint8_t *jpeg,
    size_t length,
    int32_t width,
    int32_t height,
    IMAGE_LAYER_T *il)
{
    jpegToRGB888Image(jpeg, length, width, height, &(il->image));
}

//-------------------------------------------------------------------------

void
changeSourceAndUpdateImageLayer(
    IMAGE_LAYER_T *il)
{
    vc_dispmanx_resource_write_data(il->backResource,
                                    il->image.type,
                                    il->image.pitch,
                                    il->image.buffer,
                                    &(il->bmpRect));

    DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);

    vc_dispmanx_element_change_source(update,
                                      il->element,
                                      il->backResource);

    vc_dispmanx_update_submit_sync(update);

    DISPMANX_RESOURCE_HANDLE_T tmp = il->frontResource;
    il->frontResource = il->backResource;
    il->backResource = tmp;
}

//-------------------------------------------------------------------------

void
destroyImageLayer(
    IMAGE_LAYER_T *il)
{
    DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);
    vc_dispmanx_element_remove(update, il->element);
    vc_dispmanx_update_submit_sync(update);

    //---------------------------------------------------------------------

    vc_dispmanx_resource_delete(il->frontResource);
    vc_dispmanx_resource_delete(il->backResource);

    //---------------------------------------------------------------------

    destroyImage(&(il->image));
}


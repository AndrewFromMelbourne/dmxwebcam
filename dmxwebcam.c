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

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <bsd/libutil.h>

#include <linux/videodev2.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#include "bcm_host.h"
#pragma GCC diagnostic pop

#include "backgroundLayer.h"
#include "syslogUtilities.h"
#include "yuv420Image.h"
#include "yuv420ImageLayer.h"

//-------------------------------------------------------------------------

#define DEFAULT_VIDEO_DEVICE "/dev/video0"
#define DEFAULT_DISPLAY_NUMBER 0
#define DEFAULT_FPS 30
#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480
#define NUMBER_OF_BUFFERS_TO_REQUEST 4

//-------------------------------------------------------------------------

typedef struct
{
    size_t length;
    uint8_t *buffer;
}
VIDEO_BUFFER_T;

//-------------------------------------------------------------------------

volatile bool run = true;

//-------------------------------------------------------------------------

void
printUsage(
    FILE *fp,
    const char *name)
{
    fprintf(fp, "\n");
    fprintf(fp, "Usage: %s <options>\n", name);
    fprintf(fp, "\n");
    fprintf(fp, "    --daemon - start in the background as a daemon\n");
    fprintf(fp, "    --display <number> - Raspberry Pi display number");
    fprintf(fp, " (default %d)\n", DEFAULT_DISPLAY_NUMBER);
    fprintf(fp, "    --fps <fps> - set desired frames per second");
    fprintf(fp, " (default %d frames per second)\n", DEFAULT_FPS);
    fprintf(fp, "    --fullscreen - show full screen\n");
    fprintf(fp, "    --stretch - show full screen and stretch\n");
    fprintf(fp, "    --pidfile <pidfile> - create and lock PID file");
    fprintf(fp, " (if being run as a daemon)\n");
    fprintf(fp, "    --sample <value> - only display every value frame)\n");
    fprintf(fp, "    --width <width> - set video width");
    fprintf(fp, " (default %d)\n", DEFAULT_WIDTH);
    fprintf(fp, "    --height <height> - set video height");
    fprintf(fp, " (default %d)\n", DEFAULT_HEIGHT);
    fprintf(fp, "    --videodevice <device> - video device for webcam");
    fprintf(fp, " (default %s)\n", DEFAULT_VIDEO_DEVICE);
    fprintf(fp, "    --help - print usage and exit\n");
    fprintf(fp, "\n");
}

//-------------------------------------------------------------------------

static void
signalHandler(
    int signalNumber)
{
    switch (signalNumber)
    {
    case SIGINT:
    case SIGTERM:

        run = false;
        break;
    };
}

//-------------------------------------------------------------------------

bool
initVideo(
    bool isDaemon,
    const char *program,
    int fd,
    int *width,
    int *height, 
    int fps)
{
    //---------------------------------------------------------------------

    struct v4l2_capability cap;

    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1)
    {
        perrorLog(isDaemon,
                  program,
                 "could not query V4L2 device");

        return false;
    }

    if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0)
    {
        messageLog(isDaemon,
                   program,
                   LOG_ERR,
                   "no video capture device found");

        return false;
    }

    if ((cap.capabilities & V4L2_CAP_STREAMING) == 0)
    {
        messageLog(isDaemon,
                   program,
                   LOG_ERR,
                   "no video streaming device found");

        return false;
    }

    //---------------------------------------------------------------------

    struct v4l2_format fmt;

    memset(&fmt, 0, sizeof(fmt));

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = *width;
    fmt.fmt.pix.height = *height;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == -1)
    {
        perrorLog(isDaemon,
                  program,
                 "could not get required format from video device");

        return false;
    }

    if ((fmt.fmt.pix.width != *width) || (fmt.fmt.pix.height != *height))
    {
        
        messageLog(
            isDaemon,
            program,
            LOG_INFO,
            "video dimension %dx%d -> %dx%d",
            *width,
            *height,
            fmt.fmt.pix.width,
            fmt.fmt.pix.height);

        *width = fmt.fmt.pix.width;
        *height = fmt.fmt.pix.height;
    }

    //---------------------------------------------------------------------

    if (fps > 0)
    {
        struct v4l2_streamparm streamparm;

        memset(&streamparm, 0, sizeof(streamparm));

        streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        streamparm.parm.capture.timeperframe.numerator = 1;
        streamparm.parm.capture.timeperframe.denominator = fps;

        if (ioctl(fd, VIDIOC_S_PARM, &streamparm) == -1)
        {
            messageLog(isDaemon,
                       program,
                       LOG_ERR,
                       "unable to set requested fps (%d)", fps);
        }
    }

    return true;
}

//-------------------------------------------------------------------------

int
main(
    int argc,
    char *argv[])
{
    const char *program = basename(argv[0]);

    int width = DEFAULT_WIDTH;
    int height = DEFAULT_HEIGHT;

    int fps = DEFAULT_FPS;
    suseconds_t frameDuration =  1000000 / fps;

    uint8_t sample = 1;

    bool isDaemon =  false;
    const char *pidfile = NULL;

    bool fullscreen = false;

    bool stretch = false;

    const char *vdevice = DEFAULT_VIDEO_DEVICE;

    uint32_t displayNumber = DEFAULT_DISPLAY_NUMBER;

    //---------------------------------------------------------------------

    static const char *sopts = "dD:f:FhH:p:s:v:W:";
    static struct option lopts[] = 
    {
        { "daemon", no_argument, NULL, 'd' },
        { "display", required_argument, NULL, 'D' },
        { "fps", required_argument, NULL, 'f' },
        { "fullscreen", no_argument, NULL, 'F' },
        { "height", required_argument, NULL, 'H' },
        { "help", no_argument, NULL, 'h' },
        { "pidfile", required_argument, NULL, 'p' },
        { "sample", required_argument, NULL, 's' },
        { "stretch", required_argument, NULL, 'r' },
        { "videodevice", required_argument, NULL, 'v' },
        { "width", required_argument, NULL, 'W' },
        { NULL, no_argument, NULL, 0 }
    };

    int opt = 0;

    while ((opt = getopt_long(argc, argv, sopts, lopts, NULL)) != -1)
    {
        switch (opt)
        {
        case 'd':

            isDaemon = true;
            break;

        case 'f':

            fps = atoi(optarg);

            if (fps > 0)
            {
                frameDuration = 1000000 / fps;
            }
            else
            {
                fps = 1000000 / frameDuration;
            }

            break;

        case 'F':

            fullscreen = true;

            break;

        case 'h':

            printUsage(stdout, program);
            exit(EXIT_SUCCESS);

            break;

        case 'p':

            pidfile = optarg;

            break;

        case 's':

            sample = atoi(optarg);

            if (sample < 1)
            {
                sample = 1;
            }

            break;

	case 'r':

            stretch = true;

            break;

        case 'v':

            vdevice = optarg;

            break;

        case 'D':

            displayNumber = atoi(optarg);

            break;

        case 'H':

            height = atoi(optarg);

            break;

        case 'W':

            width = atoi(optarg);

            break;

        default:

            printUsage(stderr, program);
            exit(EXIT_FAILURE);

            break;
        }
    }

    //---------------------------------------------------------------------

    struct pidfh *pfh = NULL;

    if (isDaemon)
    {
        if (pidfile != NULL)
        {
            pid_t otherpid;
            pfh = pidfile_open(pidfile, 0600, &otherpid);

            if (pfh == NULL)
            {
                fprintf(stderr,
                        "%s is already running %jd\n",
                        program,
                        (intmax_t)otherpid);
                exit(EXIT_FAILURE);
            }
        }
        
        if (daemon(0, 0) == -1)
        {
            fprintf(stderr, "Cannot daemonize\n");

            exitAndRemovePidFile(EXIT_FAILURE, pfh);
        }

        if (pfh)
        {
            pidfile_write(pfh);
        }

        openlog(program, LOG_PID, LOG_USER);
    }

    //---------------------------------------------------------------------

    if (signal(SIGINT, signalHandler) == SIG_ERR)
    {
        perrorLog(isDaemon, program, "installing SIGINT signal handler");

        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    //---------------------------------------------------------------------

    if (signal(SIGTERM, signalHandler) == SIG_ERR)
    {
        perrorLog(isDaemon, program, "installing SIGTERM signal handler");

        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    //---------------------------------------------------------------------

    bcm_host_init();

    DISPMANX_DISPLAY_HANDLE_T display
        = vc_dispmanx_display_open(displayNumber);

    if (display == 0)
    {
        messageLog(isDaemon, program, LOG_ERR, "cannot open display");
        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    DISPMANX_MODEINFO_T info;

    if (vc_dispmanx_display_get_info(display, &info) != 0)
    {
        messageLog(isDaemon,
                   program,
                   LOG_ERR,
                   "cannot get display dimensions");
        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    //---------------------------------------------------------------------

    BACKGROUND_LAYER_T bg;
    initBackgroundLayer(&bg, 0x000F, 0);

    //---------------------------------------------------------------------

    int vfd = open(vdevice, O_RDWR);

    if (vfd == -1)
    {
        perrorLog(isDaemon, program, "cannot open video device");

        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    //---------------------------------------------------------------------

    if (initVideo(isDaemon, program, vfd, &width, &height, fps) == false)
    {
        close(vfd);

        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    //---------------------------------------------------------------------

    YUV420_IMAGE_LAYER_T imageLayer;

    initYUV420ImageLayer(&imageLayer, width, height);
    createResourceYUV420ImageLayer(&imageLayer, 1);

    //---------------------------------------------------------------------

    DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);

    if (update == 0)
    {
        messageLog(isDaemon,
                   program,
                   LOG_ERR,
                   "cannot start display update");
        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    addElementBackgroundLayer(&bg, display, update);

    if (fullscreen)
    {
        addElementYUV420ImageLayerFullScreen(&imageLayer,
                                             &info,
                                             display,
                                             update);
    }
    else if (stretch)
        addElementYUV420ImageLayerStretch(&imageLayer,
                                          &info,
                                          display,
                                          update);
    else
    {
        addElementYUV420ImageLayerCentered(&imageLayer,
                                           &info,
                                           display,
                                           update);
    }

    if (vc_dispmanx_update_submit_sync(update) != 0)
    {
        messageLog(isDaemon,
                   program,
                   LOG_ERR,
                   "cannot sync display update");
        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    //---------------------------------------------------------------------

    struct v4l2_requestbuffers reqbuffers;

    memset(&reqbuffers, 0, sizeof(reqbuffers));

    reqbuffers.count = NUMBER_OF_BUFFERS_TO_REQUEST;
    reqbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuffers.memory = V4L2_MEMORY_MMAP;

    if (ioctl(vfd, VIDIOC_REQBUFS, &reqbuffers) == -1)
    {
        if (errno == EINVAL)
        {
            messageLog(isDaemon,
                       program,
                       LOG_ERR,
                       "video device does not support memory mapping");
        }
        else
        {
            perrorLog(isDaemon, program, "could not request video buffers");
        }

        close(vfd);

        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    if (reqbuffers.count < 2)
    {
        messageLog(isDaemon,
                   program,
                   LOG_ERR,
                   "insufficient buffer memory on video device");
        close(vfd);

        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    size_t numberOfVideoBuffers = reqbuffers.count;
    VIDEO_BUFFER_T *videoBuffers = malloc(numberOfVideoBuffers *
                                          sizeof(VIDEO_BUFFER_T));

    size_t bufferIndex = 0;

    for (bufferIndex = 0 ;
         bufferIndex < numberOfVideoBuffers ;
         bufferIndex++)
    {
        struct v4l2_buffer buffer;

        memset(&buffer, 0, sizeof(buffer));

        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = bufferIndex;

        if (ioctl(vfd, VIDIOC_QUERYBUF, &buffer) == -1)
        {
            perrorLog(
                isDaemon,
                program,
                "could not map video buffers");

            close(vfd);

            exitAndRemovePidFile(EXIT_FAILURE, pfh);
        }

        videoBuffers[bufferIndex].length = buffer.length;
        videoBuffers[bufferIndex].buffer = mmap(NULL,
                                                buffer.length,
                                                PROT_READ | PROT_WRITE,
                                                MAP_SHARED,
                                                vfd,
                                                buffer.m.offset);

        if (videoBuffers[bufferIndex].buffer == MAP_FAILED)
        {
            perrorLog(
                isDaemon,
                program,
                "video buffer memory map failed");

            close(vfd);

            exitAndRemovePidFile(EXIT_FAILURE, pfh);
        }
    }

    //---------------------------------------------------------------------

    for (bufferIndex = 0 ;
         bufferIndex < numberOfVideoBuffers ;
         bufferIndex++)
    {
        struct v4l2_buffer buffer;

        memset(&buffer, 0, sizeof(buffer));

        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = bufferIndex;

        if (ioctl(vfd, VIDIOC_QBUF, &buffer) == -1)
        {
            perrorLog(
                isDaemon,
                program,
                "could not enqueue video buffers");

            close(vfd);

            exitAndRemovePidFile(EXIT_FAILURE, pfh);
        }
    }

    //---------------------------------------------------------------------

    enum v4l2_buf_type buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(vfd, VIDIOC_STREAMON, &buf_type) == -1)
    {
        perrorLog(
            isDaemon,
            program,
            "could not start video capture");

        close(vfd);

        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    //---------------------------------------------------------------------

    struct timeval start_time;
    struct timeval end_time;
    struct timeval elapsed_time;

    //---------------------------------------------------------------------

    uint32_t frame = 0;

    while (run)
    {
        gettimeofday(&start_time, NULL);

        //-----------------------------------------------------------------

        struct v4l2_buffer buffer;

        memset(&buffer, 0, sizeof(buffer));

        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;

        if (ioctl(vfd, VIDIOC_DQBUF, &buffer) == -1)
        {
            perrorLog(
                isDaemon,
                program,
                "could not dequeue video buffers");

            close(vfd);

            exitAndRemovePidFile(EXIT_FAILURE, pfh);
        }

        //-----------------------------------------------------------------

        if ((frame % sample) == 0)
        {
            uint8_t *yuyv = videoBuffers[buffer.index].buffer;

            yuyvToYUV420ImageLayer(yuyv, width, height, &imageLayer);
            changeSourceAndUpdateYUV420ImageLayer(&imageLayer);
        }

        ++frame;

        //-----------------------------------------------------------------

        if (ioctl(vfd, VIDIOC_QBUF, &buffer) == -1)
        {
            perrorLog(
                isDaemon,
                program,
                "could not enqueue video buffers");

            close(vfd);

            exitAndRemovePidFile(EXIT_FAILURE, pfh);
        }

        //-----------------------------------------------------------------

        gettimeofday(&end_time, NULL);
        timersub(&end_time, &start_time, &elapsed_time);

        if (elapsed_time.tv_sec == 0)
        {
            if (elapsed_time.tv_usec < frameDuration)
            {
                usleep(frameDuration -  elapsed_time.tv_usec);
            }
        }
    }

    //---------------------------------------------------------------------

    destroyBackgroundLayer(&bg);
    destroyYUV420ImageLayer(&imageLayer);

    //---------------------------------------------------------------------

    buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(vfd, VIDIOC_STREAMOFF, &buf_type) == -1)
    {
        perrorLog(
            isDaemon,
            program,
            "could not stop video capture");

        close(vfd);

        exitAndRemovePidFile(EXIT_FAILURE, pfh);
    }

    //--------------------------------------------------------------------

    for (bufferIndex = 0 ;
         bufferIndex < numberOfVideoBuffers ;
         bufferIndex++)
    {
        munmap(videoBuffers[bufferIndex].buffer,
               videoBuffers[bufferIndex].length);
    }

    free(videoBuffers);

    //--------------------------------------------------------------------

    close(vfd);

    //---------------------------------------------------------------------

    vc_dispmanx_display_close(display);

    //---------------------------------------------------------------------

    messageLog(isDaemon, program, LOG_INFO, "exiting");

    if (isDaemon)
    {
        closelog();
    }

    if (pfh)
    {
        pidfile_remove(pfh);
    }

    //---------------------------------------------------------------------

    return 0 ;
}


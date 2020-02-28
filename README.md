# dmxwebcam

# usage

	dmxwebcam <options>

	--daemon - start in the background as a daemon
	--display <number> - Raspberry Pi display number (default 0)
	--fps <fps> - set desired frames per second
	--bestfit - choose width/height based on screen size
	--fullscreen - show full screen
	--pidfile <pidfile> - create and lock PID file (if being run as a daemon)
	--sample <value> - only display every value frame)
	--width <width> - set video width (default 640)
	--height <height> - set video height (default 480)
	--videodevice <device> - video device for webcam (default /dev/video0)
	--help - print usage and exit

# build

	sudo apt-get install libbsd-dev libturbojpeg-dev
	cd dmxwebcam
	mkdir build
	cd build
	cmake ..
	make

# TODO

    * Optimize MJPEG decoding
	* Use MMAL for JPEG decode and conversion from YUYV to YUV420
	* Add other pixel formats


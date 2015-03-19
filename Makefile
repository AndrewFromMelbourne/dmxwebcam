OBJS=dmxwebcam.o backgroundLayer.o syslogUtilities.o yuv420Image.o \
	 yuv420ImageLayer.o
BIN=dmxwebcam

CFLAGS+=-Wall -g -O3
LDFLAGS+=-L/opt/vc/lib/ -lbcm_host -lbsd

INCLUDES+=-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux

all: $(BIN)

%.o: %.c
	@rm -f $@ 
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

clean:
	@rm -f $(OBJS)
	@rm -f $(BIN)

C      = gcc
CFLAGS  += -g -MD -Wall -I. -I../../include -D_FILE_OFFSET_BITS=64 $(CPPFLAGS) 
LDFLAGS  += -lc

OBJS = bitrate.o 
TARGET = bitrate
DESTDIR ?= /usr/local/bin/

all: $(TARGET)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)  -lpthread

install: all
	install -m 755 $(TARGET) $(DESTDIR) 

clean:
	rm -f $(TARGET) $(OBJS) core* *~ *.d

-include $(wildcard *.d) dummy


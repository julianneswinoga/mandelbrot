TARGET = mandelbrot
LIBS = -lm -lxcb -lpthread
CFLAGS = -g -Wall -std=c99 -pthread

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

install: $(TARGET)
	-cp $(TARGET) /usr/local/bin

clean:
	-rm -f *.o
	-rm -f $(TARGET)

TARGET=report
CFLAGS=-Wall -Werror
LDFLAGS=-Wall -Werror

all: $(TARGET)

$(TARGET): i2c.o axp209.o si7021.o report.o

report.o: config.h

config.h:
	$(warning Creating default config, which contains invalid API keys)
	$(warning Please enter your API keys in config.h, then run "make" again)
	cp config.h.example config.h

clean:
	rm -f $(TARGET) *.o

distclean: clean
	rm -f config.h

.PHONY: all debug release

debug: CFLAGS  += -g  -DDEBUG
debug: LDFLAGS += -g  -DDEBUG
debug: ${TARGET}

release: CFLAGS  += -O2
release: LDFLAGS += -O2
release: ${TARGET}

CC      = gcc
CFLAGS += -Wall -pthread -I$(INC_DIR)
INC_DIR = ./include/
TARGET  = thread_thrashing
OBJECT  = thread_thrashing.o

SRCS    = thread_thrashing.c

INCS    = $(INC_DIR)myAssert.h
INCS   += $(INC_DIR)thread_thrashing.h
INCS   += $(INC_DIR)thread_def.h

debug : $(TARGET)
release : $(TARGET)

$(TARGET): $(OBJECT)
	$(CC) $(CFLAGS) -o $@ $(OBJECT)

thread_thrashing.o : thread_thrashing.c $(INCS)
	$(CC) $(CFLAGS) -c -o $@ $(SRCS)

clean:
	rm -f $(OBJECT) $(TARGET)

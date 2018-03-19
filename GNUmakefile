CC = gcc5
TARGET = xtetcolor
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

clean:
	$(RM) *.[oasid] $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ -L/usr/local/lib -lX11

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

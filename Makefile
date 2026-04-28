CC=gcc
CFLAGS=-Wall -Wextra -Ithirdparty
LIBS=

TARGET=shush

SRC=main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: clean

CC=gcc
CFLAGS=-Wall -Ithirdparty -fsanitize=address
LIBS=-lreadline

TARGET=shush

SRC=main.c builtin.c info.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: clean

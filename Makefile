CC = gcc
CFLAGS = `pkg-config --cflags mxml libzip` -Wall -ggdb
LFLAGS = `pkg-config --libs mxml libzip`

TARGET = docer
SRC = $(wildcard src/*.c)
OBJ = $(subst src, build, $(SRC:.c=.o))


.PHONY: test clean

$(TARGET): $(OBJ)
	$(CC) $(LFLAGS) $(CFLAGS) -o $@ $(OBJ)

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c -o $@ $<


test: $(TARGET)
	rm -rf file.odt file
	./$(TARGET)
	unzip -d file file.odt

clean:
	rm -rf build $(TARGET) file file.odt

CC=gcc
LIB_DIR=./lib
INC_DIR=./include
BIN_DIR=./bin
SRC_DIR=./src

all: obj lib

obj: $(SRC_DIR)/t2fs.c
	$(CC) -c $(SRC_DIR)/t2fs.c -I$(INC_DIR)/ -Wall -m32 -g
	mv t2fs.o $(BIN_DIR)/

lib: $(BIN_DIR)/t2fs.o
	ar crs $(LIB_DIR)/libt2fs.a $(BIN_DIR)/t2fs.o $(LIB_DIR)/apidisk.o

clean:
	rm -rf $(BIN_DIR)/t2fs.o $(LIB_DIR)/libt2fs.a
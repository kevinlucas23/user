CUR_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

CC = gcc
CFLAGS += -g -O2 -Wall -pthread
LDFLAGS +=

DEPS_DIR  := $(CUR_DIR)/.deps$(LIB_SUFFIX)
DEPCFLAGS = -MD -MF $(DEPS_DIR)/$*.d -MP

SRC_FILES = $(wildcard *.c)
OBJ_FILES := user_project3.o main_p3.o

EXE_FILES = $(SRC_FILES:.c=)

%.o:%.c $(DEPS_DIR)
        $(CC) $(CFLAGS) $(DEPCFLAGS) -c $(input) -o $(output)

main_p3: $(OBJ_FILES)
        $(CC) -o $@ $(CFLAGS) $(OBJ_FILES) $(LDFLAGS)

all: $(EXE_FILES)
        echo $(EXE_FILES)

clean:
        rm -f main_p3 $(OBJ_FILES)

.PHONY: all clean
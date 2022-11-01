SRC_DIR = src
BUILD_DIR = build
CC = g++
SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)
OBJ_NAME = play
INCLUDE_PATHS = -Iinclude
LIBRARY_PATHS = -Llib

all:
	$(CC)  $(INCLUDE_PATHS) $(LIBRARY_PATHS) -o play $(SRC_FILES) -lmingw32 -lSDL2 -lSDL2main
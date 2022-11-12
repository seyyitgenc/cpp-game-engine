SRC_DIR = src
BUILD_DIR = build/debug
CC = g++ -g -Wall
SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)
OBJ_NAME = play
INCLUDE_PATHS = -Iinclude
LIBRARY_PATHS = -Llib

all:
	$(CC)  $(INCLUDE_PATHS)  $(LIBRARY_PATHS) $(SRC_FILES) include/headers/ECS/*.cpp -o $(BUILD_DIR)/$(OBJ_NAME) -lmingw32 -lSDL2 -lSDL2main -lSDL2_ttf -lSDL2_image
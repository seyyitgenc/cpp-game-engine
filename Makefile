BUILD_DIR = build/debug
CC = g++ -g -Wall
SRC_FILES = $(wildcard src/*.cpp include/ECS/Entity/*cpp include/Engine/*.cpp include/Managers/*cpp ) 
OBJ_NAME = play
INCLUDE_PATHS = -Iinclude
LIBRARY_PATHS = -Llib

all:
	$(CC)  $(INCLUDE_PATHS)  $(LIBRARY_PATHS) $(SRC_FILES) -o $(BUILD_DIR)/$(OBJ_NAME) -lmingw32 -lSDL2 -lSDL2main -lSDL2_ttf -lSDL2_image
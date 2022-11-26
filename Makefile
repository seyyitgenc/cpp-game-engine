BUILD_DIR = build
CXX = g++ -g -Wall
SRC_FILES = $(wildcard src/*.cpp include/ECS/*cpp include/ECS/Components/*cpp) 
OBJ_NAME = play
INCLUDE_PATHS = -Iinclude
LIBRARY_PATHS = -Llib

all:
	$(CXX)  $(INCLUDE_PATHS)  $(LIBRARY_PATHS) $(SRC_FILES) -o $(BUILD_DIR)/$(OBJ_NAME) -lmingw32 -lSDL2 -lSDL2_ttf -lSDL2_image
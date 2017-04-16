DEBUG = -g
EXTERN_LIB = -lpthread
DIR_INC = ./
DIR_SRC = ./
DIR_OBJ = ./
TARGET = epoll_cli

CC = g++
INCLUDE = -I${DIR_INC}
HOST_SRC = $(wildcard ${DIR_SRC}/*.cpp)
OBJ = $(patsubst %.cpp,${DIR_OBJ}%.o,$(notdir ${HOST_SRC}))

${TARGET}:${OBJ}
	$(CC) $(OBJ) -o $@ $(EXTERN_LIB) $(INCLUDE)

${DIR_OBJ}/%.o:${DIR_SRC}/%.cpp
	$(CC) $(DEBUG) -std=c++0x -c $< -o $@ $(INCLUDE)

.PHONY:clean
clean:
	rm -f ./obj/*.o ./epoll_cli

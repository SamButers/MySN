PROJECT_NAME=mysn_server

CPP_SRC=$(wildcard src/*.cpp)

HPP_SRC=$(wildcard inc/*.hpp)

OBJ=$(subst .cpp,.o,$(subst src,obj,$(CPP_SRC)))

CC=g++

CC_FLAGS = -g -I inc -c

all: objFolder $(PROJECT_NAME)

clean:
	rm -rf obj/*.o *.bin

$(PROJECT_NAME): $(OBJ)
	$(CC) -o $@.bin $^ -pthread

obj/%.o: src/%.cpp inc/%.hpp
	$(CC) -o $@ $< $(CC_FLAGS)

obj/main.o: src/main.cpp $(HPP_SRC)
	$(CC) -o $@ $< $(CC_FLAGS)

objFolder:
	mkdir -p obj
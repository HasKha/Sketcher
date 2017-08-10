CC= g++

CFLAGS= -std=c++11 -Wall

INC_DIR= -I/usr/local/include/eigen3 $(shell sdl2-config --cflags)

L_DIR= -L/usr/local/lib $(shell sdl2-config --libs)

LDFLAGS= -framework GLUT -framework OpenGL -framework Cocoa -lOpenMeshCore

BIN := Sketcher
SRC := $(wildcard *.cpp)
OBJ := $(SRC:%.cpp=build/%.o)

default: build

build: $(BIN)
	
$(BIN): build/main.o $(OBJ)
	$(CC) $(LDFLAGS) $(L_DIR) -o $@ $^
	
build/%.o: %.cpp
	@ mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INC_DIR) -c $< -o $@
	
clean:
	rm -rf *~ $(OBJ) $(BIN) build
	
run: build execute

# this allows running with this: `make run mesh=path/to/mesh`
execute:
	./$(BIN) $(mesh)

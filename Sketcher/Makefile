# Available make targets: `make`, `make clean`, `make run`
#
# Dependencies to install: glfw, GLEW, OpenMeshCore
#
# For GLEW you need to download the source, make && sudo make install
# To install GLFW you probably need cmake and then make && make install

CC= g++
CFLAGS= -std=c++11 -Wall

PROJECT_ROOT = /Users/gori/git/Sketcher

INCLUDE_DIR = \
		-I/usr/local/include \
		-I$(PROJECT_ROOT)/Dependencies/Eigen \
		-I$(PROJECT_ROOT)/Dependencies/imgui \
		-I$(PROJECT_ROOT)/Dependencies/glm \
		-I$(PROJECT_ROOT)/Dependencies/gco-v3.0 \
		-I$(PROJECT_ROOT)/Dependencies/stb \
		-I$(PROJECT_ROOT)/Dependencies/gl3w \
		-I$(PROJECT_ROOT)/Dependencies/nativefiledialog/src

LIB_DIR = -L/usr/local/lib \
		-L/opt/local/lib \
		-L$(PROJECT_ROOT)/Dependencies/nativefiledialog/build/lib/Debug/x64/ -lnfd_d \
		-lglfw -lGLEW -lOpenMeshCore


LDFLAGS= -framework GLUT -framework OpenGL -framework Cocoa -framework AppKit

BIN := Sketcher
SRC := 	$(wildcard *.cpp) \
 		$(wildcard ../Dependencies/imgui/*.cpp) \
 		../Dependencies/imgui/examples/opengl3_example/imgui_impl_glfw_gl3.cpp \
 		../Dependencies/gl3w/GL/gl3w.cpp
 		

OBJ := $(SRC:%.cpp=build/%.o)

default: build

build: $(BIN)
	
$(BIN): build/main.o $(OBJ) 
	$(CC) $(LDFLAGS) $(LIB_DIR) -o $@ $^
	
build/%.o: %.cpp
	@ mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE_DIR) -c $< -o $@

clean:
	rm -rf *~ $(OBJ) $(BIN) build Dependencies
	
run: build execute

# this allows running with this: `make run mesh=path/to/mesh`
execute:
	./$(BIN) $(mesh)

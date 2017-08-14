# Sketcher 
A starting point for geometry processing, mesh editing and visualization applications and tests.

# Download
clone the repository recursively: `git clone --recursive https://github.com/HasKha/Sketcher.git`

Do not download as zip, as the submodules won't be included and the project will not build.

# Features
* Open and view OBJ or PLY meshes
* Quickly and easily prototype new code

The following features are implemented for fast prototyping:
* Customizable render system based on faces and edges
* Raytracer to pick vertices, edges or faces
* Independent worker and renderer threads for responsive interface while your code is thinking. The worker supports a queue of operations and can notify the renderer to refresh the view so you can see the progress of your algorithms.
* On-screen console (use `print` as if it was `printf`)

# Dependencies
All dependencies are included. Some are git submodules, so **make sure to clone the repository recursively**.
* __[OpenMesh](https://www.openmesh.org/) (Core only)__: Mesh I/O, mesh datastructure, and basic mesh operations such as iterators and circulators.
* __[ImGui](https://github.com/ocornut/imgui)__: Easy and quick development of user interfaces.
* __[Eigen](http://eigen.tuxfamily.org/)__: Used for matrix datastructures, can be used for more powerful matrix-related operations.
* __[glfw](http://www.glfw.org/) / [gl3w](https://github.com/skaslev/gl3w)__: Window management and OpenGL bindings.
* __[glm](https://github.com/g-truc/glm)__: Handle camera matrices and communication with glsl.
* __[stb](https://github.com/nothings/stb)__: Saving screenshots and has useful bits if needed.

# Operating System and compatibility
## Windows
A Visual Studio project is provided and it should work out of the box.

## Unix
All the software is compatible with Unix with the exception of:
- `FileDialog.h/cpp`: you will have to replace this, or just remove and hardcode filenames
- `main.cpp`: `WinMain`->`main` 

You will need to make your own build system. 

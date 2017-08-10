# Sketcher 
A starting point for geometry processing, mesh editing and visualization applications and tests.

# Features
* Open and view OBJ or PLY meshes
* Quickly and easily prototype new code

The following features are implemented for fast prototyping:
* Customizable render system based on faces and edges
* Raytracer to pick vertices, edges or faces
* Independent worker and renderer threads for responsive interface while your code is thinking. The worker supports a queue of operations and can notify the renderer to refresh the view so you can see the progress of your algorithms.
* On-screen console (use `print` as if it was `printf`)

# Dependencies
All dependencies are included.
* __OpenMesh (Core only)__: Mesh I/O, mesh datastructure, and basic mesh operations such as iterators and circulators.
* __ImGui__: Easy and quick development of user interfaces.
* __Eigen__: Used for matrix datastructures, can be used for more powerful matrix-related operations.
* __glfw / gl3w__: Window management and OpenGL bindings.
* __glm__: Handle camera matrices and communication with glsl.
* __stb__: Saving screenshots and has useful bits if needed.

# Operating System and compatibility
A Visual Studio project is provided and it should work out of the box.

All the software is compatible with Unix with the exception of:
- `FileDialog.h/cpp`: you will have to replace this, or just remove and hardcode filenames
- `main.cpp`: `WinMain`->`main` 

You will need to make your own build system. 

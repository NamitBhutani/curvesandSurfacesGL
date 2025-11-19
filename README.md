# Curves and Surfaces GL Project

This project contains two OpenGL applications demonstrating Bezier curves and surfaces:

1. **BezierGL** - Interactive Bezier curve editor with De Casteljau algorithm visualization
2. **SceneGL** - Animated playground scene with editable Bezier slide

## Prerequisites

- CMake (version 3.10 or higher)
- OpenGL development libraries
- GLFW3
- GLM (OpenGL Mathematics)
- A C++17 compatible compiler

## Building the Projects

### Building BezierGL (Curve Editor)

```bash
cd bezier
mkdir -p build
cd build
cmake ..
make
```

This will create the `BezierGL` executable in the `bezier/build/` directory.

### Building SceneGL (Playground Scene)

```bash
cd ../..  # Return to root directory
mkdir -p build
cd build
cmake ..
make
```

This will create the `SceneGL` executable in the `build/` directory.

## Running the Applications

### Running BezierGL

```bash
cd bezier/build
./BezierGL
```

### Running SceneGL

```bash
cd build
./SceneGL
```

## Controls

### BezierGL (Bezier Curve Editor)

#### Mouse Controls

- **Left Click** - Add a new control point or select an existing point
- **Left Click + Drag** - Move a selected control point
- **Right Click** - Delete a control point

#### Keyboard Controls

- **SPACE** - Toggle De Casteljau algorithm animation on/off
- **UP Arrow** - Increase animation speed
- **DOWN Arrow** - Decrease animation speed
- **S** - Generate Surface of Revolution from the curve and save to `surface.off`
- **ESC** - Exit the application

#### Features

- Real-time Bezier curve rendering
- Interactive control point editing
- De Casteljau algorithm visualization with animated construction steps
- Color-coded intermediate points showing the subdivision process
- Surface of Revolution generation from curve profile

---

### SceneGL (Animated Playground Scene)

#### Camera Controls (Normal Mode)

- **W** - Move camera forward
- **S** - Move camera backward
- **A** - Move camera left
- **D** - Move camera right
- **SPACE** - Move camera up
- **LEFT CTRL** - Move camera down
- **Q** - Roll camera counter-clockwise
- **E** - Roll camera clockwise
- **R** - Reset camera roll
- **Mouse Movement** - Look around (first-person view)
- **Mouse Scroll** - Zoom in/out (adjust field of view)

#### Edit Mode Controls

- **TAB** - Toggle edit mode on/off
  - When enabled: Mouse cursor becomes visible for editing Bezier slide
  - When disabled: Camera control mode (cursor hidden)

#### Mouse Controls (Edit Mode Only)

- **Left Click** - Select an existing control point or add a new one
- **Left Click + Drag** - Move the selected control point
- **Right Click** - Delete a control point (minimum 2 points required)

#### Scene Controls

- **P** - Toggle playground animations (swings, merry-go-round) on/off
- **L** - Toggle lighting on/off
- **B** - Toggle wireframe/edge rendering on/off
- **O** - Load surface of revolution from `surface.off` file into the scene
- **ESC** - Exit the application

#### Scene Features

- Animated playground with swings and merry-go-round
- Editable Bezier curve slide that updates in real-time
- Trees, benches, and ground plane
- Dynamic lighting with Phong shading
- Real-time control point editing with visual feedback (yellow spheres for selected, green for unselected)

## Workflow: Creating Custom Surfaces

1. **Design your curve** in BezierGL:

   ```bash
   cd bezier/build
   ./BezierGL
   ```

   - Add control points by clicking
   - Adjust their positions by dragging
   - Preview the curve in real-time
   - Press **S** to generate and save the surface

2. **View the surface** in SceneGL:
   ```bash
   cd ../../build
   ./SceneGL
   ```
   - Press **O** to load the generated `surface.off` file
   - The surface appears in the scene with full lighting and shading
   - Use camera controls to view from different angles

## Project Structure

```
ass2-cg/
├── main.cpp              # SceneGL application source
├── camera.h              # Camera implementation
├── shader.h              # Shader loader and manager
├── mesh.h                # Mesh rendering
├── model.h               # Model loader (OFF format)
├── animator.h            # Animation controllers
├── primitives.h          # Geometric primitive generation
├── CMakeLists.txt        # Build configuration for SceneGL
├── shaders/              # Shader files for SceneGL
│   ├── shader.vert       # Vertex shader
│   └── shader.frag       # Fragment shader
├── models/               # 3D model files
│   └── scene.off         # Scene models
├── bezier/               # Bezier curve editor
│   ├── main.cpp          # BezierGL application source
│   ├── bezier1.h         # Bezier curve implementation
│   ├── CMakeLists.txt    # Build configuration for BezierGL
│   └── shaders/          # Shader files for BezierGL
│       ├── curve.vert    # Vertex shader
│       └── curve.frag    # Fragment shader
└── glad/                 # OpenGL loader
    └── include/          # GLAD headers
```

## Technical Details

### BezierGL Implementation

- Uses De Casteljau's algorithm for curve evaluation
- Supports arbitrary number of control points
- Animated visualization shows recursive subdivision process
- Surface of Revolution generation using parametric rotation

### SceneGL Implementation

- Multiple animated objects using custom animator classes
- Real-time Bezier curve editing integrated into 3D scene
- Phong lighting model with ambient, diffuse, and specular components
- Procedural geometry generation for primitives (cubes, cylinders, cones, tori)
- OFF file format support for loading generated surfaces

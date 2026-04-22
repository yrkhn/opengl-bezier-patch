# OpenGL Bezier Patch — Procedural 3D Texture

An interactive 3D graphics demo built with **C++ / OpenGL 3.3 Core**, showcasing a tessellated Bézier patch rendered with a procedural marble-like texture computed entirely in the fragment shader.

![OpenGL](https://img.shields.io/badge/OpenGL-3.3%20Core-blue)
![C++](https://img.shields.io/badge/C++-17-blue)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey)
![IDE](https://img.shields.io/badge/IDE-Visual%20Studio%202022-purple)

---

## Features

- **Bézier Patch Tessellation** — a 4×4 bicubic Bézier patch tessellated at runtime with adjustable resolution (2–64 subdivisions)
- **Procedural 3D Marble Texture** — marble veins generated in the fragment shader using FBM (Fractal Brownian Motion) noise based on world-space coordinates — no texture files needed
- **Phong Lighting** — ambient + diffuse + specular lighting with a camera-attached light source
- **Interactive Control Point Editing** — select any of the 16 control points and move them along X / Y / Z axes in real time
- **Wireframe Mode** — toggle to inspect the patch topology
- **Normal Debug Mode** — visualise surface normals as RGB colours
- **Mouse Orbit Camera** — drag to rotate, scroll to zoom

---

## Project Structure

```
Project/
├── Project.sln               # Visual Studio solution
└── Project/
    ├── main.cpp              # Entry point, render loop, Bézier patch logic
    ├── game.cpp / game.h     # Auxiliary game/scene utilities
    ├── shader.h              # Inline shader helper (compile + link)
    ├── shader_compiler.cpp/.h
    ├── shader_utils.cpp/.h
    ├── light_casters.vs/.fs  # Vertex / fragment shaders for light caster pass
    ├── light_cube.vs/.fs     # Shaders for the light source cube
    ├── stb_image.h           # Single-header image loader
    ├── container.jpg         # Texture asset
    ├── container2.png        # Diffuse map
    └── container2_specular.png  # Specular map
```

---

## Dependencies

| Library | Purpose | How to get |
|---------|---------|------------|
| **GLFW 3** | Window + input | https://www.glfw.org |
| **GLAD** | OpenGL function loader | https://glad.dav1d.de (GL 3.3 Core) |
| **GLM** | Math (vectors, matrices) | https://github.com/g-truc/glm |
| **stb_image.h** | Texture loading | Already included in the repo |

---

## Building (Windows / Visual Studio 2022)

1. **Clone the repo** (see instructions below).
2. **Install dependencies** via [vcpkg](https://github.com/microsoft/vcpkg):
   ```
   vcpkg install glfw3 glm glad
   vcpkg integrate install
   ```
   Or place the headers/libs manually in the project include/lib directories.
3. **Open** `Project.sln` in Visual Studio 2022.
4. Set configuration to **x64 / Debug** (or Release).
5. Press **F5** to build and run.

> The built executable expects `glfw3.dll`, `glew32.dll` and `freeglut.dll` next to it in the output directory — these are already present in `x64/Debug/`.

---

## Controls

| Key / Action | Effect |
|---|---|
| **Left Mouse Drag** | Orbit camera |
| **Scroll Wheel** | Zoom in / out |
| `[` / `]` | Select previous / next control point |
| `X` / `Shift+X` | Move selected control point along X |
| `Y` / `Shift+Y` | Move selected control point along Y |
| `Z` / `Shift+Z` | Move selected control point along Z |
| `+` / `-` | Increase / decrease tessellation resolution |
| `T` | Toggle wireframe mode |
| `R` | Reset patch and camera to defaults |
| `Esc` | Exit |

---

## How It Works

### Bézier Patch
The surface is a bicubic Bézier patch defined by 16 control points arranged in a 4×4 grid. At each frame the CPU evaluates the patch at a `res × res` grid of (u, v) parameters using the Bernstein basis, computes per-vertex normals by averaging face normals, and uploads the resulting VBO/EBO to the GPU.

### Procedural Marble Texture
The fragment shader computes the marble pattern using world-space position:
1. **FBM noise** — 5 octaves of a cheap `sin(dot(p, seed))` hash layered together.
2. **Vein function** — `sin((x + noise) * 2π)` creates sinusoidal veins.
3. **Colour mix** — a cream base colour and a dark brown vein colour are blended via `smoothstep`.
4. **Speckling** — high-frequency noise adds subtle dark flecks for realism.

---

## License

This project is for educational purposes. Feel free to use and modify.

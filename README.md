# C Software Rasterizer

![](https://github.com/jqyDee/software-rasterizer.c/blob/main/.github/images/rasterizer.png)


A CPU-based 3D software rasterizer written entirely from scratch in C. 

Instead of relying on hardware acceleration (OpenGL/Vulkan) to render 3D
graphics, this project implements the entire graphics pipeline on the CPU. It
handles matrix mathematics, coordinate space transformations, `.obj` file
parsing, and rasterization logic natively, using
[Raylib](https://www.raylib.com/) strictly as a host to display the final pixel
buffer in a window.

## Features

* **Custom 3D Engine**: Core rasterization module built from scratch
  (`src/rasterizer_module/`).
* **Wavefront `.obj` Parser**: Custom parser to load and render 3D models
  (includes the classic Suzanne monkey and a basic cube).
* **Vector Mathematics**: Custom implementation of 3D vector and matrix
  operations (`vec.h`).
* **Coordinate Transformations**: Handles the full 3D pipeline (Local -> World
  -> View -> Projection -> Screen space).
* **Drawing Primitives**: Custom line and triangle drawing algorithms.
* **Separation of Concerns**: Clean architectural split between the host
  window/input (`src/host/`) and the platform-agnostic rendering engine.

## Tech Stack

* **Language**: C
* **Display/Input Host**: Raylib (included as a submodule)
* **Build System**: Make

## Project Structure

```text
.
├── libs/
│   └── raylib/                  # Submodule for window/pixel hosting
├── obj/                         # 3D Model files (.obj)
│   ├── Suzanne.obj
│   └── cube.obj
├── src/
│   ├── host/                    # Platform-specific code (Window creation, input)
│   │   └── main.c
│   └── rasterizer_module/       # Platform-agnostic 3D rendering engine
│       ├── coordinates.c/.h     # Space transformations
│       ├── draw.c/.h            # Pixel pushing and primitives
│       ├── engine.c/.h          # Core rendering loop
│       ├── parser.c/.h          # .obj file parsing
│       ├── rasterizer.c         # Rasterization logic
│       ├── update.c/.h          # State updates
│       ├── types.h              # Data structures
│       └── vec.h                # Vector/Matrix math library
├── Makefile                     # Build instructions
└── generate_compile_commands.sh # Tooling setup
```

## Getting Started

### Prerequisites

* A standard C compiler (GCC or Clang)
* `make`
* Git
* (macOS: the current build script is only validated (and as far as I know only
  works) on macOS)
  > Feel free to implement cross platform building

### Installation & Build

1. **Clone the repository** (make sure to pull the Raylib submodule):
   ```bash
   git clone --recursive <your-repo-url>
   cd software-rasterizer.c
   ```
   *(If you already cloned it without the submodule, run `git submodule update
   --init --recursive`)*

2. **Build the project**:
   ```bash
   make
   ```

3. **Run the rasterizer**:
   ```bash
   ./bin/rasterizer
   ```

### Important
Currently there is an error in the Makefile. Running `make {all,run}` does not
rebuild the shared lib. This needs to be fixed in the future. Mitigate with
`make clean && make run`.

## What I Learned

Building this project taught me the fundamental mathematics and algorithms
behind 3D graphics. Specifically, I gained hands-on experience with:
* Barycentric coordinates for rasterizing triangles.
* The math behind projection matrices (translating 3D coordinates to a 2D
  screen).
* The challenges and techniques of optimizing CPU rendering without hardware
  acceleration.

## License

This project is open source and available under the [LICENSE](LICENSE) file.

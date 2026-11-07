# PlusOS 3D Engine & Game Suite (14 Games)

A high-performance 3D graphics library and complete 14-game arcade suite created for **PlusOS**, running on classic Macintosh hardware (**Macintosh Plus** and **Macintosh SE**, Motorola 68000 @ 8 MHz, 512x342 1-bit monochrome display) and standalone desktop Windows/Linux via Python.

---

## 1. 3D Graphics Library Architecture (`plus3d`)

The 3D engine is provided in both **C** (`plus3d.h`, `plus3d.c`) for native bare-metal execution on the 68000, and **Python** (`engine_3d.py`) for instantaneous desktop testing and prototyping.

### Key Components

- **Fixed-Point / Floating 3D Vector & Matrix Math**:
  - `Vec3`, `Vec2`, `Mat4` transformations (translation, rotation around X/Y/Z, non-uniform scaling, perspective projection).
- **Procedural 3D Mesh Builders**:
  - `mesh_create_cube()`
  - `mesh_create_pyramid()`
  - `mesh_create_cylinder()`
  - `mesh_create_sphere()`
  - `mesh_create_torus()`
  - `mesh_create_car()`
  - `mesh_create_spaceship()`
  - `mesh_create_asteroid()`
- **Monochrome Bayer Dithering Engine**:
  - 16 distinct $4 \times 4$ Bayer threshold patterns (`BAYER_PATTERNS`) mapped to directional lighting diffuse intensity, simulating smooth grayscale shading on a 1-bit black & white CRT.
- **Rendering Pipeline**:
  - Camera projection matrix ($60^\circ$ FOV default)
  - Back-face culling via surface normal cross-product
  - Depth sorting (Painter's algorithm)
  - Scanline polygon rasterizer with Bayer dither patterns
- **Collision & Physics**:
  - 3D Axis-Aligned Bounding Box (AABB) intersection
  - 3D Sphere-Sphere collision testing
  - Ray-Triangle intersection testing for mouse picking and targeting
- **Entity Component System (ECS)**:
  - Transform, velocity, angular velocity, mesh pointer, bounding sphere, and active state flags.

---

## 2. Complete Game Suite (14 Games)

| # | Game | Genre | Dimension | Key Features |
|---|------|-------|-----------|--------------|
| 1 | **3D Retro Racer** | Racing | 3D | Curved pseudo-3D road, elevation hills, 3D car model, AI traffic, speed & time HUD |
| 2 | **Mac Tetris** | Puzzle | 2D | 7 tetrominoes, ghost piece projection, line clearing, score & level ramps |
| 3 | **Space Invaders** | Arcade | 2D | 55 marching aliens, destructible bunkers, alien bombs, mystery flying saucer |
| 4 | **Vector Asteroids** | Physics | 2D | Inertial physics, rotational thrust, splitting asteroids, hyperspace button |
| 5 | **Pac-Man Arcade** | Maze | 2D | Classic maze, dots, energizers, 4 ghost AI states (Blinky, Pinky, Inky, Clyde) |
| 6 | **Retro Snake** | Arcade | 2D | Growing tail segments, apple spawning, speed acceleration, score display |
| 7 | **Mac Breakout** | Arcade | 2D | Paddle deflection physics, multi-tier dithered brick wall, score & lives |
| 8 | **Lunar Lander** | Sim | 2D | Gravitational physics, attitude thrusters, fuel limits, procedural terrain & pads |
| 9 | **3D Dungeon Crawler** | RPG | 3D | Real-time DDA raycasting, perspective wall heights, distance shading, minimap radar |
| 10 | **3D Starflight Shooter** | Rail Shooter | 3D | Starfield projection, wireframe interceptors, dual laser cannons, cockpit HUD |
| 11 | **Retro Flap Jumper** | Jumper | 2D | Precise jump impulses, procedural pipes, collision hitboxes, score banner |
| 12 | **Mac Minesweeper** | Puzzle | 2D | Safe first click, recursive flood-fill discovery, flag toggling, timer & mine count |
| 13 | **3D Rubik's & Polyhedra** | Demo / Puzzle | 3D | Full 3D Rubik's cube, mouse drag orbit, back-face culling, 6-face dither shading |
| 14 | **3D Solar System** | Sim / Orbit | 3D | Keplerian orbits, Sun, Mercury through Saturn with rings, Moon, 3D camera tilt/zoom |

---

## 3. How to Run

### Desktop Launcher (Game Hub)
To browse and launch any of the 14 games with a single click in a Classic Macintosh Finder window:
```bash
python games/game_hub.py
```

### Running Individual Games Directly
Each game can be launched standalone from the command line:
```bash
python games/01_racing/racer.py
python games/02_tetris/tetris.py
python games/03_space_invaders/invaders.py
python games/04_asteroids/asteroids.py
python games/05_pacman/pacman.py
python games/06_snake/snake.py
python games/07_breakout/breakout.py
python games/08_lunar_lander/lunar.py
python games/09_dungeon_3d/dungeon.py
python games/10_starflight_3d/starflight.py
python games/11_flappy_jumper/flappy.py
python games/12_minesweeper/mines.py
python games/13_rubiks_cube_3d/cube3d.py
python games/14_solar_system_3d/solarsys.py
```

### PlusOS Native Bare-Metal Target
All C source files (`*.c`) are written using the PlusOS kernel headers (`<plusos/plusos.h>`, `<plusos/gfx.h>`, `<plusrom/OSUtil.h>`) and can be built into `.pex` binaries for execution on real Macintosh Plus / SE hardware or Mini vMac / MAME:
```bash
wsl -d Ubuntu --cd /mnt/c/Users/Evil/PlusOS -- make apps
```

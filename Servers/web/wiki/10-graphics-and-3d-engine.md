# Fixed-Point 3D Engine & Graphics

The Motorola 68000 lacks a Floating-Point Unit (FPU). PlusOS features a high-performance **16.16 fixed-point 3D graphics pipeline** (`<plus3d/plus3d.h>`) capable of rendering rotating solid and wireframe meshes at 30+ frames per second directly on an 8 MHz processor.

---

## 1. 16.16 Fixed-Point Math Pipeline

All coordinates, trigonometry, and matrix transformations use 32-bit signed fixed-point numbers ($16\text{ bits integer}.16\text{ bits fraction}$):
- **Trig Lookup Tables**: Pre-computed 256-entry sine and cosine tables stored in ROM/text segment; zero runtime calculation overhead.
- **Hardware Multiplication**: Uses the 68000's native `MULS.W` and `DIVS.W` instructions with overflow protection.
- **Matrix Transformations**: 3D rotation around $X$, $Y$, and $Z$ axes, camera translation, and perspective projection onto the 2D $512 \times 342$ screen coordinates:
  $$x' = \frac{x \cdot f}{z + d} + x_{\text{center}}, \quad y' = \frac{y \cdot f}{z + d} + y_{\text{center}}$$

---

## 2. 1-Bit Ordered Bayer Dithering

Because the Macintosh Plus and SE have monochrome 1-bit displays, PlusOS implements **$4 \times 4$ ordered Bayer dithering** to simulate 16 distinct grayscale shading levels (`gfx_poly_dither`):

```text
Bayer Matrix (4x4):
[  0   8   2  10 ]
[ 12   4  14   6 ]
[  3  11   1   9 ]
[ 15   7  13   5 ]
```

When rendering 3D solid faces:
1. Face normal vectors are dotted with the directional light source vector ($\vec{N} \cdot \vec{L}$).
2. The dot product is mapped to a light intensity index from $0$ (pure black) to $15$ (pure white).
3. The polygon rasterizer applies the corresponding $4 \times 4$ Bayer pattern during scanline horizontal filling.

---

## 3. Geometric Primitives & 3D Meshes

PlusOS includes built-in rasterizers for complex shapes:
- **Polygons & N-Gons**: Arbitrary convex polygon filling (`gfx_poly_fill`), wireframes, and regular N-gons (`gfx_ngon_fill`).
- **Hexagons**: Specialized flat-top and pointy-top hexagonal cell generators (`gfx_hexagon`).
- **3D Procedural Meshes**:
  - `mesh_build_cube`: 8 vertices, 6 faces, 12 edges.
  - `mesh_build_hex_prism`: 12 vertices, 8 faces (hexagonal cylinder).
  - `mesh_build_pyramid`: 5 vertices, 5 faces.
  - `mesh_build_octahedron`: 6 vertices, 8 triangular faces.
  - `mesh_build_torus`: Multi-segment wireframe donut.

Run the `cube3d` utility in the shell to see real-time rotating solid and wireframe models!

---

## Navigation

- **Previous Chapter**: [← 09. PlusNET: DaynaPORT TCP/IP Stack](/wiki/09-networking-plusnet)
- **Next Chapter**: [11. Complete Syscall ABI Reference (SYS 1-63) →](/wiki/11-syscall-abi-reference)
- **Table of Contents**: [Return to Getting Started](/wiki/01-getting-started)

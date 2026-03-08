# circles

A 3D OpenGL visualization of the formula:

```
π^(n/2) / (n/2)!
```

This is the **volume of the unit n-dimensional ball**, generalized to continuous values of *n* via the Gamma function: `π^(n/2) / Γ(n/2 + 1)`.

The program renders 26 circles (n = 0 to 25) stacked in 3D space, where each circle's radius corresponds to the function value. The structure grows, peaks near n ≈ 5, then shrinks toward zero — all rotating around multiple axes.

## Build

Requires **CMake ≥ 3.20**, a C++20 compiler, and **GLFW 3** (e.g. `brew install glfw` on macOS).

```bash
cmake -B build -DCMAKE_PREFIX_PATH=/opt/homebrew .
cmake --build build
./build/circles
```

## Controls

| Input | Action |
|-------|--------|
| Left-drag | Orbit camera |
| Scroll | Zoom in / out |
| Space | Toggle auto-rotation |
| R | Reset camera |
| Esc | Quit |

## Features

- **3D rotating circle stack** — each ring's radius = π^(n/2) / Γ(n/2 + 1)
- **Multi-axis rotation** — 1 main axis (fast) + 2 secondary axes (slower) for a tumbling effect
- **Translucent filled discs** with bright outer rings and subtle inner glow
- **Interpolated rings** between integer values for smooth visual continuity
- **Envelope curves** tracing the silhouette on 4 sides
- **Vertical connectors** linking adjacent circles
- **Peak highlight** — golden glow on n = 5 (integer maximum)
- **Ground grid** for spatial reference
- Anti-aliased lines, multisampling, alpha blending

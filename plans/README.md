# GeoQik Implementation Plans

## Status table

| # | Plan | Status | Dependencies |
|---|---|---|---|
| 001 | [Mesh Display](001-mesh-display.md) | DONE (`ce85bf3`) | — |
| 002 | [Basic Convenience API](002-basic-convenience-api.md) | TODO | 001 |
| 003 | [Renderer Library Foundation](003-renderer-library-foundation.md) | DONE | — |
| 004 | [Renderer Windowing & Input](004-renderer-windowing-input.md) | DONE | 003 |
| 005 | [Renderer Camera & Auto-Fit](005-renderer-camera-and-autofit.md) | DONE | 003 |
| 006 | [Renderer CameraInteractor & ImGui](006-renderer-camera-interactor-imgui.md) | DONE | 004, 005 |
| 007 | [GeoQik/Renderer Integration Cleanup](007-geoqik-renderer-integration-cleanup.md) | DONE | 006 |
| 008 | [Renderer Standalone Example](008-renderer-standalone-example.md) | DONE | 006 |
| 009 | [Renderer Decouple from geoqik::core](009-renderer-decouple-from-core.md) | TODO | 008 |
| 010 | [Mesh Serialization and Replay](010-mesh-serialization-and-replay.md) | TODO | 001 |
| 011 | [Serialization Hardening and Bulk I/O](011-serialization-hardening-and-bulk-io.md) | TODO | — |
| 012 | [Renderer Facade](012-renderer-facade.md) | TODO | 008 |
| 013 | [Renderer API Cleanup](013-renderer-api-cleanup.md) | TODO | 012 |

## Execution order

1. **001 — Mesh Display** *(DONE)*: Self-contained. Implements `MeshBuffer`, wires the scene → renderer → OpenGL pipeline, and exposes the low-level C API (`geoqik_add_mesh_opts`). No other plan is a prerequisite.
2. **002 — Basic Convenience API**: Requires 001. Adds `geoqik_vec3_t`, `geoqik_create_triangle_mesh()` (typed + validated), and `geoqik_destroy_triangle_mesh()` per spec Part One §4.
3. **003 — Renderer Library Foundation**: Self-contained. Creates the `renderer` CMake target, relocates `src/opengl` under it, and moves the 5 dependency-free data types (`Viewport`, `Color`, `MathConstants`, `PickRay`, `CameraProjectionType`). Must land before 004/005/006.
4. **004 — Renderer Windowing & Input**: Requires 003. Moves `GlfwWindow`, `WindowSettings`, `InputState`, `InputCaptureState`. **Runs in parallel with 005** — disjoint files, same single prerequisite.
5. **005 — Renderer Camera & Auto-Fit**: Requires 003. Moves `Camera`; moves and decouples `CameraAutoFit` from `Scene` (the one non-mechanical refactor in this whole initiative). **Runs in parallel with 004.**
6. **006 — Renderer CameraInteractor & ImGui**: Requires both 004 and 005 (composes `Camera` + `InputState`). Moves `RayPlaneIntersection`, `CameraInteractor`, `ImGuiOverlay` — the last generic files left in `src/geoqik/source/`.
7. **007 — GeoQik/Renderer Integration Cleanup**: Requires 006. Not a code-moving plan — sweeps for anything 003–006 missed, runs the full regression + installed-package smoke test, and documents the explicit decision *not* to make `renderer` independently installable yet. **Can run in parallel with 008.**
8. **008 — Renderer Standalone Example**: Requires 006 (needs the complete renderer surface, not 007's domain-side cleanup). Adds `example_renderer_standalone`, a window+camera+drawables+ImGui demo that links zero GeoQik domain code — the concrete proof that the extraction achieved its goal. **Can run in parallel with 007.**
9. **009 — Renderer Decouple from geoqik::core**: Requires 008 (008 proves the renderer surface is stable before we touch the build graph). Creates renderer-owned copies of the 6 generic utility headers currently borrowed from `geoqik::core` (Assert, Warnings, DLLWarnings, Compiler, FmtIncludeHelper, ScopedAction), updates all includes across renderer, and removes `geoqik::core` from the renderer CMake link list entirely.

10. **010 — Mesh Serialization and Replay**: Requires 001 (already DONE). Adds `MeshBufferSnapshot` to `SceneSnapshot`, extends `GeoQikLogEntry` with `AddMeshWithOpts`/`RemoveMesh`, adds serializer entries (type IDs 18/19), adds undo overloads, and wires mesh messages into `m_messageLog`. **Must land before any plan that depends on mesh state surviving save/load or backward replay.**
11. **011 — Serialization Hardening and Bulk I/O**: Self-contained, no prerequisites. Fixes the dead overflow guard in `read_float_vector`, adds a 100-million-float cap, and replaces per-element stream loops with single bulk `stream.read/write`. **Can run in parallel with 010**, but if 010 lands first its executor must apply the same bulk pattern to `write_uint32_vector`/`read_uint32_vector` from the start (see plan 011 Step 3).
12. **012 — Renderer Facade**: Requires 008 (renderer surface must be stable). Creates `renderer::Renderer` which owns `GlfwWindow`, `DrawablesManager`, `CameraInteractor`, and `ImGuiOverlay`. Wires all callbacks internally with ImGui-first routing. Exposes `add_point_drawable`, `add_line_drawable`, `add_mesh_drawable`, `begin_frame`, `draw`, `end_frame`, and callback extension points. Rewrites `example_renderer_standalone.cpp` to use the new API.
13. **013 — Renderer API Cleanup**: Requires 012. Removes redundant `vertexDimension`/`colorDimension` parameters from `DrawablesManager::add_point_drawable` and `add_line_drawable` (always 3 and 4). Introduces `LightingConfig` struct and collapses the 11-parameter `draw_meshes` to 6. Moves 8 internal pan/rotate state fields out of `CameraSettings` into private members of `CameraInteractor`.

## Dependency notes

- Plan 001 leaves `SceneSnapshot` incomplete for meshes (documented). **Plan 010** addresses this — it adds `MeshBufferSnapshot` to `SceneSnapshot` and enables full mesh serialization and replay.
- A follow-up plan for **mesh lights / lighting config** can expose the `lightPosition`, `lightColor`, `ambientColor`, and `shininess` parameters currently hardcoded in `Context.cpp` (head-light).
- A follow-up plan for **Extended Convenience API** (spec Part Two) should add `geoqik_triangle_mesh_data_t` (positions + normals + texcoords + colors) as an overload of `geoqik_create_triangle_mesh`. Requires 002.
- A follow-up plan for **Stable Descriptor API** (spec Part Three) should add `geoqik_mesh_descriptor_t` and `geoqik_create_mesh()`. Requires the extended convenience API plan so the convenience functions can be refactored as wrappers around the descriptor path.
- **File overlap between 001 and 007**: both touch `src/geoqik/source/Rendering/OpenGLSceneRenderer.hpp`/`.cpp` — 001 adds mesh-drawable support, 003–007 rewire the file's includes to point at the relocated `renderer`/`opengl` headers. They don't conflict logically (different parts of the same files), but if both are being executed around the same time, land 003 (foundation) before starting 001's `OpenGLSceneRenderer` edits to avoid a rebase, or land 001 fully first — don't interleave the two mid-file.
- **Renderer reusability scope, read before assuming more was done than was**: plans 003–008 make the `renderer` CMake target free of any GeoQik *domain* dependency (no `Scene`, `core::UUID`-keyed state, message/replay types) and prove it with a standalone example (008). They do **not** make `renderer` independently installable/exportable via `find_package(renderer)` from a separate repository or vcpkg port — see plan 007's Background section for why that's a deliberate, explicit deferral rather than an oversight. A future plan, **Renderer Packaging** (not yet written), would add `RendererConfig.cmake.in` + `install()` rules for `renderer_renderer`/`renderer_opengl` and decide on a distribution mechanism (vcpkg port vs. git submodule vs. CPM-fetchable subtree) before any other project can actually pull this library in without copying the source tree.

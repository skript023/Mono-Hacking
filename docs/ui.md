# Shared UI integration

The reusable library is maintained at [skript023/astra-ui](https://github.com/skript023/astra-ui). Mono Hacking and `L:/Coding/PW` fetch the pinned commit through `scripts/astra.cmake`; set `FETCHCONTENT_SOURCE_DIR_ASTRA` to a local checkout for offline development. Rendering and navigation fixes belong in that repository.

## Using the menu

The initial layout remains List. Open Settings, select Menu Layout, and use left/right to choose Window. In Window mode, click sidebar categories, use Back/breadcrumbs, search the current page, drag sliders, or Ctrl+click a slider to type.

Theme choices are Emerald, Violet, Ocean, Custom, and Studio. Studio is the default for configurations without a saved theme and provides the light/purple HaxGui-inspired design. Custom uses the saved Accent Red/Green/Blue values. Layout/theme are serialized by the existing menu settings system. Window position and size use the existing ImGui ini file.

The previous per-component List color pages were replaced by the shared theme and accent controls. List position and width remain explicit List settings; Window can be moved by its custom header and resized.

## Ownership

- `include/astra/canvas.hpp`: shared canvas facade that selects List or Window rendering and returns input events.
- `host/canvas.cpp`: shared legacy menu canvas, rendering, option snapshots, and navigation event mapping.
- `src/menu/astra_host/platform.cpp`: project adapter for keyboard/controller input, resolution lookup, and color-space setup.
- `include/astra/host/menu/*`: one shared set of legacy option/submenu classes for both projects.
- `include/astra/navigation.hpp`: one path per tab; no global history replay.
- `src/window.cpp`: modern responsive cards, sidebar, search, breadcrumbs, drag/close.
- `src/widgets.cpp`: clickable controls and direct numeric/choice bindings.
- `src/list.cpp`: shared List renderer.
- `src/theme.cpp`: palettes, scoped styling, theme transitions.
- Project renderers: device ownership, graphics backend, font/texture upload, resize and GPU synchronization.

All menu input is now polled once per rendered frame. Window messages feed ImGui and captured keyboard/mouse/raw-input messages are kept from the game window procedure. The cursor hook only suppresses recentering when mouse UI is enabled. Valheim's existing TakeInput hook also honors an atomic capture flag so polling-based player input is suppressed during UI capture. PW's engine-level behavior still requires an in-game check; its Win32/raw-input capture is integrated.

No additional game renderer is automatically selected by this migration. Mono Hacking retains its DX11/Vulkan adapters. PW retains DX11/D3D11On12. The independent library offers optional native backend targets for DX9, DX11, DX12, Vulkan, and OpenGL3 for hosts that supply their respective devices and frame integration.

## Local validation

Standalone build and regression tests from the fetched GitHub source: `out/astra-github-tests`.

```powershell
cmake --build out/astra-github-tests -j 4
ctest --test-dir out/astra-github-tests --output-on-failure
cmake --build out/build/x64-Clang-Release -j 4
ctest --test-dir out/build/x64-Clang-Release -R '^renderer_' --output-on-failure
cmake --build 'L:/Coding/PW/out/build/x64-Release' -j 4
```

Manual game checks: switch List/Window with values intact; navigate two nested pages and return; switch tabs then return to the same depth; type into search/slider without triggering menu hotkeys; resize/reopen the window; confirm clicks and keyboard input do not also trigger gameplay; confirm saved layout/theme reload. GPU/backend builds and headless input tests cannot replace these engine integration checks.

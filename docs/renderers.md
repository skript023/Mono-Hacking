# DX11 and Vulkan renderer

`src/renderer.hpp` exposes DLL candidate checks and the active backend. DLL presence is not proof of the active API: both may be loaded. Presentation to the game window confirms the active backend.

- `src/renderer.cpp`: shared ImGui context, fonts, menu callbacks, Win32 input and Unity adapter selection.
- `src/render/render_dx11.cpp`: DX11 targets, texture uploads, resize and state restoration.
- `src/render/render_vulkan.cpp`: Vulkan bootstrap/capture, swapchain resources, texture uploads and presentation synchronization.

The implementation follows the dummy-device and acquisition approach in the local HorseMenu sources (`src/core/renderer/Renderer.cpp` and `src/game/hooks/GUI/Vulkan.cpp`). Naming and formatting follow this repository's snake_case conventions and `.clang-format`.

## Startup and late injection

Startup capture still records the actual instance, device, requested queues, window surface and swapchain creation information.

When the game is already running, attachment instead:

1. Creates an owned Vulkan instance and temporary device, matching Unity's graphics vendor/device IDs when available.
2. Resolves and hooks `vkQueuePresentKHR`, `vkCreateSwapchainKHR`, `vkAcquireNextImageKHR` and `vkAcquireNextImage2KHR`, plus destruction hooks.
3. Destroys the temporary device while retaining the owned instance until hooks are removed.
4. Captures the existing game device on successful image acquisition and initializes the overlay on presentation.

Vulkan provides no API to retrieve the creation description of an existing swapchain. Like HorseMenu, initial late attachment uses **BGRA8 UNORM and the current window dimensions**. This is provisional, not a detected format. A subsequent `vkCreateSwapchainKHR` supplies the actual format, extent and usage; render targets and textures are recreated accordingly. A game that starts with a different format, HDR, or an independently sized swapchain needs swapchain recreation before correct overlay rendering can be expected.

The late path currently requires one unambiguous graphics queue family and uses its queue zero. It supports a single game swapchain presented on that graphics queue. Separate graphics/presentation queues, protected swapchains and multi-swapchain presentation remain unsupported. Startup capture also handles explicitly requested graphics queues other than queue zero. Unsupported paths pass through to the game.

The system Vulkan loader is loaded dynamically when available. DX11 does not require the Vulkan runtime. The overlay never destroys the game's instance, device or swapchain on its own; its destruction hooks only forward the game's requests after releasing overlay resources.

## Synchronization and cleanup

Each swapchain image has its own command buffer, fence and presentation semaphore. Overlay submission consumes the game's present waits; the forwarded present waits on overlay completion. The render pass loads the existing image and returns it to `PRESENT_SRC_KHR`. Cleanup waits for device work before releasing overlay resources; hook removal drains active callbacks before releasing trampolines and the owned probe instance.

## Build and checks

Vulkan-Headers 1.3.290 is fetched by CMake. No Vulkan SDK or Vulkan import library is required.

Enable `BUILD_RENDERER_SMOKE_TESTS=ON`, then build and run:

```powershell
cmake --build out/build/x64-Clang-Release --target renderer_smoke -j 4
ctest --test-dir out/build/x64-Clang-Release -R '^renderer_' --output-on-failure
```

Tests use hidden windows and the actual renderer implementations with a small UI fixture:

- DX11: WARP pixel readback, textures, background preservation, output-merger restoration, resize and repeated cleanup.
- Vulkan startup: capture before device creation.
- Vulkan late Acquire1/Acquire2: create the game device and swapchain, cache the Vulkan entry points and present three frames **before installing any overlay hooks**. Then attach via a dummy device, render 24 overlays, recreate the swapchain with a different size/format, detach/unhook and verify that game presentation continues.
- Vulkan cases exercise zero, one and two game wait semaphores.

Late fixtures begin with BGRA8 UNORM to exercise the documented initial-format fallback; they do not prove arbitrary-format late attachment. Executables run in `renderer-tests/` to avoid accidentally loading the trainer's adjacent `version.dll`. In-game testing on the user's Unity build and driver is still required.

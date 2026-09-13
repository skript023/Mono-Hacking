# Header-only dependency. Vulkan is loaded dynamically; DX11 does not need its runtime.
include(FetchContent)
FetchContent_Declare(vulkan_headers
    URL https://github.com/KhronosGroup/Vulkan-Headers/archive/refs/tags/v1.3.290.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(VULKAN_HEADERS_ENABLE_MODULE OFF CACHE BOOL "Build optional Vulkan C++ module")
FetchContent_MakeAvailable(vulkan_headers)

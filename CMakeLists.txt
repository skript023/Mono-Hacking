cmake_minimum_required(VERSION 3.20)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

include(scripts/cross-compile.cmake)

project(Valheim CXX)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
        $<$<COMPILE_LANGUAGE:C>:-w>
        $<$<COMPILE_LANGUAGE:CXX>:-w>
    )
endif()

set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")

set(OPENSSL_ROOT_DIR "${PROJECT_SOURCE_DIR}/openssl")
set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include")   # opsional
set(OPENSSL_LIBRARIES "${OPENSSL_ROOT_DIR}/lib")         # opsional
set(ZLIB_LIBRARIES "${OPENSSL_LIBRARIES}/zlib.lib")

# Networking cleanup owns the OpenSSL instance embedded in this DLL.
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)

get_target_property(ssl_loc OpenSSL::SSL LOCATION)
get_target_property(crypto_loc OpenSSL::Crypto LOCATION)

message(STATUS "OpenSSL::SSL    -> ${ssl_loc}")
message(STATUS "OpenSSL::Crypto -> ${crypto_loc}")

# Fetch modules
message("\nFetching modules")
include(scripts/minhook.cmake)
include(scripts/json.cmake)
include(scripts/g3log.cmake)

message("\nFetching custom modules")
include(scripts/imgui.cmake)
include(scripts/astra.cmake)
include(scripts/stbi.cmake)
include(scripts/asio.cmake)
include(scripts/websocket.cmake)
include(scripts/discord.cmake)
include(scripts/quickjs.cmake)
include(scripts/vulkan.cmake)

# Valheim
message(STATUS "Valheim")
file(GLOB_RECURSE SRC_MAIN
    "${SRC_DIR}/**.hpp"
    "${SRC_DIR}/**.h"
    "${SRC_DIR}/**.cpp"
    "${SRC_DIR}/**.cc"
    "${SRC_DIR}/**.cxx"
    "${SRC_DIR}/**.def"
)

if (MSVC)
  add_compile_options(/bigobj)
else ()
  add_compile_options(-Wa,-mbig-obj)
endif ()

add_library(Valheim MODULE "${SRC_MAIN}")
target_sources(Valheim PRIVATE "${astra_SOURCE_DIR}/host/canvas.cpp")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(TARGET Valheim PROPERTY CXX_STANDARD 23) # 23 Because std::format is not avalible in std:c++20 for some reason. Maybe it's because i use v142 toolset.

source_group(TREE ${SRC_DIR} PREFIX "src" FILES ${SRC_MAIN})

target_include_directories(Valheim PRIVATE 
    "${SRC_DIR}"
    "${SRC_DIR}/unreal_engine"
    "${SRC_DIR}/services"
    "${json_SOURCE_DIR}/single_include"
    "${imgui_SOURCE_DIR}"
    "${stbi_SOURCE_DIR}"
    "${websocket_SOURCE_DIR}"
    "${asio_SOURCE_DIR}/asio/include"
    "${discord_SOURCE_DIR}/include"
    "${quickjs_SOURCE_DIR}/src"
    "${OPENSSL_INCLUDE_DIR}"
    "${minhook_SOURCE_DIR}/src/hde"
)

target_precompile_headers(Valheim PRIVATE "${SRC_DIR}/common.hpp")
target_link_libraries(Valheim PRIVATE astra::ui Vulkan::Headers minhook quickjs imgui AsyncLogger dbghelp discord winhttp iphlpapi ${ZLIB_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto Crypt32 $<IF:$<BOOL:${CROSSCOMPILE}>,d3dcompiler_47,d3dcompiler> $<IF:$<BOOL:${CROSSCOMPILE}>,xinput1_4,xinput>)

# Warnings as errors
#set_property(TARGET Valheim PROPERTY COMPILE_WARNING_AS_ERROR ON)

add_compile_definitions(Valheim 
    "_CRT_SECURE_NO_WARNINGS"
    "NOMINMAX"
    "WIN32_LEAN_AND_MEAN"
)

set_target_properties(Valheim PROPERTIES OUTPUT_NAME "version")

target_compile_options(Valheim PRIVATE
    # Do not warn about non-reproducible code
    $<$<COMPILE_LANG_AND_ID:C,Clang>:-Wno-date-time>
    $<$<COMPILE_LANG_AND_ID:CXX,Clang>:-Wno-date-time>
    $<$<COMPILE_LANG_AND_ID:CXX,Clang>:-Wno-microsoft-cast>
    $<$<COMPILE_LANG_AND_ID:CXX,Clang>:-Wno-extern-c-compat>
)

if (NOT ${CROSSCOMPILE})
    add_subdirectory(injector)
endif()

option(BUILD_RENDERER_SMOKE_TESTS "Build standalone GPU renderer smoke tests" OFF)
if(BUILD_RENDERER_SMOKE_TESTS)
    add_executable(renderer_smoke tests/renderer/smoke.cpp src/render/render_dx11.cpp src/render/render_vulkan.cpp)
    target_include_directories(renderer_smoke PRIVATE tests/renderer/stubs src/render "${imgui_SOURCE_DIR}")
    # Keep test processes away from the trainer version.dll during DLL resolution.
    set_target_properties(renderer_smoke PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/renderer-tests")
    target_compile_features(renderer_smoke PRIVATE cxx_std_23)
    target_compile_definitions(renderer_smoke PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN RENDERER_SMOKE_TEST)
    target_link_libraries(renderer_smoke PRIVATE imgui minhook Vulkan::Headers d3d11 dxgi user32 gdi32 dwmapi)
    enable_testing()
    add_test(NAME renderer_dx11 COMMAND renderer_smoke dx11)
    add_test(NAME renderer_vulkan COMMAND renderer_smoke vulkan)
    add_test(NAME renderer_vulkan_late COMMAND renderer_smoke vulkan-late)
    add_test(NAME renderer_vulkan_late2 COMMAND renderer_smoke vulkan-late2)
    set_tests_properties(renderer_dx11 renderer_vulkan renderer_vulkan_late renderer_vulkan_late2 PROPERTIES WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/renderer-tests" TIMEOUT 30)
endif()

option(BUILD_STARTUP_TESTS "Build startup lifecycle regression tests" OFF)
if(BUILD_STARTUP_TESTS)
    enable_testing()
    add_executable(startup_lifecycle tests/startup/lifecycle.cpp src/thread_pool.cpp src/hooking/detour_base.cpp)
    target_include_directories(startup_lifecycle PRIVATE tests/startup src)
    target_precompile_headers(startup_lifecycle PRIVATE tests/startup/common.hpp)
    target_compile_features(startup_lifecycle PRIVATE cxx_std_23)
    set_target_properties(startup_lifecycle PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/renderer-tests")
    add_test(NAME startup_lifecycle COMMAND startup_lifecycle)
    set_tests_properties(startup_lifecycle PROPERTIES TIMEOUT 30 WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/renderer-tests")
endif()

include(FetchContent)

message("quickjspp")

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(QUICKJS_BRANCH "win_clang")
    message(STATUS "Compiler Clang terdeteksi, menggunakan branch: ${QUICKJS_BRANCH}")
else()
    set(QUICKJS_BRANCH "master")
    message(STATUS "Bukan Clang, menggunakan branch default: ${QUICKJS_BRANCH}")
endif()

FetchContent_Declare(
    quickjs
    GIT_REPOSITORY https://github.com/skript023/QuickJs.git
    GIT_TAG ${QUICKJS_BRANCH}
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(quickjs)
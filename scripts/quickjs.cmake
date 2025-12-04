include(FetchContent)

message("quickjspp")

FetchContent_Declare(
    quickjs
    GIT_REPOSITORY https://github.com/skript023/QuickJs.git
    GIT_TAG master
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(quickjs)

set_property(TARGET quickjs PROPERTY CXX_STANDARD 23)
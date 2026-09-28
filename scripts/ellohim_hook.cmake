include(FetchContent)

FetchContent_Declare(
    ellohim_hook
    GIT_REPOSITORY https://github.com/skript023/Ellohim-Hook.git
    GIT_TAG        ccd6337bdcdd0b6c6df2a554e0c7b518f6bb19c1
    GIT_PROGRESS TRUE
)
message("Ellohim-Hook")
FetchContent_MakeAvailable(ellohim_hook)

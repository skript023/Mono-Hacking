include(FetchContent)

FetchContent_Declare(
    ellohim_hook
    GIT_REPOSITORY https://github.com/skript023/Ellohim-Hook.git
    GIT_TAG        9a3219a16b2f4f4b965040cd7d4eb3b5e8a0b40e
    GIT_PROGRESS TRUE
)
message("Ellohim-Hook")
FetchContent_MakeAvailable(ellohim_hook)

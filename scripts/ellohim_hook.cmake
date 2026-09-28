include(FetchContent)

FetchContent_Declare(
    ellohim_hook
    GIT_REPOSITORY https://github.com/skript023/Ellohim-Hook.git
    GIT_TAG        590955fc59d1dfaddcd5cbdacf8161d38e824360
    GIT_PROGRESS TRUE
)
message("Ellohim-Hook")
FetchContent_MakeAvailable(ellohim_hook)

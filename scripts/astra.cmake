include(FetchContent)

message("Fetching Astra UI")
FetchContent_Declare(
    astra
    GIT_REPOSITORY https://github.com/skript023/astra-ui.git
    GIT_TAG 4920e95cdee6d88f0acb294fcb4f2a3ead896faf
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(astra)
include(FetchContent)

message("Fetching Astra UI")
FetchContent_Declare(
    astra
    GIT_REPOSITORY https://github.com/skript023/astra-ui.git
    GIT_TAG 26ba033691bcbaf170477f78a7bfb7e14f3ed151
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(astra)
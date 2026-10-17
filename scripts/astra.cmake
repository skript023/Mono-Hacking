include(FetchContent)

message("Fetching Astra UI")
FetchContent_Declare(
    astra
    GIT_REPOSITORY https://github.com/skript023/astra-ui.git
    GIT_TAG f43c900e860bca0e7e4343a785cf47b5571eb313
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(astra)
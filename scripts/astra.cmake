include(FetchContent)

message("Fetching Astra UI")
FetchContent_Declare(
    astra
    GIT_REPOSITORY https://github.com/skript023/astra-ui.git
    GIT_TAG 2e6d355323f8eed8da7016d5d22319f8c4e36056
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(astra)
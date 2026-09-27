include(FetchContent)

message("Fetching Astra UI")
FetchContent_Declare(
    astra
    GIT_REPOSITORY https://github.com/skript023/astra-ui.git
    GIT_TAG 88d59403c6d4d11ee1d42185a351b8c7a14a62cb
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(astra)
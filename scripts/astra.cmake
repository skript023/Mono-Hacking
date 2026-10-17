include(FetchContent)

message("Fetching Astra UI")
FetchContent_Declare(
    astra
    GIT_REPOSITORY https://github.com/skript023/astra-ui.git
    GIT_TAG 5df005debbb54c7a68a0b4dc21b224431a27f337
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(astra)
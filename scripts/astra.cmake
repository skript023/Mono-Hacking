include(FetchContent)

message("Fetching Astra UI")
FetchContent_Declare(
    astra
    GIT_REPOSITORY https://github.com/skript023/astra-ui.git
    GIT_TAG 1b793b51532b937681bad58f50a19995511c6bc5
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(astra)
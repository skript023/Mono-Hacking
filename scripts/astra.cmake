include(FetchContent)

message("Fetching Astra UI")
FetchContent_Declare(
    astra
    GIT_REPOSITORY https://github.com/skript023/astra-ui.git
    GIT_TAG 12f198093a57470e2ea861467e143eed6c0c5c5d
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(astra)
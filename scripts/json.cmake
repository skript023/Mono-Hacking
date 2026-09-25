include(FetchContent)

message(STATUS "Setting up nlohmann::json")
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/include.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(json)

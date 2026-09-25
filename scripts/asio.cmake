include(FetchContent)

FetchContent_Declare(
    asio
    URL https://github.com/chriskohlhoff/asio/archive/efdc25ab99786101351a5afb39f01dfaf0781401.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
) 
message("Asio")

FetchContent_MakeAvailable(asio)

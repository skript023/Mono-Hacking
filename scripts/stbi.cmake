include(FetchContent)

FetchContent_Declare(
    stbi
    URL https://github.com/nothings/stb/archive/f58f558c120e9b32c217290b80bad1a0729fbb2c.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
) 
message("stbi")

FetchContent_MakeAvailable(stbi)

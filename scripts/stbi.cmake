include(FetchContent)

FetchContent_Declare(
    stbi
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        f58f558c120e9b32c217290b80bad1a0729fbb2c
    GIT_PROGRESS TRUE
) 
message("stbi")

FetchContent_MakeAvailable(stbi)
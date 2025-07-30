include(FetchContent)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt
    GIT_TAG        11.2.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(fmt)

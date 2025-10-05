cmake_minimum_required(VERSION 3.18)
project(yt_dlp_gui LANGUAGES CXX RC)

add_executable(yt_dlp_gui WIN32
    src/main.cpp
    res/app.rc
)

target_compile_features(yt_dlp_gui PRIVATE cxx_std_17)
target_compile_definitions(yt_dlp_gui PRIVATE
    UNICODE _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _WIN32_WINNT=0x0601
)

target_link_libraries(yt_dlp_gui PRIVATE
    comctl32
    comdlg32
    shlwapi
    shell32
    ole32
)

set_target_properties(yt_dlp_gui PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release
    VS_GLOBAL_RunCodeAnalysis false
)

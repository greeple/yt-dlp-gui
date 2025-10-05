cmake_minimum_required(VERSION 3.15)
project(yt-dlp-gui)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_executable(yt-dlp-gui WIN32
        src/main.cpp
        src/app.rc
    )
    
    target_compile_definitions(yt-dlp-gui PRIVATE UNICODE _UNICODE)
    target_link_libraries(yt-dlp-gui comctl32 shell32)
    
    # Установка Unicode
    set_target_properties(yt-dlp-gui PROPERTIES
        WIN32_EXECUTABLE TRUE
        LINK_FLAGS "/ENTRY:wWinMainCRTStartup"
    )
endif()

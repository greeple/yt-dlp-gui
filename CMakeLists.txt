cmake_minimum_required(VERSION 3.15)
project(yt-dlp-gui)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_executable(yt-dlp-gui WIN32
        src/main.cpp
        src/app.rc
    )
    
    target_link_libraries(yt-dlp-gui
        comctl32
        shell32
        ole32
    )
    
    # Статическая линковка для portable exe
    if(MINGW)
        target_link_options(yt-dlp-gui PRIVATE
            -static-libgcc
            -static-libstdc++
            -static
            -lpthread
        )
    endif()
endif()

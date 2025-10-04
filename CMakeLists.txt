cmake_minimum_required(VERSION 3.15)
project(yt-dlp-gui)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_executable(yt-dlp-gui WIN32
        src/main.cpp
        src/settings.cpp
        src/app.rc
    )
    
    target_include_directories(yt-dlp-gui PRIVATE src)
    
    target_link_libraries(yt-dlp-gui
        comctl32
        shell32
        ole32
    )
    
    # Добавляем флаги для UTF-8
    target_compile_options(yt-dlp-gui PRIVATE
        -finput-charset=UTF-8
        -fexec-charset=UTF-8
    )
    
    # Статическая линковка
    if(MINGW)
        target_link_options(yt-dlp-gui PRIVATE
            -static-libgcc
            -static-libstdc++
            -static
            -municode
        )
    endif()
    
    # Указываем что используем Unicode entry point
    set_target_properties(yt-dlp-gui PROPERTIES
        LINK_FLAGS "-municode"
    )
endif()

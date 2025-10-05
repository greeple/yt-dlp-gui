cmake_minimum_required(VERSION 3.16)
project(YTDLPGui)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set Unicode
add_definitions(-DUNICODE -D_UNICODE)

# Add executable
add_executable(YTDLPGui WIN32 main.cpp)

# Link libraries
target_link_libraries(YTDLPGui 
    comctl32
    shell32
    ole32
)

# Set subsystem to Windows
set_target_properties(YTDLPGui PROPERTIES
    LINK_FLAGS "/SUBSYSTEM:WINDOWS"
)

# Copy yt-dlp.exe to output directory (if exists)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/yt-dlp.exe")
    add_custom_command(TARGET YTDLPGui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/yt-dlp.exe"
        $<TARGET_FILE_DIR:YTDLPGui>)
endif()

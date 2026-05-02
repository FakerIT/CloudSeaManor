cmake_minimum_required(VERSION 3.20)
project(yunhai_mvp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.1
)

FetchContent_MakeAvailable(SFML)

add_executable(yunhai_mvp
    src/main.cpp
)

target_link_libraries(yunhai_mvp PRIVATE sfml-graphics sfml-window sfml-system)

if(WIN32)
    add_custom_command(
        TARGET yunhai_mvp
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:yunhai_mvp>
            $<TARGET_FILE_DIR:yunhai_mvp>
        COMMAND_EXPAND_LISTS
    )
endif()

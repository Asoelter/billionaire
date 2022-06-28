set(GAME_TARGET "game")
set(GAME_TARGET "game" PARENT_SCOPE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(FetchContent)

add_library(${GAME_TARGET} "demo.h" "demo.cpp")

set(BUILD_SHARED_LIBS OFF)

FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
)

FetchContent_MakeAvailable(SFML)

target_link_libraries(${GAME_TARGET} sfml-graphics)

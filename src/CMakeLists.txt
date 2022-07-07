set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME} "main.cpp")

target_link_libraries(${PROJECT_NAME} PRIVATE ${GAME_TARGET})

include(FetchContent)

set(BUILD_SHARED_LIBS OFF)

FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
)

FetchContent_MakeAvailable(SFML)

target_link_libraries(${PROJECT_NAME} sfml-graphics)

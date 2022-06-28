set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory("game")

add_executable(${PROJECT_NAME} "main.cpp")

target_link_libraries(${PROJECT_NAME} PRIVATE ${GAME_TARGET})

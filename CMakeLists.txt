cmake_minimum_required(VERSION 3.15)
project(smart_parking_improved_astar LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(simulator
    src/main.cpp
    src/astar.cpp
)
target_include_directories(simulator PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include)
add_executable(250919repath
    src/250919repath.cpp
)
target_include_directories(250919repath PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include)

add_executable(250604statisticlog
    src/250604statisticlog.cpp
)
target_include_directories(250604statisticlog PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include)

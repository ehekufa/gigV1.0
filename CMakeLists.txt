cmake_minimum_required(VERSION 3.10)
project(GIG LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Собираем все .cpp из src
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(gig ${SOURCES})

# Если у вас заголовки в include, можно добавить:
target_include_directories(gig PRIVATE include)

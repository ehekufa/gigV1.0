cmake_minimum_required(VERSION 3.10)
project(GIG LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Пути к заголовкам: основная папка и подпапка gig
include_directories(include)
include_directories(include/gig)

# Собираем все .cpp из src
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(gig ${SOURCES})

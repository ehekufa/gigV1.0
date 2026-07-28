cmake_minimum_required(VERSION 3.10)
project(GIG LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)
include_directories(include/gig)

file(GLOB_RECURSE SOURCES "src/*.cpp")

# WIN32 – приложение без консоли
add_executable(gig WIN32 ${SOURCES})

# Подключаем библиотеки для GUI и Windows API
target_link_libraries(gig shell32 user32 gdi32 comctl32)

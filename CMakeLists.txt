cmake_minimum_required(VERSION 3.13)

set(ENV{CXX} g++)
project(ccbench_base_silo)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

# file(GLOB_RECURSE PROJECT_SOURCES *.cpp)
# file(GLOB_RECURSE PROJECT_HEADERS *.h)

file(GLOB SILO_SOURCES
    "App/main.cpp" "Enclave/*.cpp"
)

#set(CMAKE_CXX_FLAGS "-pthread -fpermissive -Wreturn-type")


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")


add_executable(silo.exe ${SILO_SOURCES})


target_compile_options(silo.exe PUBLIC -O0 -g -std=c++11 -m64)
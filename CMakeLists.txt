cmake_minimum_required(VERSION 3.13)

set(ENV{CXX} g++)
project(ccbench_base_silo)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

# file(GLOB_RECURSE PROJECT_SOURCES *.cpp)
# file(GLOB_RECURSE PROJECT_HEADERS *.h)

file(GLOB_RECURSE SILO_SOURCES
    "App/*.cpp"
    "App/**/*.cpp"
    "Enclave/*.cpp"
)

#set(CMAKE_CXX_FLAGS "-pthread -fpermissive -Wreturn-type")


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")


add_executable(silo.exe ${SILO_SOURCES})


target_compile_options(silo.exe PUBLIC -O0 -g -std=c++11 -m64)


add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/log0/pepoch
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/log0
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/log0/pepoch
    COMMENT "Creating pepoch file in log0 directory"
)

add_custom_target(
    create_pepoch_file ALL
    DEPENDS ${CMAKE_BINARY_DIR}/log0/pepoch
)

add_dependencies(silo.exe create_pepoch_file)
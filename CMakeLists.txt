cmake_minimum_required(VERSION 3.0.0)
project(test_asio_libaio)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_definitions(-DASIO_HAS_FILE)
add_definitions(-DASIO_DISABLE_IO_URING)
add_definitions(-DASIO_HAS_LIB_AIO)

include_directories(include)

add_executable(test_asio_libaio main.cpp)
target_link_libraries(test_asio_libaio aio)

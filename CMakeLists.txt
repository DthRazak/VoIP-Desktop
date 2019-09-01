cmake_minimum_required(VERSION 3.14)
project(VoIP_Desktop)

set(CMAKE_CXX_STANDARD 14)

include_directories(include)
find_package(Boost COMPONENTS system thread chrono REQUIRED)
find_library(PORTAUDIO_LIB portaudio)
find_library(PTHREAD_LIB pthread)

add_executable(VoIP_Desktop
        include/audio.h
        include/buffer.hpp
        include/client.h
        include/menu.h
        include/mulawdecoder.h
        include/mulawencoder.h
        include/server.h
        src/audio.cpp
        src/client.cpp
        src/main.cpp
        src/menu.cpp
        src/mulawdecoder.cpp
        src/mulawencoder.cpp
        src/server.cpp
        src/server_test.cpp)

target_link_libraries(VoIP_Desktop PRIVATE pthread)
target_link_libraries(VoIP_Desktop PRIVATE portaudio)
target_link_libraries(VoIP_Desktop PRIVATE Boost::thread)
target_link_libraries(VoIP_Desktop PRIVATE Boost::chrono)
cmake_minimum_required(VERSION 3.15)
project(opus_stream LANGUAGES CXX)

option(PLATFORM_DESKTOP "Build for desktop (PortAudio)" ON)
option(PLATFORM_ESP32 "Build for ESP32 (ESP-IDF)" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (PLATFORM_DESKTOP)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(OPUS REQUIRED opus)
  find_path(PORTAUDIO_INCLUDE_DIR portaudio.h)
  find_library(PORTAUDIO_LIB portaudio)
  add_compile_definitions(PLATFORM_DESKTOP)
endif()

add_executable(opus_client
  src/client_main.cpp
  src/transport_udp.cpp
  src/audio_portaudio.cpp
  src/opus_codec.cpp
  src/sink_file.cpp
  src/entry_client.cpp
)
add_executable(opus_server
  src/entry_server.cpp
  src/server_main.cpp
  src/transport_udp.cpp
  src/opus_codec.cpp
  src/sink_file.cpp
)

find_package(Threads REQUIRED)

target_link_libraries(opus_client PRIVATE ${OPUS_LIBRARIES} ${PORTAUDIO_LIB} Threads::Threads)
target_link_libraries(opus_server PRIVATE ${OPUS_LIBRARIES} Threads::Threads)


target_include_directories(opus_client PRIVATE include ${OPUS_INCLUDE_DIRS} ${PORTAUDIO_INCLUDE_DIR})
target_include_directories(opus_server PRIVATE include ${OPUS_INCLUDE_DIRS})

target_link_libraries(opus_client PRIVATE ${OPUS_LIBRARIES} ${PORTAUDIO_LIB})
target_link_libraries(opus_server PRIVATE ${OPUS_LIBRARIES})

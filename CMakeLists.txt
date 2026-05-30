cmake_minimum_required(VERSION 3.15)
project(audio_recognition CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(audio-recognition src/main.cpp)

enable_testing()

add_executable(test_fft tests/test_fft.cpp)
add_test(NAME test_fft COMMAND test_fft)

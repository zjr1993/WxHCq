cmake_minimum_required(VERSION 3.26.0 FATAL_ERROR)
# set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
project(MFPlayer LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(EXECUTABLE_NAME MFPlayer)
add_executable(${EXECUTABLE_NAME} ringBuffer.cpp main.cpp ringBuffer.hpp)
target_compile_options(${EXECUTABLE_NAME} PRIVATE /utf-8)
target_link_libraries(${EXECUTABLE_NAME}
        mfplat
        mfreadwrite
        mfuuid
        mf
)
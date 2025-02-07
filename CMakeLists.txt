cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project(XlCq_Template VERSION 2.1.0
    DESCRIPTION "A XLL Add-In Template" 
    HOMEPAGE_URL "https://space.bilibili.com/14574654"
    LANGUAGES CXX C
)

set(ADDINS_INSTALL_PATH "C:$ENV{HOMEPATH}\\AppData\\Roaming\\Microsoft\\AddIns")
set(XlCq_LIB_PATH "${PROJECT_SOURCE_DIR}/lib")
set(XlCq_INCLUDE_PATH "${PROJECT_SOURCE_DIR}/include")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

option(XlCq_INSTALL "Install output to ADDINS_PATH" ON)

add_subdirectory(src)

if (XlCq_INSTALL)
    install(
        FILES
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/XlCq.xll"
        DESTINATION
        "${ADDINS_INSTALL_PATH}"
    )
endif()
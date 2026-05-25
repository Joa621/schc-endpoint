cmake_minimum_required(VERSION 3.10)
project(schc-endpoint C)

include(config.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(SCHC_FULL_SDK_LIB "schc-full-sdk::schc-full-sdk")
set(ZLOG_LIB          "zlog::zlog")

find_package(schc-full-sdk REQUIRED)
find_package(zlog          REQUIRED)

add_subdirectory(src)

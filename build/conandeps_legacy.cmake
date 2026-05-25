message(STATUS "Conan: Using CMakeDeps conandeps_legacy.cmake aggregator via include()")
message(STATUS "Conan: It is recommended to use explicit find_package() per dependency instead")

find_package(schc-full-sdk)
find_package(zlog)

set(CONANDEPS_LEGACY  schc-full-sdk::schc-full-sdk  zlog::zlog )
cmake_minimum_required(VERSION 3.16)
project(SSEImplementations LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# change if needed!!! C++20 needed.
set(CMAKE_CXX_COMPILER /usr/bin/g++-11)
set(CMAKE_CXX_FLAGS "--std=c++20")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

if (NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/0.18.1/conan.cmake"
        "${CMAKE_BINARY_DIR}/conan.cmake"
        TLS_VERIFY ON
    )
endif ()

include(${CMAKE_BINARY_DIR}/conan.cmake)

conan_cmake_configure(
    REQUIRES openssl/3.0.2
    GENERATORS cmake_find_package_multi
)

conan_cmake_autodetect(settings BUILD_TYPE ${CMAKE_BUILD_TYPE})
conan_cmake_install(
    PATH_OR_REFERENCE .
    BUILD missing
    REMOTE conancenter
    SETTINGS ${settings}
)

find_package(OpenSSL REQUIRED CONFIG)

add_executable(main
    src/main.cpp
    src/log_src.cpp
    src/log_src_i_loc.cpp
    src/log_src_i.cpp
    src/pi_bas.cpp
    src/sda.cpp
    src/utils/cryptography.cpp
    src/utils/enc_ind.cpp
    src/utils/tdag.cpp
    src/utils/utils.cpp
)
target_include_directories(main PUBLIC ${OPENSSL_INCLUDE_DIR})
target_link_libraries(main PUBLIC OpenSSL::Crypto)

include_directories(${PROJECT_SOURCE_DIR}/src/)

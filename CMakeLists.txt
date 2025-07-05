cmake_minimum_required(VERSION 3.16)
project(SSEImplementations LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED CONFIG)

add_executable(
    main
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
# base directories for `#include`
target_include_directories(main PUBLIC ${PROJECT_SOURCE_DIR}/src/ ${OPENSSL_INCLUDE_DIR})
target_link_libraries(main PUBLIC OpenSSL::Crypto)

cmake_minimum_required(VERSION 3.16)
project(SSEImplementations LANGUAGES CXX)
#set(CMAKE_CXX_COMPILER /usr/bin/g++) # change if needed! C++20 required
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED CONFIG)

add_executable(
    main
    src/main.cpp

    src/schemes/log_src/log_src.cpp

    src/schemes/log_src_i/log_src_i.cpp
    src/schemes/log_src_i/log_src_i_base.cpp

    src/schemes/log_src_i_star/log_src_i_star.cpp
    src/schemes/log_src_i_star/underly.cpp

    src/schemes/n_log_n/n_log_n.cpp
    src/schemes/n_log_n/n_log_n_server.cpp

    src/schemes/pi_bas/pi_bas.cpp
    src/schemes/pi_bas/pi_bas_server.cpp

    src/schemes/sda/sda.cpp

    src/utils/crypto.cpp
    src/utils/db/db_disk.cpp
    src/utils/db/db_interface.cpp
    src/utils/db/db_ram.cpp
    src/utils/debugging.cpp
    src/utils/disk_storage.cpp
    src/utils/enc_ind.cpp
    src/utils/ind.cpp
    src/utils/misc.cpp
    src/utils/range.cpp
    src/utils/tdag.cpp
    src/utils/tuple.cpp
    src/utils/ustring.cpp
)

# base directories for `#include`
target_include_directories(main PUBLIC ${PROJECT_SOURCE_DIR}/src/ ${OPENSSL_INCLUDE_DIR})
target_link_libraries(main PUBLIC OpenSSL::Crypto)

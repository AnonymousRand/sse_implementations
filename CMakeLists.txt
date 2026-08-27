cmake_minimum_required(VERSION 3.16)
project(SSEImplementations LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_COMPILER /usr/bin/g++-15) # change if needed! C++20 required

find_package(OpenSSL REQUIRED CONFIG)

add_executable(main
    src/main.cpp

    src/schemes/log_src/log_src.cpp
    src/schemes/log_src/log_src_utils.cpp

    src/schemes/log_src_i/log_src_i.cpp
    src/schemes/log_src_i/log_src_i_base.cpp
    src/schemes/log_src_i/log_src_i_utils.cpp

    src/schemes/log_src_i_star/log_src_i_star.cpp
    src/schemes/log_src_i_star/log_src_i_star_underly.cpp

    src/schemes/n_log_n/n_log_n.cpp
    src/schemes/n_log_n/n_log_n_base.cpp
    src/schemes/n_log_n/n_log_n_server.cpp
    src/schemes/n_log_n/n_log_n_base_server.cpp

    src/schemes/pi_bas/pi_bas.cpp
    src/schemes/pi_bas/pi_bas_server.cpp

    src/schemes/sda/sda.cpp

    src/utils/crypto.cpp
    src/utils/debugging.cpp
    src/utils/misc.cpp

    src/utils/types/db/db_disk.cpp
    src/utils/types/db/db_ram.cpp
    src/utils/types/db/i_db.cpp

    src/utils/types/enc_ind/enc_ind_base.cpp
    src/utils/types/enc_ind/enc_ind_rand.cpp
    src/utils/types/enc_ind/enc_ind_loc.cpp
    src/utils/types/enc_ind/enc_ind_types.cpp

    src/utils/types/i_disk_storage.cpp
    src/utils/types/ind.cpp
    src/utils/types/range.cpp
    src/utils/types/tdag.cpp
    src/utils/types/tuple.cpp
    src/utils/types/ustring.cpp
)

# base directories for `#include`
target_include_directories(main PUBLIC ${PROJECT_SOURCE_DIR}/src/ ${OPENSSL_INCLUDE_DIR})

target_link_libraries(main PUBLIC OpenSSL::Crypto)

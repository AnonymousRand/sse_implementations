#pragma once

#include "evp-encrypt.cpp"

#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>


typedef std::tuple<int, int>                          IdRange;
typedef std::tuple<int, int>                          KwRange;
typedef std::tuple<std::string, std::string>          QueryToken;
typedef std::unordered_map<int, int>                  Db;
typedef std::unordered_map<KwRange, std::vector<int>> Index;
typedef std::unordered_map<std::string, std::string>  EncIndex;

typedef std::basic_string<char, std::char_traits<char>, zallocator<char>> openSslStr;

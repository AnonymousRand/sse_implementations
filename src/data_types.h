#pragma once

#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

typedef int                                          Id;
typedef int                                          Kw;
typedef std::tuple<Id, Id>                           IdRange;
typedef std::tuple<Kw, Kw>                           KwRange;
typedef std::tuple<std::string, std::string>         QueryToken;
typedef std::unordered_map<Id, Kw>                   Db;
typedef std::unordered_map<KwRange, std::vector<Id>> Index;
typedef std::unordered_map<std::string, std::string> EncIndex;

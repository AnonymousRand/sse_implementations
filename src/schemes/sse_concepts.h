#pragma once

#include "utils/utils.h"

#include <concepts>


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
class ISse;


template <class T>
concept IsSse = requires(T t) {
    []<class ... Args>(ISse<Args ...>&){}(t);
};


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
class ISdaUnderlySse;


template <class T>
concept IsSdaUnderlySse = requires(T t) {
    []<class ... Args>(ISdaUnderlySse<Args ...>&){}(t);
};

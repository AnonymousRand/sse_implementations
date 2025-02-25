#include "sda.h"

void Sda::update(const DbEntry<DbDoc, DbKw>& newEntry) {
    
}

template class Sda<Id, Kw, PiBas>;
template class Sda<IdOp, Kw, PiBas>;

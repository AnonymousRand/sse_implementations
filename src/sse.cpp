#include "sse.h"

template <IDbDoc_ DbDoc, class DbKw>
void ISse<DbDoc, DbKw>::clear() {
    this->setup(0, Db<DbDoc, DbKw> {});
}

// base
template class ISse<Id, Kw>;
template class ISse<IdOp, Kw>;

// Log-SRC-i index 1
template class ISse<SrcIDb1Doc<Kw>, Kw>;

// Log-SRC-i index 2
template class ISse<Id, IdAlias>;
template class ISse<IdOp, IdAlias>;

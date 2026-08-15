#pragma once

#include <concepts>
#include <vector>

#include "schemes/interfaces/sd_underly.h"
#include "schemes/interfaces/sse.h"

#include "utils/db/db.h"
#include "utils/range.h"
#include "utils/tdag.h"
#include "utils/tuple.h"
#include "utils/types.h"


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>>>
class LogSrc : public ISdUnderly<Tuple<>> {
private:
    using DbKw = typename ISdUnderly<Tuple<>>::DbKw;

public:
    using ISdUnderly<Tuple<>>::ISdUnderly;

    ~LogSrc();

    //----------------------------------------------------------------------
    // `ISse`

    void setup(int secParam, const Db<Tuple<>>& db) override;
    std::vector<Tuple<>> search(
        const Range<Kw>& query, bool shouldCleanUpResults = true, bool isNaive = true
    ) const override;
    void clear() override;

    //----------------------------------------------------------------------
    // `ISdUnderly`

    void getDb(Db<Tuple<>>& ret) const override;

private:
    Underly<Tuple<>>* underly = new Underly<Tuple<>>(this->benchmark);
    TdagNode<Kw>* tdag = nullptr;
};

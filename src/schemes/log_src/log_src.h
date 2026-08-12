#pragma once

#include <concepts>
#include <vector>

#include "schemes/interfaces/sd_underly.h"
#include "schemes/interfaces/sse.h"

#include "utils/db.h"
#include "utils/range.h"
#include "utils/sse_utils.h"
#include "utils/tdag.h"


template <template <class ...> class Underly> requires IsSse<Underly<Record<>, Kw>>
class LogSrc : public ISdUnderly<Record<>, Kw> {
public:
    using ISdUnderly<Record<>, Kw>::ISdUnderly;

    ~LogSrc();

    //----------------------------------------------------------------------
    // `ISse`

    void setup(int secParam, const Db<Record<>, Kw>& db) override;
    std::vector<Record<>> search(
        const Range<Kw>& query, bool shouldCleanUpResults = true, bool isNaive = true
    ) const override;
    void clear() override;

    //----------------------------------------------------------------------
    // `ISdUnderly`

    void getDb(Db<Record<>, Kw>& ret) const override;

private:
    Underly<Record<>, Kw>* underly = new Underly<Record<>, Kw>(this->benchmark);
    TdagNode<Kw>* tdag = nullptr;
};

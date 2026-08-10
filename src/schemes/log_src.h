#pragma once

#include "schemes/interfaces/sd_underly.h"
#include "schemes/interfaces/sse.h"

#include "utils/tdag.h"


template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
class LogSrc : public ISdUnderly<Doc<>, Kw> {
public:
    using ISdUnderly<Doc<>, Kw>::ISdUnderly;

    ~LogSrc();

    //----------------------------------------------------------------------
    // `ISse`

    void setup(int secParam, const Db<Doc<>, Kw>& db) override;
    std::vector<Doc<>> search(
        const Range<Kw>& query, bool shouldCleanUpResults = true, bool isNaive = true
    ) const override;
    void clear() override;

    //----------------------------------------------------------------------
    // `ISdUnderly`

    void getDb(Db<Doc<>, Kw>& ret) const override;

private:
    Underly<Doc<>, Kw>* underly = new Underly<Doc<>, Kw>(this->benchmark);
    TdagNode<Kw>* tdag = nullptr;
};

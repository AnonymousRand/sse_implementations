/**
 * Indexes are abstractly a list of `std::pair<ustring, std::pair<ustring, ustring>>` entries,
 * each of which correspond to `std::pair<label, std::pair<encrypted doc, IV>>`.
 */


#pragma once


#include <cstdio>

#include "util.h"


/******************************************************************************/
/* `EncIndBase`                                                               */
/******************************************************************************/


// common code
class EncIndBase {
    protected:
        FILE* file = nullptr;
        std::string filename = "";

        void readValFromPos(ulong pos, std::pair<ustring, ustring>& ret) const;

    public:
        virtual ~EncIndBase();

        virtual void init(long size);
        void clear();

        void writeToPos(ulong pos, const ustring& label, const std::pair<ustring, ustring>& val);
};


/******************************************************************************/
/* `EncInd`                                                                   */
/******************************************************************************/


class EncInd : public EncIndBase {
    private:
        long size;

    public:
        void init(long size);
        void write(const ustring& label, const std::pair<ustring, ustring>& val);
        int find(const ustring& label, std::pair<ustring, ustring>& ret) const;
};


/******************************************************************************/
/* `EncIndLoc`                                                                */
/******************************************************************************/


template <class DbKw>
class EncIndLoc : public EncIndBase {
    public:
        void write(
            const ustring& label, const std::pair<ustring, ustring>& val,
            const Range<DbKw>& dbKwRange, long dbKwResCount, long rank, DbKw minDbKw, long bottomLevelSize
        );
        void find(
            const Range<DbKw>& dbKwRange, long dbKwResCount, long rank, DbKw minDbKw, long bottomLevelSize,
            std::pair<ustring, ustring>& ret
        ) const;

        /**
         * Returns the position in the file/index that the given keyword range goes (with Log-SRC-i* locality).
         *
         * Precondition:
         *     - `dbKwResCount` is a power of 2.
         *     - `bottomLevelSize` is a power of 2.
         */
        ulong map(
            const Range<DbKw>& dbKwRange, long dbKwResCount, long rank, DbKw minDbKw, long bottomLevelSize
        ) const;
};

/**
 * Indexes are abstractly a list of `std::pair<ustring, std::pair<ustring, ustring>>` entries,
 * each of which correspond to `std::pair<label, std::pair<encrypted doc, IV>>`.
 */


#pragma once


#include <cstdio>

#include "utils.h"


/******************************************************************************/
/* `EncIndBase`                                                               */
/******************************************************************************/


// common code
class EncIndBase {
    public:
        virtual ~EncIndBase();

        virtual void init(long size);
        void clear();

        /**
         * Returns:
         *     - `true` if the kv pair at `pos` is valid.
         *     - `false` if the kv pair at `pos` is the null kv pair.
         */
        bool readValFromPos(ulong pos, std::pair<ustring, ustring>& ret) const;

    protected:
        FILE* file = nullptr;
        std::string filename = "";

        void writeToPos(ulong pos, const ustring& label, const std::pair<ustring, ustring>& val);
};


/******************************************************************************/
/* `EncInd`                                                                   */
/******************************************************************************/


class EncInd : public EncIndBase {
    public:
        void init(long size);
        void write(const ustring& label, const std::pair<ustring, ustring>& val);

        /**
         * Returns:
         *     - `true` if `label` found.
         *     - `false` if `label` not found.
         */
        bool find(const ustring& label, std::pair<ustring, ustring>& ret) const;

    private:
        long size;
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

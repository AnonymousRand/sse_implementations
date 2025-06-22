#pragma once

#include <cstdio>
#include <cstdlib>

#include "util.h"

// indexes are abstractly a list of `std::pair<ustring, std::pair<ustring, ustring>>` entries
// each of which correspond to `std::pair<label/key, std::pair<encrypted doc, iv>>`

/******************************************************************************/
/* `IEncInd`                                                                  */
/******************************************************************************/

class IEncInd {
    public:
        // every `init()` MUST be followed by a `clear()` for memory freeing!!
        virtual void init(unsigned long size) = 0;
        virtual void write(ustring key, std::pair<ustring, ustring> val) = 0;
        virtual void flushWrite() = 0;
        virtual int find(ustring key, std::pair<ustring, ustring>& ret) const = 0; // returns error code if not found
        // clears up memory without completely destroying object (i.e. `init()` can be called again)
        // should be idempotent and safe to call without `init()` first as well
        virtual void clear() = 0;
};

// idk if enum in constructor or templates is better design for this polymorphism
// but templates seem more natural even if less clean esp through all the inheritance
template <class T> concept IEncInd_ = std::derived_from<T, IEncInd>;

/******************************************************************************/
/* `RamEncInd`                                                                */
/******************************************************************************/

// for storing in primary memory (essentially an `std::map`)
class RamEncInd : public IEncInd {
    private:
        std::map<ustring, std::pair<ustring, ustring>> map;

    public:
        void init(unsigned long size) override;
        void write(ustring key, std::pair<ustring, ustring> val) override;
        void flushWrite() override;
        int find(ustring key, std::pair<ustring, ustring>& ret) const override;
        void clear() override;
};

/******************************************************************************/
/* `DiskEncInd`                                                               */
/******************************************************************************/

// for storing in secondary memory
class DiskEncInd : public IEncInd {
    private:
        unsigned char* buf;
        FILE* file;
        unsigned long size;
        std::unordered_map<unsigned long, bool> isPosFilled;

    public:
        DiskEncInd();
        ~DiskEncInd();

        void init(unsigned long size) override;
        void write(ustring key, std::pair<ustring, ustring> val) override;
        // flush temporary buffer to disk and then free buffer
        void flushWrite() override;
        int find(ustring key, std::pair<ustring, ustring>& ret) const override;
        void clear() override;
};

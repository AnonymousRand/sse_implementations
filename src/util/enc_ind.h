#pragma once

#include <cstdio>

#include "util.h"


// indexes are abstractly a list of `std::pair<ustring, std::pair<ustring, ustring>>` entries
// each of which correspond to `std::pair<label, std::pair<encrypted doc, IV>>`


enum class EncIndType {
    RAM,
    DISK
};


/******************************************************************************/
/* `IEncInd`                                                                  */
/******************************************************************************/


class IEncInd {
    public:
        // apparently when deleting derived class objects via pointers to base class (e.g. polymorphism)
        // the base class needs to have a virtual destructor for the derived class' constructor to be called
        virtual ~IEncInd() = default;

        // every `init()` MUST be followed by a `reset()` for memory freeing!!
        virtual void init(unsigned long size) = 0;
        virtual void write(ustring label, std::pair<ustring, ustring> val) = 0;
        virtual void flushWrite() = 0;
        virtual int find(ustring label, std::pair<ustring, ustring>& ret) const = 0; // returns error code if not found
        // clear up memory without completely destroying object (i.e. `init()` can be called again)
        // should be idempotent and safe to call without `init()` first as well
        virtual void reset() = 0;
};


/******************************************************************************/
/* `RamEncInd`                                                                */
/******************************************************************************/


// for storing in primary memory (essentially an `std::map`)
class RamEncInd : public IEncInd {
    private:
        std::map<ustring, std::pair<ustring, ustring>> map;

    public:
        void init(unsigned long size) override;
        void write(ustring label, std::pair<ustring, ustring> val) override;
        void flushWrite() override;
        int find(ustring label, std::pair<ustring, ustring>& ret) const override;
        void reset() override;
};


/******************************************************************************/
/* `DiskEncInd`                                                               */
/******************************************************************************/


// for storing in secondary memory
class DiskEncInd : public IEncInd {
    private:
        FILE* file = nullptr;
        unsigned char* buf = nullptr;
        unsigned long size;
        std::string filename = "";
        std::unordered_map<unsigned long, bool> isPosFilled;

    public:
        ~DiskEncInd();

        void init(unsigned long size) override;
        void write(ustring label, std::pair<ustring, ustring> val) override;
        // flush temporary buffer to disk and then free buffer
        void flushWrite() override;
        int find(ustring label, std::pair<ustring, ustring>& ret) const override;
        void reset() override;
};

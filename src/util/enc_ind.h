#pragma once


#include <cstdio>

#include "util.h"


enum class EncIndType {
    RAM,
    DISK
};


/******************************************************************************/
/* `IEncInd`                                                                  */
/******************************************************************************/


// indexes are abstractly a list of `std::pair<ustring, std::pair<ustring, ustring>>` entries
// each of which correspond to `std::pair<label, std::pair<encrypted doc, IV>>`
class IEncInd {
    public:
        // apparently when deleting derived class objects via pointers to base class (e.g. polymorphism)
        // the base class needs to have a virtual destructor for the derived class' constructor to be called
        virtual ~IEncInd() = default;

        // every `init()` MUST be followed by a `clear()` for memory freeing!!
        virtual void init(ulong size) = 0;
        virtual void write(ustring label, std::pair<ustring, ustring> val) = 0;
        virtual int find(ustring label, std::pair<ustring, ustring>& ret) const = 0; // returns error code if not found

        /**
         * Free memory without fully destroying this object (so we can call `init()` again with the same object).
         * Should be idempotent and safe to call without `init()` first as well.
         */
        virtual void clear() = 0;
};


/******************************************************************************/
/* `EncIndRam`                                                                */
/******************************************************************************/


// for storing in primary memory (essentially an `std::map`)
class EncIndRam : public IEncInd {
    private:
        std::map<ustring, std::pair<ustring, ustring>> map;

    public:
        void init(ulong size) override;
        void write(ustring label, std::pair<ustring, ustring> val) override;
        int find(ustring label, std::pair<ustring, ustring>& ret) const override;
        void clear() override;
};


/******************************************************************************/
/* `EncIndDisk`                                                               */
/******************************************************************************/


// for storing in secondary memory
class EncIndDisk : public IEncInd {
    private:
        static const uchar nullKv[ENC_IND_KV_LEN];

        FILE* file = nullptr;
        ulong size;
        std::string filename = "";

    public:
        ~EncIndDisk();

        void init(ulong size) override;
        void write(ustring label, std::pair<ustring, ustring> val) override;
        int find(ustring label, std::pair<ustring, ustring>& ret) const override;
        void clear() override;
};

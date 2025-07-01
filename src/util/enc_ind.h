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


// classes implementing this should attempt to store documents pseudorandomly
// (`EncIndRam` and `EncIndDisk`)
class IEncInd {
    protected:
        long size;

    public:
        // apparently when deleting derived class objects via pointers to base class (e.g. polymorphism)
        // the base class needs to have a virtual destructor for the derived class' constructor to be called
        virtual ~IEncInd() = default;

        virtual void init(long size) = 0;
        virtual void write(const ustring& label, const std::pair<ustring, ustring>& val) = 0;
        virtual int find(const ustring& label, std::pair<ustring, ustring>& ret) const = 0; // returns error code if not found

        /**
         * Free memory without fully destroying this object (so we can call `init()` again with the same object).
         * 
         * Notes:
         *     - Should be idempotent and safe to call without `init()` first as well.
         */
        virtual void clear() = 0;
};


/******************************************************************************/
/* `IEncIndLoc`                                                               */
/******************************************************************************/


// classes implementing this should store documents with Log-SRC-i* locality
// (`EncIndLocRam` and `EncIndLocDisk`)
template <class DbKw>
class IEncIndLoc {
    public:
        virtual ~IEncIndLoc() = default;

        virtual void init(long size) = 0;
        virtual void write(
            const ustring& label, const std::pair<ustring, ustring>& val,
            const Range<DbKw>& dbKwRange, long dbKwResCount, long rank, DbKw minDbKw, long bottomLevelSize
        ) = 0;
        virtual void find(
            const Range<DbKw>& dbKwRange, long dbKwResCount, long rank, DbKw minDbKw, long bottomLevelSize,
            std::pair<ustring, ustring>& ret
        ) const = 0;
        virtual void clear() = 0;

        /**
         * Returns the position in the file/index that the given keyword range goes (with Log-SRC-i* locality).
         *
         * Precondition:
         *     - `dbKwResCount` is a power of 2.
         *     - `bottomLevelSize` is a power of 2.
         */
        virtual ulong map(
            const Range<DbKw>& dbKwRange, long dbKwResCount, long rank, DbKw minDbKw, long bottomLevelSize
        ) const;
};


/******************************************************************************/
/* `EncIndRamBase`                                                            */
/******************************************************************************/


// common code between classes that store encrypted indexes in primary memory
// (`EncIndRam` and `EncIndLocRam`)
class EncIndRamBase {
    protected:
        uchar* arr = nullptr;

        void writeToPos(ulong pos, const ustring& label, const std::pair<ustring, ustring>& val);
        void readValFromPos(ulong pos, std::pair<ustring, ustring>& ret) const;

    public:
        void initBase(long size);
        void clearBase();
};


/******************************************************************************/
/* `EncIndDiskBase`                                                           */
/******************************************************************************/


// common code between classes that store encrypted indexes in secondary memory
// (`EncIndDisk` and `EncIndLocDisk`)
class EncIndDiskBase {
    protected:
        FILE* file = nullptr;
        std::string filename = "";

        void writeToPos(ulong pos, const ustring& label, const std::pair<ustring, ustring>& val);
        void readValFromPos(ulong pos, std::pair<ustring, ustring>& ret) const;

    public:
        void initBase(long size);
        void clearBase();
};


/******************************************************************************/
/* `EncIndRam`                                                                */
/******************************************************************************/


class EncIndRam : public IEncInd, public EncIndRamBase {
    public:
        ~EncIndRam();

        void init(long size) override;
        void write(const ustring& label, const std::pair<ustring, ustring>& val) override;
        int find(const ustring& label, std::pair<ustring, ustring>& ret) const override;
        void clear() override;
};


/******************************************************************************/
/* `EncIndDisk`                                                               */
/******************************************************************************/


// for storing in secondary memory
class EncIndDisk : public IEncInd, public EncIndDiskBase {
    public:
        ~EncIndDisk();

        void init(long size) override;
        void write(const ustring& label, const std::pair<ustring, ustring>& val) override;
        int find(const ustring& label, std::pair<ustring, ustring>& ret) const override;
        void clear() override;
};


/******************************************************************************/
/* `EncIndLocRam`                                                             */
/******************************************************************************/


template <class DbKw>
class EncIndLocRam : public IEncIndLoc<DbKw>, public EncIndRamBase {
    public:
        ~EncIndLocRam();

        void init(long size) override;
        void write(
            const ustring& label, const std::pair<ustring, ustring>& val,
            const Range<DbKw>& dbKwRange, long dbKwResCount, long rank, DbKw minDbKw, long bottomLevelSize
        ) override;
        void find(
            const Range<DbKw>& dbKwRange, long dbKwResCount, long rank, DbKw minDbKw, long bottomLevelSize,
            std::pair<ustring, ustring>& ret
        ) const override;
        void clear() override;
};


/******************************************************************************/
/* `EncIndLocDisk`                                                            */
/******************************************************************************/


template <class DbKw>
class EncIndLocDisk : public IEncIndLoc<DbKw>, public EncIndDiskBase {
    public:
        ~EncIndLocDisk();

        void init(long size) override;
        void write(
            const ustring& label, const std::pair<ustring, ustring>& val,
            const Range<DbKw>& dbKwRange, long dbKwResCount, long rank, DbKw minDbKw, long bottomLevelSize
        ) override;
        void find(
            const Range<DbKw>& dbKwRange, long dbKwResCount, long rank, DbKw minDbKw, long bottomLevelSize,
            std::pair<ustring, ustring>& ret
        ) const override;
        void clear() override;
};

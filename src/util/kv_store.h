#pragma once

#include "util.h"

////////////////////////////////////////////////////////////////////////////////
// `IKvStore`
////////////////////////////////////////////////////////////////////////////////

template <class KeyType, class ValType>
class IKvStore {
    public:
        virtual void init(int maxSize) = 0;
        virtual void write(KeyType key, ValType val) = 0;
        virtual void flushWrite() = 0;
        // returns error code if not found
        virtual int find(KeyType key, ValType& ret) const = 0;
        virtual void clear() = 0;
};

////////////////////////////////////////////////////////////////////////////////
// `RamKvStore`
////////////////////////////////////////////////////////////////////////////////

// for storing in primary memory (essentially an `std::map`)
template <class KeyType, class ValType>
class RamKvStore : public IKvStore<KeyType, ValType> {
    private:
        std::map<KeyType, ValType> map;

    public:
        void init(int maxSize) override;
        void write(KeyType key, ValType val) override;
        void flushWrite() override;
        int find(KeyType key, ValType& ret) const override;
        void clear() override;
};

////////////////////////////////////////////////////////////////////////////////
// `DiskKvStore`
////////////////////////////////////////////////////////////////////////////////

// for storing in secondary memory
template <class KeyType, class ValType>
class DiskKvStore : public IKvStore<KeyType, ValType> {
    private:
        char* arr;
        int fd;

    public:
        void init(int maxSize) override;
        void write(KeyType key, ValType val) override;
        void flushWrite() override;
        int find(KeyType key, ValType& ret) const override;
        void clear() override;
};

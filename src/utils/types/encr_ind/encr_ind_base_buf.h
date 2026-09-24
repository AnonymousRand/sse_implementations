#pragma once

#include <concepts>
#include <cstdio>
#include <functional>
#include <string>

#include "utils/types/basic_types.h"
#include "utils/types/encr_ind/encr_ind_base.h"
#include "utils/types/ustring.h"


struct EncrIndBase::Buf {
public:
    // mainly for debugging/assertions
    friend class EncrIndBase;

    static const bigint NOT_IN_BUF;

    //----------------------------------------------------------------------
    // constructors/destructors

    Buf(
        bigint ENTRY_CAPACITY,
        FILE* file, const std::string& filename, bigint encIndCapacity, bigint entryLen
    );

    ~Buf();

    //----------------------------------------------------------------------
    // rule of five

    // all but copy constructor deleted since copy constructor should be the only one we need

    // copy constructor
    Buf(const Buf& other);

    // copy assignment operator
    Buf& operator =(const Buf& other) = delete;

    // move constructor
    Buf(Buf&& other) noexcept = delete;

    // move assignment operator
    Buf& operator =(Buf&& other) noexcept = delete;

    //----------------------------------------------------------------------
    // interface

    /**
     * returns: a pointer to the start of the *buffer* location where the entry is.
     * IMPORTANT: this points to the same memory as the buffer data does (i.e. no `memcpy()`s),
     * so do NOT allocate any new memory to hold it or free the returned value in the caller!!
     */
    uchar* read(bigint index) const;

    /**
     * note: this *does* `memcpy()` the data from `entry` into the buffer.
     */
    void write(bigint index, const uchar* entry);

    void fill(ubigint startPos, bool allowIncompleteFill = false);
    void flushIfNotFlushed() const;

    bigint posToBufIndex(ubigint pos) const;

private:
    const bigint ENTRY_CAPACITY;
    uchar* data = nullptr;
    ubigint startPos = 0;
    ubigint endPos = 0;
    bool isFilled = false;
    mutable bool isFlushed = true;

    // members shared with its parent encr ind (do not free these in `Buf`!!)
    FILE* file;
    const std::string& filename;
    const bigint encIndCapacity;
    const bigint entryLen;

    /**
     * helper for sharing code between `fill()` and `flush()`. (the template and the `static`
     * are needed for this to be usable in both the non-const `fill()` and the const `flush()`.)
     *
     * params:
     *     - `isRead`: set to `true` for reads and `false` for writes.
     */
    template <class SelfType> requires std::is_same_v<std::remove_cv_t<SelfType>, Buf>
    static void operOnFileBase(
        SelfType* self, const std::function<bigint(uchar*, bigint)>& oper, ubigint startPos
    );
};

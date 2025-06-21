#include "kv_store.h"

////////////////////////////////////////////////////////////////////////////////
// `RamKvStore`
////////////////////////////////////////////////////////////////////////////////

template <class KeyType, class ValType>
void RamKvStore<KeyType, ValType>::init(int maxSize) {}

template <class KeyType, class ValType>
void RamKvStore<KeyType, ValType>::write(KeyType key, ValType val) {
    this->map[key] = val;
}

template <class KeyType, class ValType>
void RamKvStore<KeyType, ValType>::flushWrite() {}

template <class KeyType, class ValType>
int RamKvStore<KeyType, ValType>::find(KeyType key, ValType& ret) const {
    auto iter = this->map.find(key);
    if (iter == this->map.end()) {
        return -1;
    }
    ret = iter->second;
    return 0;
}

template <class KeyType, class ValType>
void RamKvStore<KeyType, ValType>::clear() {
    this->map.clear();
}

template class RamKvStore<ustring, std::pair<ustring, ustring>>;

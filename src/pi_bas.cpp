#include <set>

#include "pi_bas.h"

template class PiBasClient<ustring, EncInd>;
template class PiBasServer<EncInd>;

template class PiBasClient<std::pair<ustring, ustring>, std::pair<EncInd, EncInd>>;
template class PiBasServer<std::pair<EncInd, EncInd>>;

////////////////////////////////////////////////////////////////////////////////
// PiBasClient
////////////////////////////////////////////////////////////////////////////////

template <typename KeyType, typename EncIndType>
PiBasClient<KeyType, EncIndType>::PiBasClient(Db db) : SseClient<KeyType, EncIndType>(db) {};

template <>
void PiBasClient<ustring, EncInd>::setup(int secParam) {
    unsigned char* key = new unsigned char[secParam];
    int res = RAND_priv_bytes(key, secParam);
    if (res != 1) {
        handleOpenSslErrors();
    }
    this->key = toUstr(key, secParam);
    delete[] key;
}

template <typename KeyType, typename EncIndType>
void PiBasClient<KeyType, EncIndType>::setup(int secParam) {}

template <>
EncInd PiBasClient<ustring, EncInd>::buildIndex(Db db) {
    EncInd encInd;

    // generate (plaintext) index of keywords to documents mapping and list of unique keywords
    std::map<KwRange, std::vector<Id>> index;
    std::set<KwRange> uniqueKwRanges;
    for (auto pair : db) {              // this breaks apart `std::unordered_multimap` buckets as intended
        Id id = pair.first;
        KwRange kwRange = pair.second;

        if (index.count(kwRange) == 0) {
            index[kwRange] = std::vector<Id> {id};
        } else {
            index[kwRange].push_back(id);
        }
        uniqueKwRanges.insert(kwRange); // `std::set` will not insert duplicate elements
    }

    // for each w in W
    for (KwRange kwRange : uniqueKwRanges) {
        // K_1 || K_2 <- F(K, w)
        ustring K = prf(this->key, toUstr(kwRange));
        int subkeyLen = K.length() / 2;
        ustring subkey1 = K.substr(0, subkeyLen);
        ustring subkey2 = K.substr(subkeyLen, subkeyLen);
        
        unsigned int counter = 0;
        // for each id in DB(w)
        auto docsWithKwRangeIt = index.find(kwRange);
        if (docsWithKwRangeIt == index.end()) {
            continue;
        }
        for (Id id : docsWithKwRangeIt->second) {
            // l <- F(K_1, c); d <- Enc(K_2, id)
            ustring label = prf(subkey1, toUstr(counter));
            ustring iv = genIv();
            ustring data = aesEncrypt(EVP_aes_256_cbc(), subkey2, toUstr(id), iv);
            counter++;
            // add (l, d) to list L (in lex order); we add straight to dictionary since we have ordered maps in C++
            // also store IV in plain along with encrypted value
            encInd[label] = std::pair<ustring, ustring> {data, iv};
        }
    }

    return encInd;
}

template <>
EncInd PiBasClient<ustring, EncInd>::buildIndex() {
    return this->buildIndex(this->db);
}

template <typename KeyType, typename EncIndType>
EncIndType PiBasClient<KeyType, EncIndType>::buildIndex() {
    return EncIndType(this->buildIndex(this->db)));
}

template <>
QueryToken PiBasClient<ustring, EncInd>::trpdr(KwRange kwRange) {
    // the paper uses different notation for the key generation here vs. in `setup()`;
    // I'm fairly sure they meant the same thing
    ustring K = prf(this->key, toUstr(kwRange));
    int subkeyLen = K.length() / 2;
    ustring subkey1 = K.substr(0, subkeyLen);
    ustring subkey2 = K.substr(subkeyLen, subkeyLen);
    return std::pair<ustring, ustring> {subkey1, subkey2};
}

template <typename KeyType, typename EncIndType>
QueryToken PiBasClient<KeyType, EncIndType>::trpdr(KwRange kwRange) {}

////////////////////////////////////////////////////////////////////////////////
// PiBasServer
////////////////////////////////////////////////////////////////////////////////

template <>
std::vector<int> PiBasServer<EncInd>::search(QueryToken queryToken) {
    std::vector<int> results;
    ustring subkey1 = queryToken.first;
    ustring subkey2 = queryToken.second;
    int counter = 0;
    
    // for c = 0 until `Get` returns error
    while (true) {
        // d <- Get(D, F(K_1, c))
        ustring label = prf(subkey1, toUstr(counter));
        auto it = this->encInd.find(label);
        if (it == this->encInd.end()) {
            break;
        }
        std::pair<ustring, ustring> encIndV = it->second;
        ustring data = encIndV.first;
        // id <- Dec(K_2, d)
        ustring iv = encIndV.second;
        ustring ptext = aesDecrypt(EVP_aes_256_cbc(), subkey2, data, iv);
        results.push_back(ustrToInt(ptext));

        counter++;
    }
    return results;
}

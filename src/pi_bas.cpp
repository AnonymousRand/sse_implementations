#include <unordered_set>

#include <openssl/rand.h>

#include "pi_bas.h"
#include "util/openssl.h"

////////////////////////////////////////////////////////////////////////////////
// Client
////////////////////////////////////////////////////////////////////////////////

ustring PiBasClient::setup(int secParam) {
    unsigned char* key = new unsigned char[secParam];
    int res = RAND_priv_bytes(key, secParam);
    if (res != 1) {
        handleOpenSslErrors();
    }
    ustring ustrKey = toUstr(key, secParam);
    delete[] key;
    return ustrKey;
}

template <typename DbDocType, typename DbKwType>
void PiBasClient::buildIndex(const ustring& key, const Db<DbDocType, DbKwType>& db, EncInd& encInd) {
    // generate (plaintext) index of keywords to documents/ids mapping and list of unique keywords
    // TODO why does using unordered_set make buildindex like twice as slow as set??? test again
    std::unordered_map<DbKwType, std::vector<DbDocType>> index;
    std::unordered_set<DbKwType> uniqueKws;
    for (auto entry : db) {
        DbDocType dbDoc = std::get<0>(entry);
        DbKwType dbKw = std::get<1>(entry);

        if (index.count(dbKw) == 0) {
            index[dbKw] = std::vector<DbDocType> {dbDoc};
        } else {
            index[dbKw].push_back(dbDoc);
        }
        uniqueKws.insert(dbKw); // `unordered_set` will not insert duplicate elements
    }

    // for each w in W
    for (DbKwType dbKw : uniqueKws) {
        // K_1 || K_2 <- F(K, w)
        ustring K = prf(key, toUstr(dbKw));
        int subkeyLen = K.length() / 2;
        ustring subkey1 = K.substr(0, subkeyLen);
        ustring subkey2 = K.substr(subkeyLen, subkeyLen);
        
        unsigned int counter = 0;
        // for each id in DB(w)
        auto itDocsWithSameKw = index.find(dbKw);
        for (DbDocType dbDoc : itDocsWithSameKw->second) {
            // l <- F(K_1, c); d <- Enc(K_2, id)
            ustring label = prf(subkey1, toUstr(counter));
            ustring iv = genIv();
            ustring data = aesEncrypt(EVP_aes_256_cbc(), subkey2, dbDoc.encode(), iv);
            counter++;
            // add (l, d) to list L (in lex order); we add straight to dictionary since we have ordered maps in C++
            // also store IV in plain along with encrypted value
            encInd[label] = std::pair<ustring, ustring> {data, iv};
        }
    }
}

template <typename RangeType>
QueryToken PiBasClient::trpdr(const ustring& key, const Range<RangeType>& range) {
    // the paper uses different notation for the key generation here vs. in `setup()`;
    // but I'm fairly sure they meant the same thing, otherwise it doesn't work
    ustring K = prf(key, toUstr(range));
    int subkeyLen = K.length() / 2;
    ustring subkey1 = K.substr(0, subkeyLen);
    ustring subkey2 = K.substr(subkeyLen, subkeyLen);
    return QueryToken {subkey1, subkey2};
}

template void PiBasClient::buildIndex(const ustring& key, const Db<>& db, EncInd& encInd);
template void PiBasClient::buildIndex(const ustring& key, const Db<SrciDb1Doc, KwRange>& db, EncInd& encInd);
template void PiBasClient::buildIndex(const ustring& key, const Db<Id, IdRange>& db, EncInd& encInd);

template QueryToken PiBasClient::trpdr(const ustring& key, const Range<Id>& range);
template QueryToken PiBasClient::trpdr(const ustring& key, const Range<Kw>& range);

////////////////////////////////////////////////////////////////////////////////
// Server
////////////////////////////////////////////////////////////////////////////////

template <typename DbDocType>
void PiBasServer::search(const EncInd& encInd, const QueryToken& queryToken, std::vector<DbDocType>& results) {
    ustring subkey1 = queryToken.first;
    ustring subkey2 = queryToken.second;
    int counter = 0;
    
    // for c = 0 until `Get` returns error
    while (true) {
        // d <- Get(D, F(K_1, c))
        ustring label = prf(subkey1, toUstr(counter));
        auto it = encInd.find(label);
        if (it == encInd.end()) {
            break;
        }
        std::pair<ustring, ustring> encIndV = it->second;
        ustring data = encIndV.first;
        // id <- Dec(K_2, d)
        ustring iv = encIndV.second;
        ustring ptext = aesDecrypt(EVP_aes_256_cbc(), subkey2, data, iv);
        results.push_back(DbDocType::decode(ptext));

        counter++;
    }
}

template void PiBasServer::search(const EncInd& encInd, const QueryToken& queryToken, std::vector<Id>& results);
template void PiBasServer::search(const EncInd& encInd, const QueryToken& queryToken, std::vector<SrciDb1Doc>& results);

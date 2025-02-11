#include "log_srci.h"

////////////////////////////////////////////////////////////////////////////////
// Util
////////////////////////////////////////////////////////////////////////////////

template class IEncryptable<std::pair<KwRange, IdRange>>;

SrciDb1DocType::SrciDb1DocType(KwRange kwRange, IdRange idRange)
        : IEncryptable<std::pair<KwRange, IdRange>>(std::pair<KwRange, IdRange> {kwRange, idRange}) {}

ustring SrciDb1DocType::toUstr() {
    std::string str = "(" + this->val.first + "," + this->val.second + ")";
    return ::toUstr(str);
}

std::pair<KwRange, IdRange> SrciDb1DocType::fromUstr(ustring ustr) {
    std::string str = ::fromUstr(ustr);
    std::regex re("\\((.*?),(.*?)\\)");
    std::smatch matches;
    if (!std::regex_search(str, matches, re) || matches.size() != 2) {
        std::cerr << "Error: bad string passed to `SrciDb1DocType.fromUstr()`, the world is going to end" << std::endl;
        exit(EXIT_FAILURE);
    }
    KwRange kwRange = KwRange::fromStr(matches[0].str());
    IdRange idRange = IdRange::fromStr(matches[1].str());
    return std::pair<KwRange, IdRange> {kwRange, idRange};
}

template ustring toUstr(IEncryptable<SrciDb1DocType>& srciDb1DocType);

////////////////////////////////////////////////////////////////////////////////
// Client
////////////////////////////////////////////////////////////////////////////////

LogSrciClient::LogSrciClient(PiBasClient underlying)
        : IRangeSseClient<std::pair<ustring, ustring>, std::pair<EncInd, EncInd>>(underlying) {}

std::pair<ustring, ustring> LogSrciClient::setup(int secParam) {
    unsigned char* key1 = new unsigned char[secParam];
    unsigned char* key2 = new unsigned char[secParam];
    int res1 = RAND_priv_bytes(key1, secParam);
    int res2 = RAND_priv_bytes(key2, secParam);
    if (res1 != 1 || res2 != 1) {
        handleOpenSslErrors();
    }

    ustring ustrKey1 = toUstr(key1, secParam);
    ustring ustrKey2 = toUstr(key2, secParam);
    delete[] key1, key2;
    return std::pair<ustring, ustring> {ustrKey1, ustrKey2};
}

std::pair<EncInd, EncInd> LogSrciClient::buildIndex(std::pair<ustring, ustring> key, Db<Id, KwRange> db) {
    ustring key1 = key.first;
    ustring key2 = key.second;

    // build TDAG1 over keywords
    Kw maxKw = -1;
    for (auto entry : db) {
        KwRange kwRange = std::get<1>(entry);
        if (kwRange.second > maxKw) {
            maxKw = kwRange.second;
        }
    }
    this->tdag1 = TdagNode<Kw>::buildTdag(maxKw);

    // first figure out which documents share the same keywords by building index and list of unique kws like in PiBas
    std::map<KwRange, std::set<Id>> index;
    std::set<KwRange> uniqueKwRanges;
    for (auto entry : db) {
        Id id = std::get<0>(entry);
        KwRange kwRange = std::get<1>(entry);

        if (index.count(kwRange) == 0) {
            index[kwRange] = std::set<Id> {id};
        } else {
            index[kwRange].insert(id);
        }
        uniqueKwRanges.insert(kwRange);
    }

    // replicate every document (in this case pairs) to all nodes/keywords ranges in TDAG1 that cover it
    // thus `db1` contains the (inverted) pairs used to build index 1: ((kw, id range), kw range/node)
    Db<SrciDb1DocType, KwRange> db1;
    for (KwRange kwRange : uniqueKwRanges) {
        auto itDocsWithSameKwRange = index.find(kwRange);
        std::set<Id> docsWithSameKwRange = itDocsWithSameKwRange->second;
        IdRange idRange {*docsWithSameKwRange.begin(), *docsWithSameKwRange.rbegin()}; // `set` moment
        SrciDb1DocType pair {kwRange, idRange};

        std::list<TdagNode<Kw>*> ancestors = this->tdag1->getLeafAncestors(kwRange);
        for (TdagNode<Kw>* ancestor : ancestors) {
            std::pair<SrciDb1DocType, KwRange> finalEntry {pair, ancestor->getRange()};
            db1.push_back(finalEntry);
        }
    }

    EncInd encInd1 = this->underlying.buildIndexGeneric<SrciDb1DocType, KwRange>(key1, db1);
}

QueryToken trpdr1(ustring key1, KwRange kwRange) {

}

QueryToken trpdr(ustring key2, std::vector<SrciDb1DocType> choices) {

}

////////////////////////////////////////////////////////////////////////////////
// Server
////////////////////////////////////////////////////////////////////////////////

LogSrciServer::LogSrciServer(PiBasServer underlying) : IRangeSseServer<std::pair<EncInd, EncInd>>(underlying) {}

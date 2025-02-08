#include <set>

#include "log_src.h"
#include "tdag.h"

////////////////////////////////////////////////////////////////////////////////
// LogSrcClient
////////////////////////////////////////////////////////////////////////////////

LogSrcClient::LogSrcClient(Db db) : PiBasClient(db) {};

EncIndex LogSrcClient::buildIndex() {
    EncIndex encIndex;

    // build TDAG over keywords
    // need to find largest keyword: we can't pass in all the keywords raw, as leaves need to be contiguous
    Kw maxKw = -1;
    for (auto pair : this->db) {
        KwRange kwRange = pair.second;
        if (kwRange.second > maxKw) {
            maxKw = kwRange.second;
        }
    }
    TdagNode* tdag = TdagNode::buildTdag(maxKw);
    std::cout << tdag << std::endl;

    // replicate
    Db dbWithReplications;
    for (
}

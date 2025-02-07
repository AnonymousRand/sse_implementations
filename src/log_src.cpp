#include <set>

#include "log_src.h"
#include "tree.h"

////////////////////////////////////////////////////////////////////////////////
// PiBasClient
////////////////////////////////////////////////////////////////////////////////

EncIndex LogSrcClient::buildIndex() {
    EncIndex encIndex;

    // build TDAG over keywords
    TdagNode* tdag = TdagNode::buildTdag(self->uniqueKwRanges);

    // replicate 
    Index index;
}

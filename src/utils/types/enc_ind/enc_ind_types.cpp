#include "utils/types/enc_ind/enc_ind_types.h"

#include "utils/types/ustring.h"


namespace utils::enc_ind {


ustring toUstr(const EncIndEntry& encIndEntry) {
    ustring key = encIndEntry.first;
    EncIndVal val = encIndEntry.second;
    return key + val.first + val.second;
}


} // namespace `utils::enc_ind`

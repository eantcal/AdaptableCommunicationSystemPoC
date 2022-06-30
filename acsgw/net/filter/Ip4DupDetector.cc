#include "IpPacketParser.h"


/* -------------------------------------------------------------------------- */

bool Ip4DupDetector::isADuplicated(const IpPacketParser &parser)
{
    const std::lock_guard<std::mutex> lock(_mutex);

    auto &dupTable_orderedByIdTable = dupTables[uint64_t(((uint64_t)parser.getU32SrcAddr()) << 32) |
                                                uint64_t(((uint64_t)parser.getU32DstAddr()) & 0x00000000ffffffffULL)];

    auto &dupTable = dupTable_orderedByIdTable.first;
    auto &orderedByIdTable = dupTable_orderedByIdTable.second;

    const auto pktId = makeUniqueId(parser);
    const auto dup = !dupTable.insert(pktId).second;

    if (dup)
    {
        orderedByIdTable.insert({++_pktcnt, pktId});

        if (orderedByIdTable.size() > DUP_HISTORY_LEN)
        {
            const auto &idToRemove = orderedByIdTable.begin()->second;

            dupTable.erase(idToRemove);

            orderedByIdTable.erase(orderedByIdTable.begin());
        }
    }

    return dup;
}
#pragma once

#include <stdint.h>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <mutex>
#include <ostream>
#include <arpa/inet.h>

/* -------------------------------------------------------------------------- */

class IpPacketParser
{
public:
    enum ProtocolNumbers
    {
        IP_PROTO_ICMP = 1,
        IP_PROTO_TCP = 6,
        IP_PROTO_UDP = 17,
    };

#pragma pack(push, 1)
    struct Ip4Header
    {
        uint8_t ver_hlen;   /* Header version and length (dwords). */
        uint8_t service;    /* Service type. */
        uint16_t length;    /* Length of datagram (bytes). */
        uint16_t ident;     /* Unique packet identification. */
        uint16_t fragment;  /* Flags; Fragment offset. */
        uint8_t timetolive; /* Packet time to live (in network). */
        uint8_t protocol;   /* Upper level protocol (UDP, TCP). */
        uint16_t checksum;  /* IP header checksum. */
        uint32_t src_addr;  /* Source IP address. */
        uint32_t dest_addr; /* Destination IP address. */
    };
#pragma pack(pop)

    IpPacketParser(const char *packetBytes, int size) : _bytes(packetBytes),
                                                        _size(size)
    {
    }

    uint16_t getLength() const noexcept
    {
        return ntohs(((Ip4Header *)_bytes)->length);
    }

    uint16_t getCheksum() const noexcept
    {
        return ntohs(((Ip4Header *)_bytes)->checksum);
    }

    uint16_t getIdent() const noexcept
    {
        return ntohs(((Ip4Header *)_bytes)->ident);
    }

    uint16_t getFragment() const noexcept
    {
        return ntohs(((Ip4Header *)_bytes)->fragment);
    }

    uint8_t getProtocol() const noexcept
    {
        return ((Ip4Header *)_bytes)->protocol & 0xff;
    }

    bool isTcp() const noexcept
    {
        return getProtocol() == IP_PROTO_TCP;
    }

    bool isUdp() const noexcept
    {
        return getProtocol() == IP_PROTO_UDP;
    }

    bool isIcmp() const noexcept
    {
        return getProtocol() == IP_PROTO_ICMP;
    }

    uint32_t getU32SrcAddr() const noexcept
    {
        return ntohl(((Ip4Header *)_bytes)->src_addr);
    }

    static inline std::string formatAddr(const uint32_t &ip) noexcept
    {
        char buf[20] = {0};
        snprintf(buf, sizeof(buf), "%i.%i.%i.%i", (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
        return buf;
    }

    uint32_t getU32DstAddr() const noexcept
    {
        return ntohl(((Ip4Header *)_bytes)->dest_addr);
    }

    std::string getSrcAddr() const noexcept
    {
        return formatAddr(getU32SrcAddr());
    }

    std::string getDstAddr() const noexcept
    {
        return formatAddr(getU32DstAddr());
    }

    void dump(std::ostream &os) const
    {
        char buf[256] = {0};
        snprintf(buf, sizeof(buf) - 1, "Proto=%i %s->%s Id=%08x Frg=%08x",
                 getProtocol(),
                 getSrcAddr().c_str(),
                 getDstAddr().c_str(),
                 getIdent(),
                 getFragment());
        os << buf;
    }

private:
    const char *_bytes;
    const int _size;
};

/* -------------------------------------------------------------------------- */

class Ip4DupDetector
{
public:
    using SrcDestPair = uint64_t;
    using OrderedByIdTable = std::map<uint64_t, uint64_t>;
    using DupTable = std::unordered_set<uint64_t>;

    using DupTables = std::unordered_map<SrcDestPair, std::pair<DupTable, OrderedByIdTable>>;

    static inline uint64_t makeUniqueId(const IpPacketParser &parser) noexcept
    {
        return (uint64_t(uint32_t(((uint32_t)parser.getIdent()) << 16UL) | (uint32_t(((uint32_t)parser.getFragment()) & 0x0000ffffUL))) << 32ULL) |
               (uint64_t(uint32_t(((uint32_t)parser.getLength()) << 16UL) | uint32_t(((uint32_t)parser.getCheksum()) & 0x0000ffffUL) & ((1ULL << 32ULL) - 1ULL)));
    }

    static constexpr int DUP_HISTORY_LEN = 10;

    bool isADuplicated(const IpPacketParser &parser);

    bool isADuplicated(const uint64_t & id) {
        const std::lock_guard<std::mutex> lock(_mutex);
        return _pktidset.insert(id).second == false;
    }

private:
    DupTables dupTables;
    std::mutex _mutex;
    uint64_t _pktcnt{0};
    std::unordered_set<uint64_t> _pktidset;
};
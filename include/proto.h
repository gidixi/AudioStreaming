#pragma once
#include <cstdint>
#include <cstddef>

namespace proto {
#pragma pack(push, 1)
struct AudioHeader {
    uint32_t client_id;
    uint32_t seq;
    uint64_t ts_ns;
};
#pragma pack(pop)

static constexpr int SAMPLE_RATE = 48000;
static constexpr int CHANNELS    = 1;
static constexpr int FRAME_MS    = 20;
static constexpr int SAMPLES_PER_FRAME = SAMPLE_RATE * FRAME_MS / 1000;
static constexpr int BYTES_PER_SAMPLE = 2;

#if defined(_WIN32)
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <arpa/inet.h>
#endif

inline uint64_t htobe64_u64(uint64_t x) {
#if defined(__APPLE__) || defined(__linux__)
    return htobe64(x);
#else
    return ((x & 0x00000000000000FFULL) << 56) |
           ((x & 0x000000000000FF00ULL) << 40) |
           ((x & 0x0000000000FF0000ULL) << 24) |
           ((x & 0x00000000FF000000ULL) << 8)  |
           ((x & 0x000000FF00000000ULL) >> 8)  |
           ((x & 0x0000FF0000000000ULL) >> 24) |
           ((x & 0x00FF000000000000ULL) >> 40) |
           ((x & 0xFF00000000000000ULL) >> 56);
#endif
}
inline uint64_t be64toh_u64(uint64_t x) { return htobe64_u64(x); }
} // namespace proto

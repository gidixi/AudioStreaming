#include "proto.h"
#include "audio_source.h"
#include "transport.h"
#include "opus_codec.h"

#include <chrono>
#include <vector>
#include <iostream>
#include <memory>
#include <cstdint>
#include <string>

#if defined(_WIN32)
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <arpa/inet.h>
#endif

using namespace std::chrono;

static uint64_t now_ns() {
    return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

int client_main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: opus_client <client_id> <server_ip> <port>\n";
        return 1;
    }
    uint32_t client_id = (uint32_t)std::stoul(argv[1]);
    std::string server_ip = argv[2];
    uint16_t port = (uint16_t)std::stoul(argv[3]);

#if defined(PLATFORM_DESKTOP)
    std::unique_ptr<IAudioSource> src(makePortAudioSource(proto::SAMPLE_RATE, proto::CHANNELS));
#elif defined(PLATFORM_ESP32)
    std::unique_ptr<IAudioSource> src(makeEsp32I2SSource(proto::SAMPLE_RATE, proto::CHANNELS));
#else
    #error "Select PLATFORM_DESKTOP or PLATFORM_ESP32"
#endif

    std::unique_ptr<ITransportSender> tx(makeUdpSender(server_ip, port));
    std::unique_ptr<OpusEncoderWrapper> enc(makeOpusEncoder(proto::SAMPLE_RATE, proto::CHANNELS, 32000)); // 32 kbps

    const int frame_samples = proto::SAMPLES_PER_FRAME; // 960
    std::vector<int16_t> pcm(frame_samples);
    std::vector<uint8_t> packet(sizeof(proto::AudioHeader) + 400);

    uint32_t seq = 0;
    while (true) {
        size_t got = src->read(pcm.data(), pcm.size());
        if (got != (size_t)frame_samples) continue;

        int nbytes = enc->encode(pcm.data(), frame_samples,
                                 packet.data() + sizeof(proto::AudioHeader),
                                 (int)(packet.size() - sizeof(proto::AudioHeader)));
        if (nbytes <= 0) continue;

        auto* h = (proto::AudioHeader*)packet.data();
        h->client_id = htonl(client_id);
        h->seq       = htonl(seq++);
        h->ts_ns     = proto::htobe64_u64(now_ns());

        size_t total = sizeof(proto::AudioHeader) + (size_t)nbytes;
        tx->send(packet.data(), total);
    }
}

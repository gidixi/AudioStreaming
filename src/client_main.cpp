#include "audio_source.h"
#include "opus_codec.h"
#include "transport.h"
#include "proto.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <memory>
#include <iostream>
#include <chrono>

#if defined(_WIN32)
  #include <winsock2.h>
  #include <ws2tcpip.h>
#elif defined(PLATFORM_ESP32)
  #include <lwip/inet.h>
#else
  #include <arpa/inet.h>
#endif

// Selezione della sorgente audio
#if defined(PLATFORM_ESP32)
  IAudioSource* makeEsp32I2SSource(int sample_rate, int channels);
  #define MAKE_DEFAULT_SOURCE  makeEsp32I2SSource
#else
  IAudioSource* makePortAudioSource(int sample_rate, int channels);
  #define MAKE_DEFAULT_SOURCE  makePortAudioSource
#endif

static inline uint64_t now_ns() {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

int client_main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << (argc>0?argv[0]:"opus_client")
                  << " <client_id> <server_ip> <port>\n";
        return 1;
    }

    const uint32_t client_id = static_cast<uint32_t>(std::strtoul(argv[1], nullptr, 10));
    const std::string server_ip = argv[2];
    const uint16_t port = static_cast<uint16_t>(std::strtoul(argv[3], nullptr, 10));

    try {
        // Sorgente audio
        std::unique_ptr<IAudioSource> src(MAKE_DEFAULT_SOURCE(proto::SAMPLE_RATE, proto::CHANNELS));

        // Trasporto
        std::unique_ptr<ITransportSender> tx(makeUdpSender(server_ip, port));

        // Opus encoder (32 kbps, 20 ms, mono)
        std::unique_ptr<OpusEncoderWrapper> enc(makeOpusEncoder(proto::SAMPLE_RATE, proto::CHANNELS, 32000));

        const int frame_samples = proto::SAMPLES_PER_FRAME; // 960 @ 48kHz, 20ms
        std::vector<int16_t> pcm(frame_samples);

        // Pacchetto: header + payload opus (400B è abbondante a 20ms, 32kbps)
        std::vector<uint8_t> packet(sizeof(proto::AudioHeader) + 400);

        uint32_t seq = 0;
        while (true) {
            // 1) cattura PCM 20ms
            size_t got = src->read(pcm.data(), pcm.size());
            if (got != (size_t)frame_samples) {
                // su desktop può capitare con buffer variabile; aspetta/continua
                continue;
            }

            // 2) encode Opus
            uint8_t* payload = packet.data() + sizeof(proto::AudioHeader);
            int max_payload = (int)(packet.size() - sizeof(proto::AudioHeader));
            int nbytes = enc->encode(pcm.data(), frame_samples, payload, max_payload);
            if (nbytes <= 0) {
                // frame drop, continua
                continue;
            }

            // 3) header
            auto* h = reinterpret_cast<proto::AudioHeader*>(packet.data());
            h->client_id = htonl(client_id);
            h->seq       = htonl(seq++);
            h->ts_ns     = proto::htobe64_u64(now_ns());

            // 4) invia
            size_t total = sizeof(proto::AudioHeader) + (size_t)nbytes;
            if (!tx->send(packet.data(), total)) {
                // opzionale: log o retry/backoff
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "client_main error: " << e.what() << "\n";
        return 2;
    }
}

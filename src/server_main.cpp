#include "proto.h"
#include "transport.h"
#include "opus_codec.h"
#include "sink.h"

#include <unordered_map>
#include <filesystem>
#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <cstdint>

#if defined(_WIN32)
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <arpa/inet.h>
#endif

struct ClientCtx {
    std::unique_ptr<OpusDecoderWrapper> dec;
    std::unique_ptr<IPcmSink>           sink;
    uint32_t last_seq = 0;
    bool has_seq = false;
};

int server_main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: opus_server <bind_ip> <port> <out_dir>\n";
        return 1;
    }
    std::string bind_ip = argv[1];
    uint16_t port = (uint16_t)std::stoul(argv[2]);
    std::string out_dir = argv[3];

#if __cpp_lib_filesystem
    std::filesystem::create_directories(out_dir);
#endif

    std::unique_ptr<ITransportReceiver> rx(makeUdpReceiver(bind_ip, port));

    std::unordered_map<uint32_t, ClientCtx> map;
    std::vector<uint8_t>  buf(1500);
    std::vector<int16_t>  pcm(proto::SAMPLES_PER_FRAME * 2);

    while (true) {
        std::string from_ip;
        uint16_t from_port = 0;
        size_t n = rx->recv(buf.data(), buf.size(), &from_ip, &from_port);
        if (n < sizeof(proto::AudioHeader)) continue;

        auto* h = (const proto::AudioHeader*)buf.data();
        uint32_t cid = ntohl(h->client_id);
        uint32_t seq = ntohl(h->seq);
        uint64_t ts  = proto::be64toh_u64(h->ts_ns);
        (void)ts;

        const uint8_t* payload = buf.data() + sizeof(proto::AudioHeader);
        int pay = (int)(n - sizeof(proto::AudioHeader));

        auto& ctx = map[cid];
        if (!ctx.dec) {
            ctx.dec.reset(makeOpusDecoder(proto::SAMPLE_RATE, proto::CHANNELS));
            std::string f = out_dir + "/client_" + std::to_string(cid) + ".pcm";
            ctx.sink.reset(makeFilePcmSink(f));
            std::cerr << "[new client] id=" << cid << " from=" << from_ip << ":" << from_port
                      << " file=" << f << "\n";
        }
        if (ctx.has_seq && seq != ctx.last_seq + 1) {
            std::cerr << "[warn] seq gap client=" << cid << " " << ctx.last_seq << "->" << seq << "\n";
        }
        ctx.last_seq = seq; ctx.has_seq = true;

        int samples = ctx.dec->decode(payload, pay, pcm.data(), (int)pcm.size());
        if (samples > 0) ctx.sink->write(pcm.data(), (size_t)samples);
    }
}

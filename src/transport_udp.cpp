#include "transport.h"

#include <cstring>
#include <memory>
#include <string>
#include <string>


#if defined(_WIN32)
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
#else
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <unistd.h>
#endif

namespace {

class UdpSender : public ITransportSender {
public:
    UdpSender(const std::string& ip, uint16_t port) {
#if defined(_WIN32)
        WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa);
#endif
        sock_ = ::socket(AF_INET, SOCK_DGRAM, 0);
        std::memset(&dst_, 0, sizeof(dst_));
        dst_.sin_family = AF_INET;
        dst_.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &dst_.sin_addr);
    }
    ~UdpSender() override {
#if defined(_WIN32)
        closesocket(sock_); WSACleanup();
#else
        ::close(sock_);
#endif
    }
    bool send(const uint8_t* data, size_t len) override {
#if defined(_WIN32)
        int s = ::sendto(sock_, (const char*)data, (int)len, 0, (sockaddr*)&dst_, sizeof(dst_));
        return s == (int)len;
#else
        ssize_t s = ::sendto(sock_, data, len, 0, (sockaddr*)&dst_, sizeof(dst_));
        return s == (ssize_t)len;
#endif
    }
private:
    int sock_{};
    sockaddr_in dst_{};
};

class UdpReceiver : public ITransportReceiver {
public:
    UdpReceiver(const std::string& ip, uint16_t port) {
#if defined(_WIN32)
        WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa);
#endif
        sock_ = ::socket(AF_INET, SOCK_DGRAM, 0);
        int yes = 1;
        setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes));
        std::memset(&addr_, 0, sizeof(addr_));
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);
        ::bind(sock_, (sockaddr*)&addr_, sizeof(addr_));
    }
    ~UdpReceiver() override {
#if defined(_WIN32)
        closesocket(sock_); WSACleanup();
#else
        ::close(sock_);
#endif
    }
    size_t recv(uint8_t* buf, size_t cap, std::string* from_ip, uint16_t* from_port) override {
        sockaddr_in src{}; socklen_t slen = sizeof(src);
#if defined(_WIN32)
        int n = ::recvfrom(sock_, (char*)buf, (int)cap, 0, (sockaddr*)&src, &slen);
        if (n <= 0) return 0;
#else
        ssize_t n = ::recvfrom(sock_, buf, cap, 0, (sockaddr*)&src, &slen);
        if (n <= 0) return 0;
#endif
        if (from_ip) {
            char ipstr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &src.sin_addr, ipstr, sizeof ipstr);
            *from_ip = ipstr;
        }
        if (from_port) *from_port = ntohs(src.sin_port);
        return (size_t)n;
    }
private:
    int sock_{};
    sockaddr_in addr_{};
};

} // namespace

ITransportSender*   makeUdpSender  (const std::string& ip, uint16_t port) { return new UdpSender(ip, port); }
ITransportReceiver* makeUdpReceiver(const std::string& ip, uint16_t port) { return new UdpReceiver(ip, port); }

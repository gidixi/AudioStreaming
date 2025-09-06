#pragma once
#include <string>
#if defined(_WIN32)
  #include <winsock2.h>
  #include <ws2tcpip.h>
#elif defined(PLATFORM_ESP32)
  #include <lwip/sockets.h>
  #include <lwip/inet.h>
  #include <lwip/netdb.h>
  #include <unistd.h>
#else
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <unistd.h>
#endif


class ITransportSender {
public:
    virtual ~ITransportSender() = default;
    virtual bool send(const uint8_t* data, size_t len) = 0;
};

class ITransportReceiver {
public:
    virtual ~ITransportReceiver() = default;
    virtual size_t recv(uint8_t* buf, size_t cap, std::string* from_ip, uint16_t* from_port) = 0;
};

ITransportSender*   makeUdpSender  (const std::string& ip, uint16_t port);
ITransportReceiver* makeUdpReceiver(const std::string& bind_ip, uint16_t port);

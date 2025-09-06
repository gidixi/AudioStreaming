#pragma once
#include <cstdint>
#include <string>
#include <cstddef>

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

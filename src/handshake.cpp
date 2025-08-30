#include "handshake.h"
#include <stdexcept>
#include <cstring>
#include <random>
#include <sstream>
#include <iomanip>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

// Helper: Convert bytes to hex string
static std::string to_hex(const std::string& data) {
    std::ostringstream oss;
    for (unsigned char c : data) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    }
    return oss.str();
}

std::string perform_handshake(
    const std::string& peer_ip,
    uint16_t peer_port,
    const std::string& info_hash,
    const std::string& peer_id
) {
    // Build handshake message
    std::string handshake;
    handshake += static_cast<char>(19); // length of protocol string
    handshake += "BitTorrent protocol";
    handshake += std::string(8, '\0'); // reserved
    handshake += info_hash;            // 20 bytes
    handshake += peer_id;              // 20 bytes

    // Connect to peer
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) throw std::runtime_error("Failed to create socket");

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(peer_port);
    if (inet_pton(AF_INET, peer_ip.c_str(), &addr.sin_addr) != 1) {
        close(sock);
        throw std::runtime_error("Invalid peer IP address");
    }
    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        throw std::runtime_error("Failed to connect to peer");
    }

    // Send handshake
    ssize_t sent = send(sock, handshake.data(), handshake.size(), 0);
    if (sent != (ssize_t)handshake.size()) {
        close(sock);
        throw std::runtime_error("Failed to send handshake");
    }

    // Receive handshake
    char recv_buf[68];
    ssize_t recvd = recv(sock, recv_buf, 68, MSG_WAITALL);
    close(sock);
    if (recvd != 68) throw std::runtime_error("Failed to receive handshake");

    // Extract peer id (last 20 bytes)
    std::string peer_id_received(recv_buf + 48, 20);
    return peer_id_received;
}
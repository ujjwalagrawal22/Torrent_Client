#pragma once
#include <string>

// Returns the peer id received from the peer (raw 20 bytes)
std::string perform_handshake(
    const std::string& peer_ip,
    uint16_t peer_port,
    const std::string& info_hash, // 20 raw bytes
    const std::string& peer_id    // 20 raw bytes
);
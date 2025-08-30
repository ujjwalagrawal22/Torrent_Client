#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct Peer {
    std::string ip;
    uint16_t port;
};

std::vector<Peer> get_peers_from_tracker(
    const std::string& announce_url,
    const std::string& info_hash_raw,
    const std::string& peer_id,
    int64_t file_length
);
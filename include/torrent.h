#ifndef TORRENT_H
#define TORRENT_H

#include <string>
#include <vector>
#include <cstdint>
#include "../lib/nlohmann/json.hpp" // Adjust path

using json = nlohmann::json;

namespace torrent {

    // Represents the "info" dictionary within a torrent file.
    struct Info {
        std::string name;
        int64_t length;
        int64_t piece_length;
        std::string pieces_hash_concat; // The raw concatenated SHA-1 hashes
         int num_pieces; // Add this
        json to_json() const; // For bencoding
    };

    // Represents the entire torrent file.
    struct Torrent {
        std::string announce_url;
        Info info;

        std::string info_hash_raw; // 20-byte raw SHA-1 hash
        std::string info_hash_hex; // 40-char hex representation
    };

    // Loads a torrent file from the given path and returns a populated Torrent struct.
    Torrent load_from_file(const std::string& filename);

} // namespace torrent

#endif // TORRENT_H
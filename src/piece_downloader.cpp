#include "piece_downloader.h"
#include "torrent.h"
#include "tracker.h"
#include "handshake.h"
#include "utils.h"
#include <fstream>
#include <vector>
#include <random>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

constexpr int BLOCK_SIZE = 16 * 1024;

static void send_message(int sock, uint8_t id, const std::vector<uint8_t>& payload = {}) {
    uint32_t len = htonl(payload.size() + 1);
    send(sock, &len, 4, 0);
    send(sock, &id, 1, 0);
    if (!payload.empty()) send(sock, payload.data(), payload.size(), 0);
}

static std::vector<uint8_t> recv_message(int sock, uint8_t& id) {
    uint32_t len_n;
    if (recv(sock, &len_n, 4, MSG_WAITALL) != 4) throw std::runtime_error("Failed to read length");
    uint32_t len = ntohl(len_n);
    if (len == 0) throw std::runtime_error("Keep-alive not supported");
    if (recv(sock, &id, 1, MSG_WAITALL) != 1) throw std::runtime_error("Failed to read id");
    std::vector<uint8_t> payload(len - 1);
    if (len > 1 && recv(sock, payload.data(), len - 1, MSG_WAITALL) != (ssize_t)(len - 1))
        throw std::runtime_error("Failed to read payload");
    return payload;
}

bool download_piece_to_file(
    const std::string& torrent_path,
    int piece_index,
    const std::string& output_path
) {
    // 1. Load torrent and get info
    auto t = torrent::load_from_file(torrent_path);

    // 2. Get peers
    std::string peer_id(20, '\0');
    std::random_device rd;
    for (int i = 0; i < 20; ++i) peer_id[i] = static_cast<char>(rd() % 256);
    auto peers = get_peers_from_tracker(t.announce_url, t.info_hash_raw, peer_id, t.info.length);
    if (peers.empty()) throw std::runtime_error("No peers found");

    // 3. Connect to first peer and handshake
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(peers[0].port);
    inet_pton(AF_INET, peers[0].ip.c_str(), &addr.sin_addr);
    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) throw std::runtime_error("Connect failed");

    // Handshake
    std::string handshake;
    handshake += static_cast<char>(19);
    handshake += "BitTorrent protocol";
    handshake += std::string(8, '\0');
    handshake += t.info_hash_raw;
    handshake += peer_id;
    if (send(sock, handshake.data(), handshake.size(), 0) != 68) throw std::runtime_error("Handshake send failed");
    char recv_buf[68];
    if (recv(sock, recv_buf, 68, MSG_WAITALL) != 68) throw std::runtime_error("Handshake recv failed");

    // 4. Wait for bitfield (id=5)
    uint8_t id;
    recv_message(sock, id); // bitfield

    // 5. Send interested (id=2)
    send_message(sock, 2);

    // 6. Wait for unchoke (id=1)
    do { recv_message(sock, id); } while (id != 1);

    // 7. Request blocks
    int piece_len = (piece_index == t.info.num_pieces - 1)
        ? t.info.length - (t.info.piece_length * (t.info.num_pieces - 1))
        : t.info.piece_length;
    std::vector<uint8_t> piece_data(piece_len);
    int offset = 0;
    while (offset < piece_len) {
        int req_len = std::min(BLOCK_SIZE, piece_len - offset);
        std::vector<uint8_t> payload(12);
        uint32_t idx_n = htonl(piece_index);
        uint32_t begin_n = htonl(offset);
        uint32_t len_n = htonl(req_len);
        std::memcpy(payload.data(), &idx_n, 4);
        std::memcpy(payload.data() + 4, &begin_n, 4);
        std::memcpy(payload.data() + 8, &len_n, 4);
        send_message(sock, 6, payload);

        // Wait for piece (id=7)
        do { payload = recv_message(sock, id); } while (id != 7);
        int resp_idx = ntohl(*reinterpret_cast<uint32_t*>(payload.data()));
        int resp_begin = ntohl(*reinterpret_cast<uint32_t*>(payload.data() + 4));
        std::memcpy(piece_data.data() + resp_begin, payload.data() + 8, payload.size() - 8);
        offset += req_len;
    }
    close(sock);

    // 8. Write to file
    std::ofstream ofs(output_path, std::ios::binary);
    ofs.write(reinterpret_cast<const char*>(piece_data.data()), piece_data.size());
    return true;
}

//downloaing the whole file piece by piece
bool download_file(
    const std::string& torrent_path,
    const std::string& output_path
) {
    auto t = torrent::load_from_file(torrent_path);

    // Generate peer_id
    std::string peer_id(20, '\0');
    std::random_device rd;
    for (int i = 0; i < 20; ++i) peer_id[i] = static_cast<char>(rd() % 256);

    // Get peers
    auto peers = get_peers_from_tracker(t.announce_url, t.info_hash_raw, peer_id, t.info.length);
    if (peers.empty()) throw std::runtime_error("No peers found");

    // Connect to first peer and handshake
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(peers[0].port);
    inet_pton(AF_INET, peers[0].ip.c_str(), &addr.sin_addr);
    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) throw std::runtime_error("Connect failed");

    // Handshake
    std::string handshake;
    handshake += static_cast<char>(19);
    handshake += "BitTorrent protocol";
    handshake += std::string(8, '\0');
    handshake += t.info_hash_raw;
    handshake += peer_id;
    if (send(sock, handshake.data(), handshake.size(), 0) != 68) throw std::runtime_error("Handshake send failed");
    char recv_buf[68];
    if (recv(sock, recv_buf, 68, MSG_WAITALL) != 68) throw std::runtime_error("Handshake recv failed");

    // Wait for bitfield (id=5)
    uint8_t id;
    recv_message(sock, id); // bitfield

    // Send interested (id=2)
    send_message(sock, 2);

    // Wait for unchoke (id=1)
    do { recv_message(sock, id); } while (id != 1);

    // Prepare output file
    std::ofstream ofs(output_path, std::ios::binary);
    if (!ofs) throw std::runtime_error("Failed to open output file");

    // Download all pieces
    for (int piece_index = 0; piece_index < t.info.num_pieces; ++piece_index) {
        int piece_len = (piece_index == t.info.num_pieces - 1)
            ? t.info.length - (t.info.piece_length * (t.info.num_pieces - 1))
            : t.info.piece_length;
        std::vector<uint8_t> piece_data(piece_len);
        int offset = 0;
        while (offset < piece_len) {
            int req_len = std::min(BLOCK_SIZE, piece_len - offset);
            std::vector<uint8_t> payload(12);
            uint32_t idx_n = htonl(piece_index);
            uint32_t begin_n = htonl(offset);
            uint32_t len_n = htonl(req_len);
            std::memcpy(payload.data(), &idx_n, 4);
            std::memcpy(payload.data() + 4, &begin_n, 4);
            std::memcpy(payload.data() + 8, &len_n, 4);
            send_message(sock, 6, payload);

            // Wait for piece (id=7)
            do { payload = recv_message(sock, id); } while (id != 7);
            int resp_idx = ntohl(*reinterpret_cast<uint32_t*>(payload.data()));
            int resp_begin = ntohl(*reinterpret_cast<uint32_t*>(payload.data() + 4));
            std::memcpy(piece_data.data() + resp_begin, payload.data() + 8, payload.size() - 8);
            offset += req_len;
        }
        // Verify piece hash
        std::string hash = utils::sha1_hash(std::string(reinterpret_cast<char*>(piece_data.data()), piece_data.size()));
        std::string expected = t.info.pieces_hash_concat.substr(piece_index * 20, 20);
        if (hash != expected) throw std::runtime_error("Piece hash mismatch at index " + std::to_string(piece_index));
        ofs.write(reinterpret_cast<const char*>(piece_data.data()), piece_data.size());
    }
    close(sock);
    ofs.close();
    return true;
}
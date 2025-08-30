#include "tracker.h"
#include "bencode.h"
#include <curl/curl.h>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <arpa/inet.h> // For ntohs
#include <iostream>

// Helper: URL-encode a string (for info_hash and peer_id)
static std::string url_encode(const std::string& s) {
    std::ostringstream oss;
    for (unsigned char c : s) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            oss << c;
        } else {
            oss << '%' << std::uppercase << std::setw(2) << std::setfill('0') << std::hex << (int)c;
        }
    }
    return oss.str();
}

// Helper: libcurl write callback
static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::vector<Peer> get_peers_from_tracker(
    const std::string& announce_url,
    const std::string& info_hash_raw,
    const std::string& peer_id,
    int64_t file_length
) {
    // Build query string
    std::ostringstream url;
    url << announce_url
        << "?info_hash=" << url_encode(info_hash_raw)
        << "&peer_id=" << url_encode(peer_id)
        << "&port=6881"
        << "&uploaded=0"
        << "&downloaded=0"
        << "&left=" << file_length
        << "&compact=1";

    // Make HTTP GET request
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("Failed to init curl");

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) throw std::runtime_error("Tracker request failed");

    // Print the raw tracker response for debugging
std::cout << "Raw tracker response: " << response << std::endl;


    // Parse bencoded response
    auto dict = bencode::decode(response);
    if (!dict.contains("peers")) throw std::runtime_error("No peers in tracker response");

    std::string peers_str = dict["peers"].get<std::string>();
    std::vector<Peer> peers;
    for (size_t i = 0; i + 6 <= peers_str.size(); i += 6) {
        uint32_t ip_raw;
        uint16_t port_raw;
        std::memcpy(&ip_raw, peers_str.data() + i, 4);
        std::memcpy(&port_raw, peers_str.data() + i + 4, 2);
        ip_raw = ntohl(ip_raw);
        port_raw = ntohs(port_raw);

        std::ostringstream ip;
        ip << ((ip_raw >> 24) & 0xFF) << "."
           << ((ip_raw >> 16) & 0xFF) << "."
           << ((ip_raw >> 8) & 0xFF) << "."
           << (ip_raw & 0xFF);

        peers.push_back(Peer{ip.str(), port_raw});
    }
    return peers;
}
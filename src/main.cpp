#include <iostream>
#include <string>
#include <vector>
#include "tracker.h"
#include "bencode.h" // For the 'decode' command
#include "torrent.h" // For torrent-related commands
#include "utils.h"
#include "handshake.h"
#include <sstream>
#include <random>
#include "piece_downloader.h"
using namespace std;

void print_full_info(const string& torrent_file_path) {
    try {
        torrent::Torrent t = torrent::load_from_file(torrent_file_path);

        cout << "Tracker URL: " << t.announce_url << endl;
        cout << "Length: " << t.info.length << endl;
        cout << "Info Hash: " << t.info_hash_hex << endl;
        cout << "Piece Length: " << t.info.piece_length << endl;
        cout << "Piece Hashes:" << endl;

        // The pieces string is a concatenation of 20-byte SHA-1 hashes
        for (size_t i = 0; i < t.info.pieces_hash_concat.length(); i += 20) {
            string piece_hash = t.info.pieces_hash_concat.substr(i, 20);
            cout << utils::to_hex(
                reinterpret_cast<const unsigned char*>(piece_hash.data()),
                20
            ) <<endl;
        }

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        exit(1);
    }
}

int main(int argc, char* argv[]) {
    // Flush after every cout / cerr
    cout << unitbuf;
    cerr << unitbuf;

    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <command> [args]" << endl;
        return 1;
    }

    string command = argv[1];

    if (command == "decode") {
        if (argc < 3) {
            cerr << "Usage: " << argv[0] << " decode <bencoded_string>" <<endl;
            return 1;
        }
        try {
            json decoded_value = bencode::decode(argv[2]);
             cout<< decoded_value.dump() <<endl;
        } catch (const exception& e) {
            cerr << "Error decoding value: " << e.what() << endl;
            return 1;
        }
    } else if (command == "info") {
        if (argc < 3) {
            cerr << "Usage: " << argv[0] << " info <torrent_file>" << endl;
            return 1;
        }
        print_full_info(argv[2]);
    } else if (command == "peers") {
        if (argc < 3) {
            cerr << "Usage: " << argv[0] << " peers <torrent_file_path>" << endl;
            return 1;
        }
        torrent::Torrent t = torrent::load_from_file(argv[2]);
        // Use a random or fixed 20-byte peer_id for now
        string peer_id = "-PC0001-123456789012";
        auto peers = get_peers_from_tracker(
            t.announce_url,
            t.info_hash_raw, // must be the raw 20-byte string, not hex
            peer_id,
            t.info.length
        );
        cout << "Peers:" << endl;
        for (const auto& peer : peers) {
            cout << peer.ip << ":" << peer.port << endl;
        }
    }
    else if (command == "handshake") {
    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " handshake <torrent_file_path> <peer_ip>:<peer_port>" << endl;
        return 1;
    }
    torrent::Torrent t = torrent::load_from_file(argv[2]);
    // Parse <peer_ip>:<peer_port>
    string peer_arg = argv[3];
    size_t colon = peer_arg.find(':');
    if (colon == string::npos) {
        cerr << "Invalid peer address format." << endl;
        return 1;
    }
    string peer_ip = peer_arg.substr(0, colon);
    uint16_t peer_port = stoi(peer_arg.substr(colon + 1));

    // Generate random 20-byte peer_id
    string my_peer_id(20, '\0');
    random_device rd;
    for (int i = 0; i < 20; ++i) my_peer_id[i] = static_cast<char>(rd() % 256);

    try {
        string peer_id_recv = perform_handshake(peer_ip, peer_port, t.info_hash_raw, my_peer_id);
        // Print as hex
        ostringstream oss;
        for (unsigned char c : peer_id_recv) {
            oss << hex << setw(2) << setfill('0') << (int)c;
        }
       cout << "Peer ID: " << oss.str() << endl;
    } catch (const exception& e) {
        cerr << "Handshake failed: " << e.what() << endl;
        return 1;
    }
    }
    else if (command == "download_piece") {
    string output_path;
    int argi = 2;
    if (argc > 2 && string(argv[2]) == "-o") {
        output_path = argv[3];
        argi += 2;
    }
    if (argc <= argi + 1) {
        cerr << "Usage: " << argv[0] << " download_piece -o <output_path> <torrent_file> <piece_index>" << std::endl;
        return 1;
    }
    string torrent_path = argv[argi];
    int piece_index = stoi(argv[argi + 1]);
    try {
        if (download_piece_to_file(torrent_path, piece_index, output_path)) {
            cout << "Piece downloaded successfully." << endl;
        }
    } catch (const exception& e) {
        cerr << "Download failed: " << e.what() << endl;
        return 1;
    }
    }

    else if (command == "download") {
    string output_path;
    int argi = 2;
    if (argc > 2 && std::string(argv[2]) == "-o") {
        output_path = argv[3];
        argi += 2;
    }
    if (argc <= argi) {
        cerr << "Usage: " << argv[0] << " download -o <output_path> <torrent_file>" << endl;
        return 1;
    }
    string torrent_path = argv[argi];
    try {
        if (download_file(torrent_path, output_path)) {
           cout << "File downloaded successfully." << endl;
        }
    } catch (const exception& e) {
        cerr << "Download failed: " << e.what() << endl;
        return 1;
    }
    }

    else {
        cerr << "unknown command: " << command << endl;
        return 1;
    }

    return 0;
}
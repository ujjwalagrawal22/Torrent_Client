#include "torrent.h"
#include "utils.h"   // We use our utils for file reading and hashing
#include "bencode.h" // And our bencode module for decoding
#include <iostream>
#include <stdexcept>
#include <vector>

namespace torrent
{

    json Info::to_json() const
    {
        return json{
            {"name", this->name},
            {"length", this->length},
            {"piece length", this->piece_length},
            {"pieces", this->pieces_hash_concat}};
    }

    Torrent load_from_file(const std::string &filename)
    {
        std::string file_content = utils::read_file_as_binary_string(filename);
        json torrent_json = bencode::decode(file_content);

        if (!torrent_json.is_object())
        {
            throw std::runtime_error("Invalid torrent file: root is not a dictionary.");
        }

        Torrent t;
        t.announce_url = torrent_json.at("announce").get<std::string>();

        const json &info_json = torrent_json.at("info");
        t.info.name = info_json.at("name").get<std::string>();

        // Handle both single-file and multi-file torrents
        if (info_json.contains("length"))
        {
            t.info.length = info_json.at("length").get<int64_t>();
        }
        else if (info_json.contains("files"))
        {
            // Multi-file: sum the lengths
            t.info.length = 0;
            for (const auto &file : info_json.at("files"))
            {
                t.info.length += file.at("length").get<int64_t>();
            }
        }
        else
        {
            throw std::runtime_error("No length or files in torrent info");
        }

        //t.info.length = info_json.at("length").get<int64_t>();
        t.info.piece_length = info_json.at("piece length").get<int64_t>();
        t.info.pieces_hash_concat = info_json.at("pieces").get<std::string>();
        t.info.num_pieces = t.info.pieces_hash_concat.size() / 20;

        std::cout << "Number of pieces: " << t.info.num_pieces << std::endl;

        // Calculate the info hash
        std::string bencoded_info = bencode::encode(info_json);
        t.info_hash_raw = utils::sha1_hash(bencoded_info);
        t.info_hash_hex = utils::to_hex(
            reinterpret_cast<const unsigned char *>(t.info_hash_raw.data()),
            t.info_hash_raw.length());

        return t;
    }

} // namespace torrent
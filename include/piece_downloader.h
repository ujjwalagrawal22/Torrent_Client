#pragma once
#include <string>
#include <cstdint>

bool download_piece_to_file(
    const std::string& torrent_path,
    int piece_index,
    const std::string& output_path
);

bool download_file(
    const std::string& torrent_path,
    const std::string& output_path
);
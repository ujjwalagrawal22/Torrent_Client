#ifndef BENCODE_H
#define BENCODE_H

#include <string>
#include "../lib/nlohmann/json.hpp" // Adjust path as needed

using json = nlohmann::json;

namespace bencode {

    // Decodes a bencoded string into a json object.
    json decode(const std::string& bencoded_string);

    // Encodes a json object into a bencoded string.
    std::string encode(const json& value);

} // namespace bencode

#endif // BENCODE_H
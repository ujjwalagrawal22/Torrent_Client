#include "bencode.h"
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <cctype>

namespace bencode {

// Forward declaration for the recursive helper
static size_t decode_internal(const std::string& val, size_t pos, json& out);

json decode(const std::string& bencoded_string) {
    json result;
    decode_internal(bencoded_string, 0, result);
    return result;
}

std::string encode(const json& value) {
    if (value.is_string()) {
        const std::string& s = value.get<std::string>();
        return std::to_string(s.size()) + ":" + s;
    } else if (value.is_number_integer()) {
        return "i" + std::to_string(value.get<int64_t>()) + "e";
    } else if (value.is_array()) {
        std::string out = "l";
        for (const auto& elem : value) {
            out += encode(elem);
        }
        out += "e";
        return out;
    } else if (value.is_object()) {
        std::string out = "d";
        std::vector<std::string> keys;
        for (auto it = value.begin(); it != value.end(); ++it) {
            keys.push_back(it.key());
        }
        std::sort(keys.begin(), keys.end());
        for (const auto& key : keys) {
            out += std::to_string(key.size()) + ":" + key;
            out += encode(value.at(key));
        }
        out += "e";
        return out;
    }
    throw std::runtime_error("Unsupported type for bencoding");
}


// --- Implementation of the decoder ---

static size_t decode_internal(const std::string& val, size_t pos, json& out) {
    if (pos >= val.size())
        throw std::runtime_error("Unexpected end of input");

    if (isdigit(val[pos])) {
        size_t colon_index = val.find(':', pos);
        if (colon_index == std::string::npos)
            throw std::runtime_error("Invalid string encoding");

        int64_t len = stoll(val.substr(pos, colon_index - pos));
        size_t str_start = colon_index + 1;
        if (str_start + len > val.size())
            throw std::runtime_error("String length out of bounds");
        out = val.substr(str_start, len);
        return str_start + len;
    } else if (val[pos] == 'i') {
        size_t end_index = val.find('e', pos);
        if (end_index == std::string::npos)
            throw std::runtime_error("Invalid integer encoding");
        out = stoll(val.substr(pos + 1, end_index - pos - 1));
        return end_index + 1;
    } else if (val[pos] == 'l') {
        json arr = json::array();
        size_t cur = pos + 1;
        while (cur < val.size() && val[cur] != 'e') {
            json elem;
            cur = decode_internal(val, cur, elem);
            arr.push_back(elem);
        }
        if (cur >= val.size() || val[cur] != 'e')
            throw std::runtime_error("List not terminated properly");
        out = arr;
        return cur + 1;
    } else if (val[pos] == 'd') {
        json obj = json::object();
        size_t cur = pos + 1;
        while (cur < val.size() && val[cur] != 'e') {
            json key_json;
            size_t key_end = decode_internal(val, cur, key_json);
            if (!key_json.is_string())
                throw std::runtime_error("Dictionary key is not a string");

            json value_json;
            size_t value_end = decode_internal(val, key_end, value_json);
            obj[key_json.get<std::string>()] = value_json;
            cur = value_end;
        }
        if (cur >= val.size() || val[cur] != 'e')
            throw std::runtime_error("Dictionary not terminated properly");
        out = obj;
        return cur + 1;
    } else {
        throw std::runtime_error("Unhandled encoded value at pos " + std::to_string(pos));
    }
}

} // namespace bencode
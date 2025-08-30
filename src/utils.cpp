#include "utils.h" // Note: we include our own header
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

namespace utils {

    std::string read_file_as_binary_string(const std::string& filename) {
        // The `ios::binary` flag is important
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Failed to open file: " + filename);
        }
        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    std::string to_hex(const unsigned char* data, size_t len) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (size_t i = 0; i < len; ++i) {
            ss << std::setw(2) << static_cast<int>(data[i]);
        }
        return ss.str();
    }

    std::string sha1_hash(const std::string& data) {
        unsigned char hash[SHA_DIGEST_LENGTH];
        SHA1(reinterpret_cast<const unsigned char*>(data.data()), data.size(), hash);
        return std::string(reinterpret_cast<char*>(hash), SHA_DIGEST_LENGTH);
    }

} // namespace utils
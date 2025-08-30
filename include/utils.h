#ifndef UTILS_H
#define UTILS_H

#include <string>

// A namespace helps prevent naming conflicts
namespace utils {

    // Reads an entire file into a string.
    std::string read_file_as_binary_string(const std::string& filename);

    // Converts a block of data to its hexadecimal string representation.
    std::string to_hex(const unsigned char* data, size_t len);

    // Computes the SHA-1 hash of a string.
    std::string sha1_hash(const std::string& data);

} // namespace utils

#endif // UTILS_H
cmake_minimum_required(VERSION 3.13)
project(codecrafters-bittorrent)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the OpenSSL library, which provides cryptography functions
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

# --- Define our executable and its source files ---
# We list only the .cpp files. The compiler finds the .h files
# via the include directories specified below.
add_executable(bittorrent
    src/main.cpp
    src/bencode.cpp
    src/torrent.cpp
    src/utils.cpp
    src/tracker.cpp
    src/handshake.cpp
    src/piece_downloader.cpp
)

# --- Configure Target-Specific Properties ---

# Tell our 'bittorrent' target where to find its header files.
# The compiler will now look in 'include/' and 'lib/'.
target_include_directories(bittorrent PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/lib
)

# Link our 'bittorrent' target against the OpenSSL libraries.
target_link_libraries(bittorrent PRIVATE OpenSSL::SSL OpenSSL::Crypto CURL::libcurl)
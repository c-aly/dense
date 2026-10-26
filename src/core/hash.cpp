// src/core/hash.cpp
#include "core/hash.hpp"
#include <xxhash.h>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <vector>


namespace {
    std::string to_hex(XXH128_hash_t h) {   // hash to text
        char buf[33];
        std::snprintf(buf, sizeof buf, "%016llx%016llx",
            (unsigned long long)h.high64, (unsigned long long)h.low64);
        return buf;
    }
}


// hash string or file
std::string hash_bytes(std::string_view data) {
    return to_hex(XXH3_128bits(data.data(), data.size()));
}

std::string hash_file(const std::filesystem::path& file) {
    // open fstream
    std::ifstream in(file, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open " + file.string());

    // hash up to 64KB
    XXH3_state_t* state = XXH3_createState();
    XXH3_128bits_reset(state);
    std::vector<char> buf(1 << 16);
    while (in.read(buf.data(), buf.size()) || in.gcount() > 0)
        XXH3_128bits_update(state, buf.data(), static_cast<size_t>(in.gcount()));
    std::string out = to_hex(XXH3_128bits_digest(state));
    XXH3_freeState(state);
    return out;
}
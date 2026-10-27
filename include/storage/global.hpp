#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

// Global config lives at ~/.dense/config.json (readable by the owner only).
// It holds who you are — nothing about any particular server or directory.
namespace Global {

std::filesystem::path app_dir();  // ~/.dense/

struct Config {
    std::string email;        // set by `dense signup` / `dense login`
    std::string user_token;   // dns_usr_… session token; authorizes every command
    std::vector<std::string> ignore;  // global ignore patterns
};

// Returns nullopt if ~/.dense/config.json is missing or malformed.
std::optional<Config> load();

// Creates ~/.dense/ if needed, then writes config.json (mode 0600).
// Throws std::runtime_error if the file can't be written.
void save(const Config& cfg);

} // namespace Global

#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

// Per-source config lives in .dense/ within the tracked directory.
//   .dense/config.json   — source identity + connection state
//   .dense/.denseignore  — local ignore patterns (merged with global ignore)
namespace Local {

std::filesystem::path dense_dir(
    const std::filesystem::path& root = std::filesystem::current_path());

bool is_initialized(
    const std::filesystem::path& root = std::filesystem::current_path());

struct Config {
    std::string source_id;                // empty until connected
    std::vector<std::string> servers;     // names of the servers this source feeds
                                          // (informational; the server is the source of truth)

    bool connected() const { return !source_id.empty(); }
};

// Returns nullopt if not an initialized dense directory.
std::optional<Config> load(
    const std::filesystem::path& root = std::filesystem::current_path());

void save(const Config& cfg,
          const std::filesystem::path& root = std::filesystem::current_path());

// Patterns from .dense/.denseignore (empty if file missing).
std::vector<std::string> load_ignore(
    const std::filesystem::path& root = std::filesystem::current_path());

// Create .dense/ with a blank config and a default .denseignore.
// Errors if already initialized.
void init(const std::filesystem::path& root = std::filesystem::current_path());

} // namespace Local

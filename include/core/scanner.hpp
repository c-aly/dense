#pragma once
#include <filesystem>
#include <string>
#include <vector>

struct FileInfo {
    std::filesystem::path abs_path;
    std::filesystem::path rel_path;  // relative to source root, forward-slash separated
    std::string           format;    // lowercase extension without dot, e.g. "md", "py"
    uintmax_t             size = 0;
};

struct ScanStats {
    int total   = 0;   // files accepted
    int skipped = 0;   // ignored by patterns or unsupported format
    int failed  = 0;   // filesystem errors (permission denied, etc.)

    std::string to_string() const;
};

struct ScanResult {
    std::vector<FileInfo> files;
    ScanStats             stats;
};

// Walk root recursively.
// ignore_patterns: directory names, file names, or "*.ext" extension globs.
// .dense/ is always skipped regardless of patterns.
// Returns a ScanResult; never throws — errors increment stats.failed.
ScanResult scan_directory(
    const std::filesystem::path&    root,
    const std::vector<std::string>& ignore_patterns = {}
);

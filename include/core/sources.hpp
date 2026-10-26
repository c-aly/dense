// include/core/sources.hpp
#pragma once
#include "core/scanner.hpp"       // ScanStats
#include "storage/database.hpp"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>


// Stats returned from remove_source
struct RemoveStats {
    int documents = 0;
    int chunks    = 0;
    std::string to_string() const;
};

// One row from list_sources — identity + lightweight stats
struct SourceInfo {
    int64_t     id;
    std::string path;
    std::string kind;       // "dir" | "file"
    int64_t     added_at;   // unix epoch
    int         doc_count;
    std::string to_string() const;
};

// add (or re-scan) a source path; supersedes any narrower tracked sources
ScanStats add_source(Database& db, const std::filesystem::path& path);

// remove a source and all its documents/chunks from the DB
RemoveStats remove_source(Database& db, const std::filesystem::path& path);

// list all tracked sources with per-source doc counts
std::vector<SourceInfo> list_sources(Database& db);

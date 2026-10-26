// include/core/scanner.hpp
#pragma once
#include "storage/database.hpp"
#include <cstdint>
#include <string>


// Stats returned from scan
struct ScanStats {
    int added = 0, changed = 0, unchanged = 0, removed = 0, skipped = 0;
    std::string to_string() const;
};

// scan a source into DB
ScanStats scan_source(Database& db, int64_t source_id);

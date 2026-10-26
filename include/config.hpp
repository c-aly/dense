#pragma once

#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>


namespace Config {
    // get data directory
    inline std::filesystem::path data_dir() {
        if (const char* p = std::getenv("DENSE_HOME")) return p;

        #ifdef _WIN32
            const char* home = std::getenv("USERPROFILE");
        #else 
            const char* home = std::getenv("HOME");
        #endif

        return std::filesystem::path(home ? home : ".");
    }

    // get path to sqlite db
    inline std::filesystem::path db_path() {
        return data_dir() / "dense.db";
    }

    // path to user-editable config file
    inline std::filesystem::path config_path() {
        return data_dir() / "dense.cfg";
    }

    // chunker settings
    inline constexpr int chunk_min_tokens    = 100;
    inline constexpr int chunk_target_tokens = 400;
    inline constexpr int chunk_max_tokens    = 800;
    inline constexpr std::size_t max_file_bytes = 10 * 1024 * 1024;

    // read ignore_dirs from dense.cfg; creates the file with defaults if absent.
    // called on-demand so edits take effect without restart.
    std::vector<std::string> load_ignore_dirs();
}

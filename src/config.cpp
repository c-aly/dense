#include "config.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>


// TODO: add ignore_files too?

namespace {
    // Written to dense.cfg on first run; users can edit freely.
    const char* DEFAULT_CONFIG = R"CFG(# dense configuration
# ─────────────────────────────────────────────────────────────────────────────
# ignore_dirs: directory names to skip entirely during scanning.
# One name per line. Lines starting with '#' are comments. Blank lines ignored.
# Applies to every source. Per-folder .denseignore support is planned.

[ignore_dirs]
.git
.hg
.svn
build
dist
out
target
node_modules
venv
.venv
.env
__pycache__
vcpkg
vcpkg_installed
.tox
.mypy_cache
.cache
.next
.nuxt
.svelte-kit
coverage
)CFG";
}


namespace Config {

// Read [ignore_dirs] entries from dense.cfg.
// Creates the file with defaults if it doesn't exist.
std::vector<std::string> load_ignore_dirs() {
    namespace fs = std::filesystem;

    const fs::path cfg = config_path();

    // create with defaults if absent
    if (!fs::exists(cfg)) {
        fs::create_directories(cfg.parent_path());
        std::ofstream out(cfg);
        if (out) out << DEFAULT_CONFIG;
        // if write fails, we just continue with the in-memory defaults below
    }

    std::vector<std::string> dirs;
    std::ifstream in(cfg);
    if (!in) {
        // can't read file — fall back to safe hardcoded minimum
        return {".git", "build", "node_modules", "venv", ".venv",
                "vcpkg", "vcpkg_installed", "__pycache__"};
    }

    // read everything from ignore section
    bool in_ignore_section = false;
    std::string line;
    while (std::getline(in, line)) {
        // strip trailing whitespace / CR
        while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r'))
            line.pop_back();

        if (line.empty() || line[0] == '#') continue;

        if (line[0] == '[') {
            in_ignore_section = (line == "[ignore_dirs]");
            continue;
        }

        if (in_ignore_section) dirs.push_back(line);
    }

    return dirs;
}

} // namespace Config

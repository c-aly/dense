#include "storage/local.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
using json = nlohmann::json;

namespace Local {

// ── paths ─────────────────────────────────────────────────────────────────────

std::filesystem::path dense_dir(const std::filesystem::path& root) {
    return root / ".dense";
}

bool is_initialized(const std::filesystem::path& root) {
    return std::filesystem::exists(dense_dir(root) / "config.json");
}

// ── config ────────────────────────────────────────────────────────────────────

std::optional<Config> load(const std::filesystem::path& root) {
    std::ifstream f(dense_dir(root) / "config.json");
    if (!f) return std::nullopt;

    try {
        json data = json::parse(f);
        Config cfg;
        cfg.source_id = data.value("source_id", std::string{});
        cfg.servers   = data.value("servers",   std::vector<std::string>{});
        // older configs stored a single server name
        if (cfg.servers.empty()) {
            auto legacy = data.value("server", std::string{});
            if (!legacy.empty()) cfg.servers.push_back(legacy);
        }
        return cfg;
    } catch (...) {
        return std::nullopt;
    }
}

void save(const Config& cfg, const std::filesystem::path& root) {
    json data = {
        {"source_id", cfg.source_id},
        {"servers",   cfg.servers},
    };
    std::ofstream f(dense_dir(root) / "config.json");
    f << data.dump(2) << '\n';
}

// ── .denseignore ──────────────────────────────────────────────────────────────

std::vector<std::string> load_ignore(const std::filesystem::path& root) {
    std::ifstream f(dense_dir(root) / ".denseignore");
    if (!f) return {};

    std::vector<std::string> patterns;
    std::string line;
    while (std::getline(f, line)) {
        // strip trailing whitespace/CR
        while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r'))
            line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        patterns.push_back(line);
    }
    return patterns;
}

// ── init ──────────────────────────────────────────────────────────────────────

// Default .denseignore — ported from the old dense.cfg defaults.
// Includes common build output, VCS, dependency, and env directories.
static constexpr std::string_view DEFAULT_DENSEIGNORE = R"(# dense ignore patterns
# Directory names or file extensions (*.ext) to skip during push.
# Lines starting with '#' are comments.

# version control
.git
.hg
.svn

# build output
build
dist
out
target
_build

# dependencies
node_modules
vendor
vcpkg
vcpkg_installed

# python
venv
.venv
__pycache__
.tox
.mypy_cache
*.pyc

# env / secrets
.env
.env.*

# editor / OS
.DS_Store
.idea
.vscode
*.swp

# caches
.cache
.next
.nuxt
.svelte-kit
coverage
)";

void init(const std::filesystem::path& root) {
    if (is_initialized(root))
        throw std::runtime_error(".dense/ already exists in " + root.string());

    auto dir = dense_dir(root);
    std::filesystem::create_directories(dir);

    // blank config — not connected until `dense connect`
    save(Config{}, root);

    // default .denseignore
    std::ofstream ig(dir / ".denseignore");
    ig << DEFAULT_DENSEIGNORE;
}

} // namespace Local

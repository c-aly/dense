#include "storage/global.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
using json = nlohmann::json;

namespace Global {

std::filesystem::path app_dir() {
    const char* home = std::getenv("HOME");
    return home
        ? std::filesystem::path(home) / ".dense"
        : std::filesystem::current_path() / ".dense";
}

std::optional<Config> load() {
    std::ifstream f(app_dir() / "config.json");
    if (!f) return std::nullopt;
    try {
        json d = json::parse(f);
        Config cfg;
        cfg.email      = d.value("email",      std::string{});
        cfg.user_token = d.value("user_token", std::string{});
        cfg.ignore     = d.value("ignore",     std::vector<std::string>{});
        return cfg;
    } catch (...) {
        return std::nullopt;
    }
}

void save(const Config& cfg) {
    namespace fs = std::filesystem;
    auto dir  = app_dir();
    auto path = dir / "config.json";
    fs::create_directories(dir);

    json d;
    d["email"]      = cfg.email;
    d["user_token"] = cfg.user_token;
    d["ignore"]     = cfg.ignore;

    // create the file owner-only *before* writing secrets into it
    if (!fs::exists(path)) { std::ofstream touch(path); }
    std::error_code ec;
    fs::permissions(path, fs::perms::owner_read | fs::perms::owner_write,
                    fs::perm_options::replace, ec);   // best effort (no-op semantics on Windows)

    std::ofstream f(path, std::ios::trunc);
    if (!f) throw std::runtime_error("cannot write " + path.string());
    f << d.dump(2) << '\n';
    if (!f) throw std::runtime_error("failed writing " + path.string());
}

} // namespace Global

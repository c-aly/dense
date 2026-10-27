#include "cli/commands/common.hpp"
#include "cli/prompt.hpp"
#include "storage/local.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <vector>

namespace dense::cli::detail {

std::optional<Global::Config> require_login() {
    auto cfg = Global::load();
    if (!cfg || cfg->user_token.empty()) {
        std::cerr << "error: not logged in.\n"
                  << "       Run `dense signup` or `dense login` first.\n";
        return std::nullopt;
    }
    return cfg;
}

void report_error(const std::string& cmd, const std::exception& e) {
    // the server's 401 message already says to run `dense login`
    std::cerr << "dense " << cmd << ": " << e.what() << "\n";
}

std::string ask_email(const std::string& from_flag) {
    std::string email = from_flag.empty() ? prompt("Email: ") : from_flag;
    const auto b = email.find_first_not_of(" \t\r\n");
    const auto e = email.find_last_not_of(" \t\r\n");
    return b == std::string::npos ? std::string{} : email.substr(b, e - b + 1);
}

void save_session(const Http::AuthResult& auth) {
    auto cfg = Global::load().value_or(Global::Config{});
    cfg.email      = auth.email;
    cfg.user_token = auth.token;
    Global::save(cfg);
}

void forget_local_server(const std::string& server_name) {
    auto cfg = Local::load();
    if (!cfg) return;
    auto& v = cfg->servers;
    const auto it = std::remove(v.begin(), v.end(), server_name);
    if (it == v.end()) return;
    v.erase(it, v.end());
    Local::save(*cfg);
}

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::optional<Http::ServerInfo> resolve_server(Http::DenseClient& client,
                                               const std::string& ref,
                                               const std::string& cmd)
{
    const auto all = client.list_servers();

    for (const auto& s : all)
        if (s.server_id == ref) return s;

    std::vector<Http::ServerInfo> matches;
    for (const auto& s : all)
        if (s.name == ref) matches.push_back(s);

    if (matches.empty()) {
        std::cerr << "dense " << cmd << ": no server '" << ref
                  << "' that you own or belong to (see `dense server list`).\n";
        return std::nullopt;
    }
    if (matches.size() > 1) {
        std::cerr << "dense " << cmd << ": more than one server is named '" << ref
                  << "'. Use its id instead:\n";
        for (const auto& s : matches)
            std::cerr << "  " << s.server_id << "  (owner: " << s.owner_email << ")\n";
        return std::nullopt;
    }
    return matches.front();
}

} // namespace dense::cli::detail

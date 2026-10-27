// Per-directory commands: dense init / connect / list
#include "cli/commands.hpp"
#include "cli/commands/common.hpp"
#include "http/client.hpp"
#include "storage/global.hpp"
#include "storage/local.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#endif

namespace {

std::string get_hostname() {
    char buf[256] = {};
    return (gethostname(buf, sizeof buf) == 0) ? buf : "unknown";
}

} // namespace

namespace dense::cli {

using detail::report_error;
using detail::require_login;
using detail::resolve_server;

void handle_init(const std::filesystem::path& path) {
    try {
        Local::init(path);
        std::cout << "Initialized dense source at " << path << "\n"
                  << "Run `dense connect <server>` to link it to a server.\n";
    } catch (const std::exception& e) {
        std::cerr << "dense init: " << e.what() << "\n";
    }
}

void handle_connect(const std::string& server_ref) {
    const auto cwd = std::filesystem::current_path();

    if (!Local::is_initialized(cwd)) {
        std::cerr << "dense connect: not a dense directory — run `dense init` first.\n";
        return;
    }

    auto global = require_login();
    if (!global) return;

    try {
        Http::DenseClient client(global->user_token);

        auto server = resolve_server(client, server_ref, "connect");
        if (!server) return;

        const std::string source_name = cwd.filename().string();
        auto result = client.connect(server->server_id, source_name, get_hostname());

        // a source can feed several servers, so remember all of them
        auto cfg = Local::load(cwd).value_or(Local::Config{});
        cfg.source_id = result.source_id;
        if (std::find(cfg.servers.begin(), cfg.servers.end(), server->name) == cfg.servers.end())
            cfg.servers.push_back(server->name);
        Local::save(cfg, cwd);

        std::cout << "Connected '" << source_name << "' → server '" << server->name << "'";
        if (server->role != "owner") std::cout << " (owned by " << server->owner_email << ")";
        std::cout << ".\nRun `dense push` to upload files.\n";

    } catch (const std::exception& e) {
        report_error("connect", e);
    }
}

void handle_list() {
    auto local = Local::load();
    if (!local) { std::cout << "Not a dense directory.\n"; return; }

    // The server is the source of truth: forget servers that were deleted (or that you
    // left) from another directory or machine. If we can't reach it, show what we have.
    auto global = Global::load();
    bool checked = true;
    if (global && !global->user_token.empty()) {
        try {
            Http::DenseClient client(global->user_token);
            std::vector<std::string> live;
            for (const auto& s : client.list_servers()) live.push_back(s.name);
            auto& v = local->servers;
            const auto before = v.size();
            v.erase(std::remove_if(v.begin(), v.end(), [&](const std::string& name) {
                        return std::find(live.begin(), live.end(), name) == live.end();
                    }), v.end());
            if (v.size() != before) Local::save(*local);
        } catch (const std::exception&) {
            checked = false;
        }
    }

    std::cout << "source_id: " << (local->source_id.empty() ? "(not connected)" : local->source_id) << "\n"
              << "servers:   ";
    if (local->servers.empty()) {
        std::cout << "(none)";
    } else {
        for (size_t i = 0; i < local->servers.size(); ++i)
            std::cout << (i ? ", " : "") << local->servers[i];
    }
    std::cout << "\n";

    if (!checked) std::cout << "(couldn't reach the server; this list may be out of date)\n";
    if (global && !global->email.empty())
        std::cout << "user:      " << global->email << "\n";
}

} // namespace dense::cli

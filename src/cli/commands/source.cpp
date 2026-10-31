// Per-directory commands: dense init / connect / disconnect / status
#include "cli/commands.hpp"
#include "cli/commands/common.hpp"
#include "http/client.hpp"
#include "storage/global.hpp"
#include "storage/local.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

namespace {

struct Feed {
    std::string name;
    std::string note;   // empty, or "owned by …"
};

void print_feeds(const std::vector<Feed>& feeds) {
    std::cout << "  feeds   ";
    if (feeds.empty()) {
        std::cout << "(none)\n";
        return;
    }
    for (size_t i = 0; i < feeds.size(); ++i) {
        if (i) std::cout << "          ";
        std::cout << feeds[i].name;
        if (!feeds[i].note.empty()) std::cout << "  (" << feeds[i].note << ")";
        std::cout << "\n";
    }
}

// One server you're a member of: no note when you own it. Several servers share
// the name: list every owner, since the folder can't tell them apart by name.
std::string owners_note(const std::vector<Http::ServerInfo>& linked) {
    if (linked.size() == 1) {
        if (linked[0].role == "owner") return {};
        return "owned by " + linked[0].owner_email;
    }
    std::string note = "owned by ";
    for (size_t i = 0; i < linked.size(); ++i) {
        if (i) note += ", ";
        note += linked[i].owner_email;
    }
    return note;
}

bool remember_server(Local::Config& cfg, const std::string& name, const std::filesystem::path& cwd) {
    if (std::find(cfg.servers.begin(), cfg.servers.end(), name) != cfg.servers.end())
        return false;
    cfg.servers.push_back(name);
    Local::save(cfg, cwd);
    return true;
}

} // namespace

namespace dense::cli {

using detail::forget_local_server;
using detail::report_error;
using detail::require_login;
using detail::resolve_server;

int handle_init(const std::filesystem::path& path) {
    try {
        Local::init(path);
        std::cout << "Initialized dense source at " << path << "\n"
                  << "Next: `dense connect <server>`\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "dense init: " << e.what() << "\n";
        return 1;
    }
}

int handle_connect(const std::string& server_ref) {
    const auto cwd = std::filesystem::current_path();

    if (!Local::is_initialized(cwd)) {
        std::cerr << "dense connect: not a dense directory — run `dense init` first.\n";
        return 1;
    }

    auto global = require_login();
    if (!global) return 1;

    try {
        Http::DenseClient client(global->user_token);

        auto server = resolve_server(client, server_ref, "connect");
        if (!server) return 1;

        auto cfg = Local::load(cwd).value_or(Local::Config{});
        const std::string source_name = cwd.filename().string();

        // The id in .dense is this folder. A missing or foreign id means create a new
        // source (a copied folder must not publish into someone else's).
        Http::ConnectResult result;
        if (cfg.source_id.empty()) {
            result = client.connect(server->server_id, source_name);
        } else {
            try {
                result = client.connect(server->server_id, source_name, cfg.source_id);
            } catch (const Http::ApiError& e) {
                if (e.status_code != 404 || std::string(e.what()).find("source not found") == std::string::npos)
                    throw;
                result = client.connect(server->server_id, source_name);
            }
        }

        cfg.source_id = result.source_id;
        remember_server(cfg, server->name, cwd);
        Local::save(cfg, cwd);

        if (result.already_linked) {
            std::cout << "Already publishing this folder to '" << server->name << "'.\n";
            return 0;
        }
        std::cout << "Connected '" << source_name << "' to '" << server->name << "'";
        if (server->role != "owner") std::cout << " (owned by " << server->owner_email << ")";
        std::cout << ".\nNext: `dense push`\n";
        return 0;

    } catch (const std::exception& e) {
        report_error("connect", e);
        return 1;
    }
}

int handle_disconnect(const std::string& server_ref) {
    const auto cwd = std::filesystem::current_path();

    if (!Local::is_initialized(cwd)) {
        std::cerr << "dense disconnect: not a dense directory — run `dense init` first.\n";
        return 1;
    }

    auto local = Local::load(cwd);
    if (!local || !local->connected()) {
        std::cerr << "dense disconnect: this folder isn't connected.\n";
        return 1;
    }

    auto global = require_login();
    if (!global) return 1;

    try {
        Http::DenseClient client(global->user_token);
        auto server = resolve_server(client, server_ref, "disconnect");
        if (!server) return 1;

        bool linked = false;
        for (const auto& src : client.list_sources(server->server_id))
            if (src.source_id == local->source_id) { linked = true; break; }

        if (!linked) {
            forget_local_server(server->name);
            std::cout << "This folder isn't publishing to '" << server->name << "'.\n";
            return 0;
        }

        client.unlink_source(server->server_id, local->source_id);
        forget_local_server(server->name);
        std::cout << "Stopped publishing this folder to '" << server->name
                  << "'. Uploaded files are kept.\n";
        return 0;
    } catch (const std::exception& e) {
        report_error("disconnect", e);
        return 1;
    }
}

int handle_status() {
    const auto cwd = std::filesystem::current_path();
    auto local = Local::load(cwd);
    auto global = Global::load();
    const bool logged_in = global && !global->user_token.empty();

    if (!logged_in) {
        std::cout << "Not logged in.\n"
                  << "Run `dense signup` or `dense login`.\n";
    } else {
        std::cout << "Logged in as " << global->email << "\n";
    }

    if (!local) {
        std::cout << "\nThis folder is not a dense source.\n";
        if (logged_in) std::cout << "Next: `dense init`\n";
        return 0;
    }

    const std::string folder = cwd.filename().string();
    if (!local->connected()) {
        std::cout << "\n" << folder << "/\n"
                  << "  not connected\n";
        if (logged_in) std::cout << "Next: `dense connect <server>`\n";
        return 0;
    }

    std::vector<Feed> feeds;
    bool tried = false;
    bool reached = false;
    bool expired = false;

    if (logged_in) {
        tried = true;
        try {
            Http::DenseClient client(global->user_token);
            auto servers = client.list_servers();

            // Drop servers that were deleted, left, or unlinked elsewhere.
            std::vector<std::string> still;
            for (const auto& name : local->servers) {
                std::vector<Http::ServerInfo> matches;
                for (const auto& s : servers)
                    if (s.name == name) matches.push_back(s);

                std::vector<Http::ServerInfo> linked;
                for (const auto& s : matches) {
                    for (const auto& src : client.list_sources(s.server_id)) {
                        if (src.source_id != local->source_id) continue;
                        linked.push_back(s);
                        break;
                    }
                }
                if (linked.empty()) continue;
                still.push_back(name);
                feeds.push_back({name, owners_note(linked)});
            }
            if (still != local->servers) {
                local->servers = std::move(still);
                Local::save(*local, cwd);
            }
            reached = true;
        } catch (const Http::ApiError& e) {
            expired = e.status_code == 401;
        } catch (const std::exception&) {
        }
    }

    if (!reached) {
        feeds.clear();
        for (const auto& name : local->servers) feeds.push_back({name, {}});
    }

    std::cout << "\n" << folder << "/\n"
              << "  source  " << local->source_id << "\n";
    print_feeds(feeds);

    if (expired)
        std::cout << "Session expired. Run `dense login`.\n";
    else if (tried && !reached)
        std::cout << "(couldn't reach the server; this may be out of date)\n";
    else if (logged_in && feeds.empty())
        std::cout << "Next: `dense connect <server>`\n";
    return 0;
}

} // namespace dense::cli

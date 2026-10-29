// dense server key create / list / revoke — agent credentials for a server's /mcp endpoint
#include "cli/commands.hpp"
#include "cli/commands/common.hpp"
#include "http/client.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>

namespace dense::cli {

using detail::report_error;
using detail::require_login;
using detail::resolve_server;

namespace {

// The key is a credential for an MCP *client* (Claude Code, Cursor, ...). We can't
// write their config for them yet, so show exactly what to paste where.
void print_setup(const std::string& server_name, const std::string& key) {
    const std::string url  = Http::mcp_url();   // with a key, the key (not the URL) names the server
    const std::string name = "dense-" + server_name;

    std::cout << "Add it to your agent:\n\n"
              << "  Claude Code\n"
              << "    claude mcp add --transport http " << name << " " << url
              << " --header \"Authorization: Bearer " << key << "\"\n\n"
              << "  Cursor (~/.cursor/mcp.json) and other clients that use \"mcpServers\"\n"
              << "    {\n"
              << "      \"mcpServers\": {\n"
              << "        \"" << name << "\": {\n"
              << "          \"url\": \"" << url << "\",\n"
              << "          \"headers\": { \"Authorization\": \"Bearer " << key << "\" }\n"
              << "        }\n"
              << "      }\n"
              << "    }\n\n";
}

} // namespace

void handle_server_key_create(const std::string& ref, const std::string& label) {
    auto global = require_login();
    if (!global) return;

    try {
        Http::DenseClient client(global->user_token);
        auto server = resolve_server(client, ref, "server key create");
        if (!server) return;

        auto k = client.create_server_key(server->server_id, label);
        std::cout << "Created a key for '" << server->name << "'. Copy it now; it can't be shown again.\n\n"
                  << "  " << k.key << "\n\n";
        print_setup(server->name, k.key);
        std::cout << "Anyone holding this key can read everything in '" << server->name << "'.\n"
                  << "Revoke it with `dense server key revoke " << server->name << " " << k.key_id << "`.\n";
    } catch (const std::exception& e) {
        report_error("server key create", e);
    }
}

void handle_server_key_list(const std::string& ref) {
    auto global = require_login();
    if (!global) return;

    try {
        Http::DenseClient client(global->user_token);
        auto server = resolve_server(client, ref, "server key list");
        if (!server) return;

        auto keys = client.list_server_keys(server->server_id);
        if (keys.empty()) {
            std::cout << "No keys for '" << server->name << "'. Create one with `dense server key create "
                      << server->name << "`.\n";
            return;
        }

        size_t label_w = 5, by_w = 10;
        for (const auto& k : keys) {
            label_w = std::max(label_w, k.label.size());
            by_w    = std::max(by_w, k.created_by_email.size());
        }
        std::cout << (server->role == "owner" ? "All keys" : "Your keys") << " for '" << server->name << "':\n"
                  << std::left
                  << "  " << std::setw(static_cast<int>(label_w + 2)) << "LABEL"
                  << std::setw(17)                                    << "KEY"
                  << std::setw(static_cast<int>(by_w + 2))            << "CREATED BY"
                  << std::setw(12)                                    << "CREATED"
                  << "ID\n";
        for (const auto& k : keys)
            std::cout << "  " << std::setw(static_cast<int>(label_w + 2)) << (k.label.empty() ? "-" : k.label)
                      << std::setw(17)                                    << (k.hint + "...")
                      << std::setw(static_cast<int>(by_w + 2))            << k.created_by_email
                      << std::setw(12)                                    << k.created_at.substr(0, 10)
                      << k.key_id << "\n";
    } catch (const std::exception& e) {
        report_error("server key list", e);
    }
}

void handle_server_key_revoke(const std::string& ref, const std::string& key_ref) {
    auto global = require_login();
    if (!global) return;

    try {
        Http::DenseClient client(global->user_token);
        auto server = resolve_server(client, ref, "server key revoke");
        if (!server) return;

        // accept the full id, or any prefix of it that's unambiguous
        std::vector<Http::ServerKeyInfo> matches;
        for (const auto& k : client.list_server_keys(server->server_id))
            if (k.key_id.rfind(key_ref, 0) == 0) matches.push_back(k);

        if (matches.empty()) {
            std::cerr << "dense server key revoke: no key '" << key_ref << "' on '" << server->name
                      << "' (see `dense server key list " << server->name << "`).\n";
            return;
        }
        if (matches.size() > 1) {
            std::cerr << "dense server key revoke: '" << key_ref << "' matches " << matches.size()
                      << " keys; use more of the id.\n";
            return;
        }
        client.revoke_server_key(server->server_id, matches[0].key_id);
        std::cout << "Revoked key " << matches[0].hint << "... on '" << server->name
                  << "'. Agents using it lose access immediately.\n";
    } catch (const std::exception& e) {
        report_error("server key revoke", e);
    }
}

} // namespace dense::cli

// dense server create / list / remove / share / unshare / members
#include "cli/commands.hpp"
#include "cli/commands/common.hpp"
#include "cli/prompt.hpp"
#include "http/client.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>

namespace dense::cli {

using detail::forget_local_server;
using detail::lower;
using detail::report_error;
using detail::require_login;
using detail::resolve_server;

void handle_server_create(const std::string& name) {
    auto global = require_login();
    if (!global) return;

    try {
        Http::DenseClient client(global->user_token);
        auto s = client.create_server(name);
        std::cout << "Created server '" << s.name << "' (id: " << s.server_id << ").\n"
                  << "Link a directory to it with `dense connect " << s.name << "`.\n"
                  << "Give an agent access with `dense server key create " << s.name << "`,\n"
                  << "or connect Claude / ChatGPT with `dense server url " << s.name << "`.\n";
    } catch (const std::exception& e) {
        report_error("server create", e);
    }
}

void handle_server_list() {
    auto global = require_login();
    if (!global) return;

    try {
        Http::DenseClient client(global->user_token);
        auto servers = client.list_servers();
        if (servers.empty()) {
            std::cout << "No servers yet. Create one with `dense server create <name>`.\n";
            return;
        }

        size_t name_w = 4, owner_w = 5;
        for (const auto& s : servers) {
            name_w  = std::max(name_w,  s.name.size());
            owner_w = std::max(owner_w, s.owner_email.size());
        }
        std::cout << std::left
                  << std::setw(static_cast<int>(name_w + 2))  << "NAME"
                  << std::setw(9)                             << "ROLE"
                  << std::setw(static_cast<int>(owner_w + 2)) << "OWNER"
                  << "ID\n";
        for (const auto& s : servers)
            std::cout << std::setw(static_cast<int>(name_w + 2))  << s.name
                      << std::setw(9)                             << s.role
                      << std::setw(static_cast<int>(owner_w + 2)) << s.owner_email
                      << s.server_id << "\n";
    } catch (const std::exception& e) {
        report_error("server list", e);
    }
}

void handle_server_url(const std::string& ref) {
    auto global = require_login();
    if (!global) return;

    try {
        Http::DenseClient client(global->user_token);
        auto server = resolve_server(client, ref, "server url");
        if (!server) return;

        std::cout << Http::mcp_url(server->server_id) << "\n\n"
                  << "Add this as a custom connector in Claude or ChatGPT (Settings > Connectors).\n"
                  << "You'll sign in with Google, using the email on your dense account, and the\n"
                  << "agent can then read '" << server->name << "' and nothing else.\n"
                  << "For Cursor, Claude Code and other tools that take a key: `dense server key create "
                  << server->name << "`.\n";
    } catch (const std::exception& e) {
        report_error("server url", e);
    }
}

void handle_server_remove(const std::string& ref, bool yes) {
    auto global = require_login();
    if (!global) return;

    try {
        Http::DenseClient client(global->user_token);
        auto server = resolve_server(client, ref, "server remove");
        if (!server) return;

        if (server->role != "owner") {
            std::cerr << "dense server remove: only the owner (" << server->owner_email
                      << ") can delete '" << server->name << "'.\n"
                      << "  (to stop using it yourself: `dense server unshare "
                      << server->name << " " << global->email << "`)\n";
            return;
        }

        if (!yes) {
            std::cout << "This deletes server '" << server->name << "' for everyone it's shared with.\n"
                      << "Sources and their files are kept.\n";
            if (prompt("Type the server name to confirm: ") != server->name) {
                std::cout << "Cancelled.\n";
                return;
            }
        }
        client.delete_server(server->server_id);
        forget_local_server(server->name);
        std::cout << "Deleted server '" << server->name << "'.\n";
    } catch (const std::exception& e) {
        report_error("server remove", e);
    }
}

void handle_server_share(const std::string& ref, const std::string& email) {
    auto global = require_login();
    if (!global) return;

    try {
        Http::DenseClient client(global->user_token);
        auto server = resolve_server(client, ref, "server share");
        if (!server) return;

        auto m = client.share_server(server->server_id, email);
        if (m.role == "owner")
            std::cout << m.email << " already owns '" << server->name << "'.\n";
        else
            std::cout << "Shared '" << server->name << "' with " << m.email << ".\n";
    } catch (const std::exception& e) {
        report_error("server share", e);
    }
}

void handle_server_unshare(const std::string& ref, const std::string& email) {
    auto global = require_login();
    if (!global) return;

    try {
        Http::DenseClient client(global->user_token);
        auto server = resolve_server(client, ref, "server unshare");
        if (!server) return;

        const std::string wanted = lower(email);
        std::string user_id;
        for (const auto& m : client.list_members(server->server_id))
            if (lower(m.email) == wanted) user_id = m.user_id;

        if (user_id.empty()) {
            std::cerr << "dense server unshare: " << email << " isn't a member of '"
                      << server->name << "'.\n";
            return;
        }
        client.remove_member(server->server_id, user_id);

        if (wanted == lower(global->email)) {
            forget_local_server(server->name);
            std::cout << "You left '" << server->name << "'.\n";
        }
        else
            std::cout << "Removed " << email << " from '" << server->name << "'.\n";
    } catch (const std::exception& e) {
        report_error("server unshare", e);
    }
}

void handle_server_members(const std::string& ref) {
    auto global = require_login();
    if (!global) return;

    try {
        Http::DenseClient client(global->user_token);
        auto server = resolve_server(client, ref, "server members");
        if (!server) return;

        std::cout << "Members of '" << server->name << "':\n";
        for (const auto& m : client.list_members(server->server_id))
            std::cout << "  " << m.email << "  (" << m.role << ")\n";
    } catch (const std::exception& e) {
        report_error("server members", e);
    }
}

} // namespace dense::cli

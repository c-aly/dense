// dense server create / list / delete / visibility / sources / url / share / unshare / members
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

int handle_server_create(const std::string& name) {
    auto global = require_login();
    if (!global) return 1;

    try {
        Http::DenseClient client(global->user_token);
        auto s = client.create_server(name);
        std::cout << "Created server '" << s.name << "' (id: " << s.server_id << ").\n"
                  << "Next: from the folder you want to publish, run `dense init` then `dense connect "
                  << s.name << "`.\n";
        return 0;
    } catch (const std::exception& e) {
        report_error("server create", e);
        return 1;
    }
}

int handle_server_list() {
    auto global = require_login();
    if (!global) return 1;

    try {
        Http::DenseClient client(global->user_token);
        auto servers = client.list_servers();
        if (servers.empty()) {
            std::cout << "No servers yet. Create one with `dense server create <name>`.\n";
            return 0;
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
        return 0;
    } catch (const std::exception& e) {
        report_error("server list", e);
        return 1;
    }
}

int handle_server_sources(const std::string& ref) {
    auto global = require_login();
    if (!global) return 1;

    try {
        Http::DenseClient client(global->user_token);
        auto server = resolve_server(client, ref, "server sources");
        if (!server) return 1;

        auto sources = client.list_sources(server->server_id);
        if (sources.empty()) {
            std::cout << "No folders feeding '" << server->name << "'.\n";
            return 0;
        }

        size_t name_w = 4, owner_w = 5;
        for (const auto& s : sources) {
            name_w  = std::max(name_w, s.name.size());
            owner_w = std::max(owner_w, s.owner_email.size());
        }
        std::cout << "Folders feeding '" << server->name << "':\n"
                  << std::left
                  << "  " << std::setw(static_cast<int>(name_w + 2))  << "NAME"
                  << std::setw(static_cast<int>(owner_w + 2))         << "OWNER"
                  << "ID\n";
        for (const auto& s : sources)
            std::cout << "  " << std::setw(static_cast<int>(name_w + 2)) << s.name
                      << std::setw(static_cast<int>(owner_w + 2))        << s.owner_email
                      << s.source_id << "\n";
        return 0;
    } catch (const std::exception& e) {
        report_error("server sources", e);
        return 1;
    }
}

int handle_server_url(const std::string& ref) {
    auto global = require_login();
    if (!global) return 1;

    try {
        Http::DenseClient client(global->user_token);
        auto server = resolve_server(client, ref, "server url");
        if (!server) return 1;

        std::cout << Http::mcp_url(server->server_id) << "\n\n"
                  << "Add this as a custom connector in Claude (Settings > Connectors).\n"
                  << "You'll sign in with Google, using the email on your dense account, and the\n"
                  << "agent can then read '" << server->name << "' and nothing else.\n"
                  << "ChatGPT not supported.\n"
                  << "For Cursor, Claude Code and other tools that take a key: `dense server key create "
                  << server->name << "`.\n";
        return 0;
    } catch (const std::exception& e) {
        report_error("server url", e);
        return 1;
    }
}

int handle_server_visibility(const std::string& ref, const std::string& visibility) {
    if (visibility != "public" && visibility != "private") {
        std::cerr << "dense server visibility: expected public or private.\n";
        return 1;
    }

    auto global = require_login();
    if (!global) return 1;

    try {
        Http::DenseClient client(global->user_token);
        auto server = resolve_server(client, ref, "server visibility");
        if (!server) return 1;

        client.set_server_visibility(server->server_id, visibility);
        std::cout << "Server '" << server->name << "' is " << visibility << ".\n";
        if (visibility == "public")
            std::cout << "Anyone with the URL from `dense server url " << server->name << "` can read it.\n";
        return 0;
    } catch (const std::exception& e) {
        report_error("server visibility", e);
        return 1;
    }
}

int handle_server_delete(const std::string& ref, bool yes) {
    auto global = require_login();
    if (!global) return 1;

    try {
        Http::DenseClient client(global->user_token);
        auto server = resolve_server(client, ref, "server delete");
        if (!server) return 1;

        if (server->role != "owner") {
            std::cerr << "dense server delete: only the owner (" << server->owner_email
                      << ") can delete '" << server->name << "'.\n"
                      << "  (to stop using it yourself: `dense server unshare "
                      << server->name << " " << global->email << "`)\n";
            return 1;
        }

        if (!yes) {
            std::cout << "This deletes server '" << server->name << "' for everyone it's shared with.\n"
                      << "Sources and their files are kept.\n";
            if (prompt("Type the server name to confirm: ") != server->name) {
                std::cout << "Cancelled.\n";
                return 0;
            }
        }
        client.delete_server(server->server_id);
        forget_local_server(server->name);
        std::cout << "Deleted server '" << server->name << "'.\n";
        return 0;
    } catch (const std::exception& e) {
        report_error("server delete", e);
        return 1;
    }
}

int handle_server_share(const std::string& ref, const std::string& email) {
    auto global = require_login();
    if (!global) return 1;

    try {
        Http::DenseClient client(global->user_token);
        auto server = resolve_server(client, ref, "server share");
        if (!server) return 1;

        auto m = client.share_server(server->server_id, email);
        if (m.role == "owner")
            std::cout << m.email << " already owns '" << server->name << "'.\n";
        else
            std::cout << "Shared '" << server->name << "' with " << m.email << ".\n";
        return 0;
    } catch (const std::exception& e) {
        report_error("server share", e);
        return 1;
    }
}

int handle_server_unshare(const std::string& ref, const std::string& email) {
    auto global = require_login();
    if (!global) return 1;

    try {
        Http::DenseClient client(global->user_token);
        auto server = resolve_server(client, ref, "server unshare");
        if (!server) return 1;

        const std::string wanted = lower(email);
        std::string user_id, role;
        for (const auto& m : client.list_members(server->server_id))
            if (lower(m.email) == wanted) { user_id = m.user_id; role = m.role; }

        if (user_id.empty()) {
            std::cerr << "dense server unshare: " << email << " isn't a member of '"
                      << server->name << "'.\n";
            return 1;
        }
        if (role == "owner") {
            std::cerr << "dense server unshare: can't remove the owner of '" << server->name << "'.";
            if (server->role == "owner")
                std::cerr << " Delete it with `dense server delete " << server->name << "`.";
            std::cerr << "\n";
            return 1;
        }
        client.remove_member(server->server_id, user_id);

        if (wanted == lower(global->email)) {
            forget_local_server(server->name);
            std::cout << "You left '" << server->name << "'.\n";
        }
        else
            std::cout << "Removed " << email << " from '" << server->name << "'.\n";
        return 0;
    } catch (const std::exception& e) {
        report_error("server unshare", e);
        return 1;
    }
}

int handle_server_members(const std::string& ref) {
    auto global = require_login();
    if (!global) return 1;

    try {
        Http::DenseClient client(global->user_token);
        auto server = resolve_server(client, ref, "server members");
        if (!server) return 1;

        std::cout << "Members of '" << server->name << "':\n";
        for (const auto& m : client.list_members(server->server_id))
            std::cout << "  " << m.email << "  (" << m.role << ")\n";
        return 0;
    } catch (const std::exception& e) {
        report_error("server members", e);
        return 1;
    }
}

} // namespace dense::cli

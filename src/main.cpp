#include <CLI/CLI.hpp>
#include <filesystem>
#include <iostream>
#include <string>

#include "cli/commands.hpp"

int main(int argc, char** argv) {
    CLI::App app("Publish a folder to an info server that agents can search");
    app.require_subcommand(1);

    // ── account ──────────────────────────────────────────────────────────────

    std::string signup_email;
    auto* signup_cmd = app.add_subcommand("signup", "Create a dense account and log in");
    signup_cmd->group("Account");
    signup_cmd->add_option("--email", signup_email, "Email (prompted if omitted)");

    std::string login_email;
    auto* login_cmd = app.add_subcommand("login", "Log in to your dense account");
    login_cmd->group("Account");
    login_cmd->add_option("--email", login_email, "Email (prompted if omitted)");

    // ── this directory ───────────────────────────────────────────────────────

    std::filesystem::path init_path = std::filesystem::current_path();
    auto* init_cmd = app.add_subcommand("init", "Set this folder up so it can be published");
    init_cmd->group("This folder");
    init_cmd->add_option("path", init_path, "Directory to initialize (default: cwd)")
            ->check(CLI::ExistingDirectory);

    std::string connect_server;
    auto* connect_cmd = app.add_subcommand("connect", "Publish this folder to a server");
    connect_cmd->group("This folder");
    connect_cmd->add_option("server", connect_server, "Server name or id")->required();

    std::string disconnect_server;
    auto* disconnect_cmd = app.add_subcommand("disconnect", "Stop publishing this folder to a server");
    disconnect_cmd->group("This folder");
    disconnect_cmd->add_option("server", disconnect_server, "Server name or id")->required();

    auto* push_cmd = app.add_subcommand("push", "Upload new and changed files");
    push_cmd->group("This folder");

    auto* status_cmd = app.add_subcommand("status", "Show your account and what this folder publishes");
    status_cmd->group("This folder");

    // Hidden. `list` used to mean status; point people at the new name instead of
    // the generic "a subcommand is required" that CLI11 prints for an unknown command.
    auto* list_cmd = app.add_subcommand("list", "Renamed to status");
    list_cmd->group("");

    // ── servers ──────────────────────────────────────────────────────────────

    auto* server_cmd = app.add_subcommand("server", "Create and share info servers");
    server_cmd->require_subcommand(1);

    std::string server_create_name;
    auto* server_create = server_cmd->add_subcommand("create", "Create a server (you become its owner)");
    server_create->group("Server");
    server_create->add_option("name", server_create_name, "Server name (letters, digits, . _ -)")->required();

    auto* server_list = server_cmd->add_subcommand("list", "List servers you own or belong to");
    server_list->group("Server");

    std::string server_delete_ref;
    bool server_delete_yes = false;
    auto* server_delete = server_cmd->add_subcommand("delete", "Delete a server you own");
    server_delete->group("Server");
    server_delete->add_option("server", server_delete_ref, "Server name or id")->required();
    server_delete->add_flag("-y,--yes", server_delete_yes, "Skip the confirmation prompt");

    auto* server_remove = server_cmd->add_subcommand("remove", "Renamed to delete");
    server_remove->group("");

    std::string server_visibility_ref, server_visibility_value;
    auto* server_visibility = server_cmd->add_subcommand("visibility", "Make a server public or private");
    server_visibility->group("Server");
    server_visibility->add_option("server", server_visibility_ref, "Server name or id")->required();
    server_visibility->add_option("visibility", server_visibility_value, "public or private")->required();

    std::string share_ref, share_email;
    auto* server_share = server_cmd->add_subcommand("share", "Give an existing dense user access to a server");
    server_share->group("Members");
    server_share->add_option("server", share_ref, "Server name or id")->required();
    server_share->add_option("email",  share_email, "Their account email")->required();

    std::string unshare_ref, unshare_email;
    auto* server_unshare = server_cmd->add_subcommand("unshare", "Remove a member (or leave, using your own email)");
    server_unshare->group("Members");
    server_unshare->add_option("server", unshare_ref, "Server name or id")->required();
    server_unshare->add_option("email",  unshare_email, "Member's email")->required();

    std::string members_ref;
    auto* server_members = server_cmd->add_subcommand("members", "List who has access to a server");
    server_members->group("Members");
    server_members->add_option("server", members_ref, "Server name or id")->required();

    std::string server_sources_ref;
    auto* server_sources = server_cmd->add_subcommand("sources", "List the folders feeding a server");
    server_sources->group("Folders");
    server_sources->add_option("server", server_sources_ref, "Server name or id")->required();

    std::string server_url_ref;
    auto* server_url = server_cmd->add_subcommand("url", "Show the URL to connect Claude to a server");
    server_url->group("Agents");
    server_url->add_option("server", server_url_ref, "Server name (or id if two share a name)")->required();

    auto* key_cmd = server_cmd->add_subcommand("key", "Manage the keys agents use to read a server");
    key_cmd->require_subcommand(1);

    std::string key_create_ref, key_create_label;
    auto* key_create = key_cmd->add_subcommand("create", "Mint a key for an agent (shown once)");
    key_create->add_option("server", key_create_ref, "Server name or id")->required();
    key_create->add_option("--label", key_create_label, "A name to tell keys apart, e.g. 'laptop-claude'");

    std::string key_list_ref;
    auto* key_list = key_cmd->add_subcommand("list", "List keys (all if you own the server, otherwise yours)");
    key_list->add_option("server", key_list_ref, "Server name or id")->required();

    std::string key_revoke_ref, key_revoke_id;
    auto* key_revoke = key_cmd->add_subcommand("revoke", "Revoke a key (any key if you own the server, otherwise yours)");
    key_revoke->add_option("server", key_revoke_ref, "Server name or id")->required();
    key_revoke->add_option("key_id", key_revoke_id, "Key id from `key list` (a unique prefix works)")->required();

    // After key's own subcommands exist, so they don't inherit this group.
    key_cmd->group("Agents");
    server_cmd->group("Servers");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    if      (signup_cmd->parsed())         return dense::cli::handle_signup(signup_email);
    else if (login_cmd->parsed())          return dense::cli::handle_login(login_email);
    else if (init_cmd->parsed())           return dense::cli::handle_init(init_path);
    else if (connect_cmd->parsed())        return dense::cli::handle_connect(connect_server);
    else if (disconnect_cmd->parsed())     return dense::cli::handle_disconnect(disconnect_server);
    else if (push_cmd->parsed())           return dense::cli::handle_push();
    else if (status_cmd->parsed())         return dense::cli::handle_status();
    else if (list_cmd->parsed()) {
        std::cerr << "dense list: renamed to `dense status`\n";
        return 1;
    }
    else if (server_create->parsed())      return dense::cli::handle_server_create(server_create_name);
    else if (server_list->parsed())        return dense::cli::handle_server_list();
    else if (server_sources->parsed())     return dense::cli::handle_server_sources(server_sources_ref);
    else if (server_url->parsed())         return dense::cli::handle_server_url(server_url_ref);
    else if (server_delete->parsed())      return dense::cli::handle_server_delete(server_delete_ref, server_delete_yes);
    else if (server_visibility->parsed())  return dense::cli::handle_server_visibility(server_visibility_ref, server_visibility_value);
    else if (server_remove->parsed()) {
        std::cerr << "dense server remove: renamed to `dense server delete`\n";
        return 1;
    }
    else if (server_share->parsed())       return dense::cli::handle_server_share(share_ref, share_email);
    else if (server_unshare->parsed())     return dense::cli::handle_server_unshare(unshare_ref, unshare_email);
    else if (server_members->parsed())     return dense::cli::handle_server_members(members_ref);
    else if (key_create->parsed())         return dense::cli::handle_server_key_create(key_create_ref, key_create_label);
    else if (key_list->parsed())           return dense::cli::handle_server_key_list(key_list_ref);
    else if (key_revoke->parsed())         return dense::cli::handle_server_key_revoke(key_revoke_ref, key_revoke_id);

    return 0;
}

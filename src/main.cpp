#include <CLI/CLI.hpp>
#include <filesystem>
#include <string>

#include "cli/commands.hpp"

int main(int argc, char** argv) {
    CLI::App app("dense — local file tracker for dense info servers");
    app.require_subcommand(1);

    // ── account ──────────────────────────────────────────────────────────────

    // dense signup [--email <email>]
    std::string signup_email;
    auto* signup_cmd = app.add_subcommand("signup", "Create a dense account and log in");
    signup_cmd->add_option("--email", signup_email, "Email (prompted if omitted)");

    // dense login [--email <email>]
    std::string login_email;
    auto* login_cmd = app.add_subcommand("login", "Log in to your dense account");
    login_cmd->add_option("--email", login_email, "Email (prompted if omitted)");

    // ── servers ──────────────────────────────────────────────────────────────

    auto* server_cmd = app.add_subcommand("server", "Create and share info servers");
    server_cmd->require_subcommand(1);

    std::string server_create_name;
    auto* server_create = server_cmd->add_subcommand("create", "Create a server (you become its owner)");
    server_create->add_option("name", server_create_name, "Server name (letters, digits, . _ -)")->required();

    auto* server_list = server_cmd->add_subcommand("list", "List servers you own or belong to");

    std::string server_url_ref;
    auto* server_url = server_cmd->add_subcommand("url", "Show the URL to connect Claude or ChatGPT to a server");
    server_url->add_option("server", server_url_ref, "Server name (or id if two share a name)")->required();

    std::string server_remove_ref;
    bool server_remove_yes = false;
    auto* server_remove = server_cmd->add_subcommand("remove", "Delete a server you own");
    server_remove->add_option("server", server_remove_ref, "Server name or id")->required();
    server_remove->add_flag("-y,--yes", server_remove_yes, "Skip the confirmation prompt");

    std::string share_ref, share_email;
    auto* server_share = server_cmd->add_subcommand("share", "Give an existing dense user access to a server");
    server_share->add_option("server", share_ref, "Server name or id")->required();
    server_share->add_option("email",  share_email, "Their account email")->required();

    std::string unshare_ref, unshare_email;
    auto* server_unshare = server_cmd->add_subcommand("unshare", "Remove a member (or leave, using your own email)");
    server_unshare->add_option("server", unshare_ref, "Server name or id")->required();
    server_unshare->add_option("email",  unshare_email, "Member's email")->required();

    std::string members_ref;
    auto* server_members = server_cmd->add_subcommand("members", "List who has access to a server");
    server_members->add_option("server", members_ref, "Server name or id")->required();

    // dense server key create|list|revoke <server> — credentials for agents (MCP)
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

    // ── this directory ───────────────────────────────────────────────────────

    // dense init [path]
    std::filesystem::path init_path = std::filesystem::current_path();
    auto* init_cmd = app.add_subcommand("init", "Initialize a dense source in a directory");
    init_cmd->add_option("path", init_path, "Directory to initialize (default: cwd)")
            ->check(CLI::ExistingDirectory);

    // dense connect <server>
    std::string connect_server;
    auto* connect_cmd = app.add_subcommand("connect", "Feed this directory into a server");
    connect_cmd->add_option("server", connect_server, "Server name or id")->required();

    // dense push
    auto* push_cmd = app.add_subcommand("push", "Upload new/changed files to the servers this directory feeds");

    // dense list
    auto* list_cmd = app.add_subcommand("list", "Show connection info for the current directory");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    if      (signup_cmd->parsed())     dense::cli::handle_signup(signup_email);
    else if (login_cmd->parsed())      dense::cli::handle_login(login_email);
    else if (server_create->parsed())  dense::cli::handle_server_create(server_create_name);
    else if (server_list->parsed())    dense::cli::handle_server_list();
    else if (server_url->parsed())     dense::cli::handle_server_url(server_url_ref);
    else if (server_remove->parsed())  dense::cli::handle_server_remove(server_remove_ref, server_remove_yes);
    else if (server_share->parsed())   dense::cli::handle_server_share(share_ref, share_email);
    else if (server_unshare->parsed()) dense::cli::handle_server_unshare(unshare_ref, unshare_email);
    else if (server_members->parsed()) dense::cli::handle_server_members(members_ref);
    else if (key_create->parsed())     dense::cli::handle_server_key_create(key_create_ref, key_create_label);
    else if (key_list->parsed())       dense::cli::handle_server_key_list(key_list_ref);
    else if (key_revoke->parsed())     dense::cli::handle_server_key_revoke(key_revoke_ref, key_revoke_id);
    else if (init_cmd->parsed())       dense::cli::handle_init(init_path);
    else if (connect_cmd->parsed())    dense::cli::handle_connect(connect_server);
    else if (push_cmd->parsed())       dense::cli::handle_push();
    else if (list_cmd->parsed())       dense::cli::handle_list();

    return 0;
}

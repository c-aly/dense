#pragma once
#include <filesystem>
#include <string>

namespace dense::cli {

// ── account ──────────────────────────────────────────────────────────────────
// Create an account / log in. Both store the returned session token in
// ~/.dense/config.json. Email comes from the flag or a prompt; the password is
// always prompted (never a flag — it would land in shell history).
void handle_signup(const std::string& email);
void handle_login(const std::string& email);

// ── servers ──────────────────────────────────────────────────────────────────
// `server` arguments are a server name, or its id when two names collide.
void handle_server_create(const std::string& name);
void handle_server_list();
void handle_server_remove(const std::string& server, bool yes);
void handle_server_share(const std::string& server, const std::string& email);
void handle_server_unshare(const std::string& server, const std::string& email);
void handle_server_members(const std::string& server);

// ── agent keys for a server's /mcp endpoint ──────────────────────────────────
void handle_server_key_create(const std::string& server, const std::string& label);
void handle_server_key_list(const std::string& server);
void handle_server_key_revoke(const std::string& server, const std::string& key_id);

// ── this directory ───────────────────────────────────────────────────────────
void handle_init(const std::filesystem::path& path);
void handle_connect(const std::string& server);
void handle_push();
void handle_list();

} // namespace dense::cli

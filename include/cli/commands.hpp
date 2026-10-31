#pragma once
#include <filesystem>
#include <string>

namespace dense::cli {

// Commands return 0 on success and 1 on failure. A declined confirmation is success.

// ── account ──────────────────────────────────────────────────────────────────
// Create an account / log in. Both store the returned session token in
// ~/.dense/config.json. Email comes from the flag or a prompt; the password is
// always prompted (never a flag — it would land in shell history).
// Signup (commands/signup.cpp) emails a verification code, then asks for the code and a
// new password. Login (commands/login.cpp) is just email + password.
int handle_signup(const std::string& email);
int handle_login(const std::string& email);

// ── servers ──────────────────────────────────────────────────────────────────
// `server` arguments are a server name, or its id when two names collide.
int handle_server_create(const std::string& name);
int handle_server_list();
int handle_server_delete(const std::string& server, bool yes);
int handle_server_visibility(const std::string& server, const std::string& visibility);
int handle_server_share(const std::string& server, const std::string& email);
int handle_server_unshare(const std::string& server, const std::string& email);
int handle_server_members(const std::string& server);
int handle_server_url(const std::string& server);   // the URL to give a browser agent (Claude, ChatGPT)
int handle_server_sources(const std::string& server);

// ── agent keys for a server's /mcp endpoint ──────────────────────────────────
int handle_server_key_create(const std::string& server, const std::string& label);
int handle_server_key_list(const std::string& server);
int handle_server_key_revoke(const std::string& server, const std::string& key_id);

// ── this directory ───────────────────────────────────────────────────────────
int handle_init(const std::filesystem::path& path);
int handle_connect(const std::string& server);
int handle_disconnect(const std::string& server);
int handle_push();
int handle_status();

} // namespace dense::cli

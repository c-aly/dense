#pragma once
// Helpers shared by the command implementations in src/cli/commands/.
// Not part of the public CLI surface — main.cpp only needs cli/commands.hpp.

#include <exception>
#include <optional>
#include <string>

#include "http/client.hpp"
#include "storage/global.hpp"

namespace dense::cli::detail {

// Load global config and require a logged-in user session, or print an actionable error.
std::optional<Global::Config> require_login();

// Print "dense <cmd>: <error>" to stderr.
void report_error(const std::string& cmd, const std::exception& e);

// Email from the flag, or prompt for it. Trims surrounding whitespace.
std::string ask_email(const std::string& from_flag);

// Persist a fresh user session.
void save_session(const Http::AuthResult& auth);

// Find one of the caller's servers (owned or shared) by name or by id.
// Prints an error and returns nullopt if there's no match or the name is ambiguous
// (two people can each own a server called "docs"; the id disambiguates).
// May throw on network/API errors — call inside a try block.
std::optional<Http::ServerInfo> resolve_server(Http::DenseClient& client,
                                               const std::string& ref,
                                               const std::string& cmd);

// Drop `server_name` from the current directory's list of servers it feeds (if it's a
// dense directory and lists it). Call after a server is deleted or you leave it.
void forget_local_server(const std::string& server_name);

// ASCII lowercase (emails).
std::string lower(std::string s);

} // namespace dense::cli::detail

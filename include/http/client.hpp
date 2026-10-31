#pragma once
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace Http {

// dense server URL — reads DENSE_URL env var, falls back to production URL.
// Override in dev: DENSE_URL=http://localhost:8000
const char* dense_url();

// The MCP endpoint for agents: `<url>/mcp`, or `<url>/mcp/<server_id>` for one server's own
// URL (what browser agents such as Claude and ChatGPT connect to; they sign in with OAuth).
std::string mcp_url(const std::string& server_id = {});

struct AuthResult    { std::string user_id, email, token; };            // signup / login
struct ServerInfo    { std::string server_id, name, owner_id, owner_email, role; };  // role: owner|member
struct MemberInfo    { std::string user_id, email, role; };             // role: owner|member
struct NewServerKey  { std::string key_id, key, hint, label; };       // `key` is shown once
struct ServerKeyInfo { std::string key_id, hint, label, created_by_email, created_at; bool mine = false; };
struct ConnectResult { std::string server_id, source_id; bool already_linked = false; };
struct SourceInfo    { std::string source_id, name, type, owner_id, owner_email; };
struct UploadResult  { std::string file_id, status; bool skipped = false; };

// Non-2xx response. what() is "HTTP <code> — <server's detail message>".
struct ApiError : std::runtime_error {
    int status_code;
    ApiError(int code, const std::string& msg)
        : std::runtime_error(msg), status_code(code) {}
};

// One client, one credential: the user's session token (empty for signup/login).
class DenseClient {
public:
    explicit DenseClient(std::string session_token = {});

    // ── auth ────────────────────────────────────────────────────────────────
    // Signup is two calls: the server emails a code, then the code + new password create the account.
    // request_signup_code throws ApiError(409) if the email is registered, 429 if a code was
    //                     just sent, 503 if the server can't send email.
    // signup              throws ApiError(400) for a wrong code (retry), 410 if the code expired
    //                     or was used up (request a new one), 409 if the email is registered.
    // login               throws ApiError(401) on bad credentials.
    void       request_signup_code(const std::string& email);
    AuthResult signup(const std::string& email, const std::string& code, const std::string& password);
    AuthResult login (const std::string& email, const std::string& password);

    // ── servers ─────────────────────────────────────────────────────────────
    ServerInfo              create_server(const std::string& name);   // 409 if you own that name
    std::vector<ServerInfo> list_servers();                           // owned + shared with you
    void                    delete_server(const std::string& server_id);   // owner only
    // Owner only. visibility is "public" or "private". Servers are created private.
    void                    set_server_visibility(const std::string& server_id, const std::string& visibility);

    // ── members ─────────────────────────────────────────────────────────────
    MemberInfo              share_server (const std::string& server_id, const std::string& email);
    std::vector<MemberInfo> list_members (const std::string& server_id);
    void                    remove_member(const std::string& server_id, const std::string& user_id);

    // ── server keys (agent credentials for /mcp) ────────────────────────────
    // Any member can mint. The raw key is only returned here, never again.
    NewServerKey              create_server_key(const std::string& server_id, const std::string& label);
    std::vector<ServerKeyInfo> list_server_keys (const std::string& server_id);   // owner: all, member: own
    void                      revoke_server_key(const std::string& server_id, const std::string& key_id);

    // ── sources ─────────────────────────────────────────────────────────────
    // Link this folder's source to a server you can access. source_id empty means
    // create a new source (the returned id is the folder's identity from then on).
    // Throws ApiError(404) if the server isn't visible, or if source_id isn't yours.
    ConnectResult connect(const std::string& server_id,
                          const std::string& source_name,
                          const std::string& source_id = {});

    // Folders currently feeding a server, and removing one of those links.
    // Unlink keeps the folder's files; other servers it still feeds are untouched.
    // Either the folder's owner or the server's owner may unlink.
    std::vector<SourceInfo> list_sources(const std::string& server_id);
    void                    unlink_source(const std::string& server_id, const std::string& source_id);

    // ── ingest ──────────────────────────────────────────────────────────────
    // Only the source's owner may upload. Caller provides pre-read bytes and
    // a pre-computed hash.
    UploadResult upload_file(
        const std::string&           source_id,
        const std::filesystem::path& rel_path,
        const std::string&           format,
        const std::string&           content_hash,
        const std::string&           content_bytes
    );

private:
    std::string token_;
};

} // namespace Http

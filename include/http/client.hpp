#pragma once
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace Http {

// dense server URL — reads DENSE_URL env var, falls back to production URL.
// Override in dev: DENSE_URL=http://localhost:8000
const char* dense_url();

struct AuthResult    { std::string user_id, email, token; };            // signup / login
struct ServerInfo    { std::string server_id, name, owner_id, owner_email, role; };  // role: owner|member
struct MemberInfo    { std::string user_id, email, role; };             // role: owner|member
struct NewServerKey  { std::string key_id, key, hint, label; };       // `key` is shown once
struct ServerKeyInfo { std::string key_id, hint, label, created_by_email, created_at; bool mine = false; };
struct ConnectResult { std::string server_id, source_id; };
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
    // signup throws ApiError(409) if the email is already registered.
    // login   throws ApiError(401) on bad credentials.
    AuthResult signup(const std::string& email, const std::string& password);
    AuthResult login (const std::string& email, const std::string& password);

    // ── servers ─────────────────────────────────────────────────────────────
    ServerInfo              create_server(const std::string& name);   // 409 if you own that name
    std::vector<ServerInfo> list_servers();                           // owned + shared with you
    void                    delete_server(const std::string& server_id);   // owner only

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
    // Register this machine's source and link it to a server you can access.
    // Idempotent. Throws ApiError(404) if the server isn't visible to you.
    ConnectResult connect(const std::string& server_id,
                          const std::string& source_name,
                          const std::string& device);

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

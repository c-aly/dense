#include "http/client.hpp"

#include <httplib.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace Http {

// ── URL ───────────────────────────────────────────────────────────────────────

const char* dense_url() {
    if (const char* env = std::getenv("DENSE_URL")) return env;
    return "https://api.dense.io";
}

// ── internal helpers ──────────────────────────────────────────────────────────

// FastAPI errors look like {"detail": "message"} (or a list for validation errors).
// Prefer the human-readable message; fall back to the raw body.
static std::string error_detail(const std::string& body) {
    auto j = json::parse(body, nullptr, /*allow_exceptions=*/false);
    if (j.is_object() && j.contains("detail") && j["detail"].is_string())
        return j["detail"].get<std::string>();
    return body;
}

static void check(const httplib::Result& res, const char* label) {
    if (!res)
        throw std::runtime_error(std::string(label) + ": connection failed — " +
                                 httplib::to_string(res.error()));
    if (res->status < 200 || res->status >= 300)
        throw ApiError(res->status,
                       "HTTP " + std::to_string(res->status) + " — " + error_detail(res->body));
}

static httplib::Client make_client(const std::string& token) {
    httplib::Client cli(dense_url());
    if (!token.empty()) cli.set_bearer_token_auth(token);
    cli.set_connection_timeout(10);
    cli.set_read_timeout(30);
    return cli;
}

static AuthResult parse_auth(const std::string& body) {
    auto d = json::parse(body);
    return {d.at("user_id").get<std::string>(),
            d.at("email").get<std::string>(),
            d.at("token").get<std::string>()};
}

static ServerInfo parse_server(const json& d) {
    return {d.at("server_id").get<std::string>(),
            d.at("name").get<std::string>(),
            d.at("owner_id").get<std::string>(),
            d.at("owner_email").get<std::string>(),
            d.at("role").get<std::string>()};
}

static MemberInfo parse_member(const json& d) {
    return {d.at("user_id").get<std::string>(),
            d.at("email").get<std::string>(),
            d.at("role").get<std::string>()};
}

// label is null when unset
static std::string opt_string(const json& d, const char* key) {
    return d.contains(key) && d[key].is_string() ? d[key].get<std::string>() : std::string();
}

static ServerKeyInfo parse_server_key(const json& d) {
    return {d.at("key_id").get<std::string>(),
            d.at("hint").get<std::string>(),
            opt_string(d, "label"),
            d.at("created_by_email").get<std::string>(),
            d.at("created_at").get<std::string>(),
            d.at("mine").get<bool>()};
}

// ── DenseClient ───────────────────────────────────────────────────────────────

DenseClient::DenseClient(std::string session_token)
    : token_(std::move(session_token)) {}


// ── auth ──────────────────────────────────────────────────────────────────────

AuthResult DenseClient::signup(const std::string& email, const std::string& password) {
    auto cli = make_client(token_);
    json body = {{"email", email}, {"password", password}};
    auto res = cli.Post("/api/v1/auth/signup", body.dump(), "application/json");
    check(res, "signup");
    return parse_auth(res->body);
}

AuthResult DenseClient::login(const std::string& email, const std::string& password) {
    auto cli = make_client(token_);
    json body = {{"email", email}, {"password", password}};
    auto res = cli.Post("/api/v1/auth/login", body.dump(), "application/json");
    check(res, "login");
    return parse_auth(res->body);
}


// ── servers ───────────────────────────────────────────────────────────────────

ServerInfo DenseClient::create_server(const std::string& name) {
    auto cli = make_client(token_);
    json body = {{"name", name}};
    auto res = cli.Post("/api/v1/servers", body.dump(), "application/json");
    check(res, "create_server");
    return parse_server(json::parse(res->body));
}

std::vector<ServerInfo> DenseClient::list_servers() {
    auto cli = make_client(token_);
    auto res = cli.Get("/api/v1/servers");
    check(res, "list_servers");
    std::vector<ServerInfo> out;
    for (const auto& d : json::parse(res->body)) out.push_back(parse_server(d));
    return out;
}

void DenseClient::delete_server(const std::string& server_id) {
    auto cli = make_client(token_);
    auto res = cli.Delete("/api/v1/servers/" + server_id);
    check(res, "delete_server");
}


// ── members ───────────────────────────────────────────────────────────────────

MemberInfo DenseClient::share_server(const std::string& server_id, const std::string& email) {
    auto cli = make_client(token_);
    json body = {{"email", email}};
    auto res = cli.Post("/api/v1/servers/" + server_id + "/members", body.dump(), "application/json");
    check(res, "share_server");
    return parse_member(json::parse(res->body));
}

std::vector<MemberInfo> DenseClient::list_members(const std::string& server_id) {
    auto cli = make_client(token_);
    auto res = cli.Get("/api/v1/servers/" + server_id + "/members");
    check(res, "list_members");
    std::vector<MemberInfo> out;
    for (const auto& d : json::parse(res->body)) out.push_back(parse_member(d));
    return out;
}

void DenseClient::remove_member(const std::string& server_id, const std::string& user_id) {
    auto cli = make_client(token_);
    auto res = cli.Delete("/api/v1/servers/" + server_id + "/members/" + user_id);
    check(res, "remove_member");
}


// ── server keys ───────────────────────────────────────────────────────────────

NewServerKey DenseClient::create_server_key(const std::string& server_id, const std::string& label) {
    auto cli = make_client(token_);
    json body = json::object();
    if (!label.empty()) body["label"] = label;
    auto res = cli.Post("/api/v1/servers/" + server_id + "/keys", body.dump(), "application/json");
    check(res, "create_server_key");
    auto d = json::parse(res->body);
    return {d.at("key_id").get<std::string>(),
            d.at("key").get<std::string>(),
            d.at("hint").get<std::string>(),
            opt_string(d, "label")};
}

std::vector<ServerKeyInfo> DenseClient::list_server_keys(const std::string& server_id) {
    auto cli = make_client(token_);
    auto res = cli.Get("/api/v1/servers/" + server_id + "/keys");
    check(res, "list_server_keys");
    std::vector<ServerKeyInfo> out;
    for (const auto& d : json::parse(res->body)) out.push_back(parse_server_key(d));
    return out;
}

void DenseClient::revoke_server_key(const std::string& server_id, const std::string& key_id) {
    auto cli = make_client(token_);
    auto res = cli.Delete("/api/v1/servers/" + server_id + "/keys/" + key_id);
    check(res, "revoke_server_key");
}


// ── sources ───────────────────────────────────────────────────────────────────

ConnectResult DenseClient::connect(const std::string& server_id,
                                   const std::string& source_name,
                                   const std::string& device)
{
    auto cli = make_client(token_);
    json body = {
        {"server_id",   server_id},
        {"source_name", source_name},
        {"device",      device},
    };
    auto res = cli.Post("/api/v1/sources/connect", body.dump(), "application/json");
    check(res, "connect");
    auto data = json::parse(res->body);
    return {data.at("server_id").get<std::string>(), data.at("source_id").get<std::string>()};
}


// ── ingest ────────────────────────────────────────────────────────────────────

UploadResult DenseClient::upload_file(const std::string&           source_id,
                                      const std::filesystem::path& rel_path,
                                      const std::string&           format,
                                      const std::string&           content_hash,
                                      const std::string&           content_bytes)
{
    auto cli = make_client(token_);
    auto url = "/api/v1/ingest/sources/" + source_id + "/files";

    httplib::UploadFormDataItems items = {
        {"rel_path",     rel_path.generic_string(),              "", ""},
        {"content_hash", content_hash,                           "", ""},
        {"size",         std::to_string(content_bytes.size()),   "", ""},
        {"format",       format,                                 "", ""},
        {"file",         content_bytes, rel_path.filename().string(), "application/octet-stream"},
    };

    auto res = cli.Post(url, items);
    check(res, "upload_file");

    auto data = json::parse(res->body);
    return {
        data.value("file_id",  std::string{}),
        data.value("status",   std::string{}),
        data.value("skipped",  false),
    };
}

} // namespace Http

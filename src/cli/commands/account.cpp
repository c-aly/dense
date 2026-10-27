// dense signup / dense login
#include "cli/commands.hpp"
#include "cli/commands/common.hpp"
#include "cli/prompt.hpp"
#include "http/client.hpp"

#include <iostream>

namespace {
constexpr size_t kMinPasswordLen = 8;   // mirrors the server (api/auth.py); server is the source of truth
}

namespace dense::cli {

using detail::ask_email;
using detail::save_session;

void handle_signup(const std::string& email_flag) {
    const std::string email = ask_email(email_flag);
    if (email.empty()) { std::cerr << "dense signup: email is required.\n"; return; }

    const std::string pw = prompt_secret("Password: ");
    if (pw.size() < kMinPasswordLen) {
        std::cerr << "dense signup: password must be at least " << kMinPasswordLen << " characters.\n";
        return;
    }
    if (prompt_secret("Confirm password: ") != pw) {
        std::cerr << "dense signup: passwords don't match.\n";
        return;
    }

    try {
        Http::DenseClient client;
        auto auth = client.signup(email, pw);
        save_session(auth);
        std::cout << "Signed up and logged in as " << auth.email << ".\n"
                  << "Next: `dense server create <name>`.\n";
    } catch (const Http::ApiError& e) {
        std::cerr << "dense signup: " << e.what() << "\n";
        if (e.status_code == 409)
            std::cerr << "  (already have an account? run `dense login`)\n";
    } catch (const std::exception& e) {
        std::cerr << "dense signup: " << e.what() << "\n";
    }
}

void handle_login(const std::string& email_flag) {
    const std::string email = ask_email(email_flag);
    if (email.empty()) { std::cerr << "dense login: email is required.\n"; return; }
    const std::string pw = prompt_secret("Password: ");

    try {
        Http::DenseClient client;
        auto auth = client.login(email, pw);
        save_session(auth);
        std::cout << "Logged in as " << auth.email << ".\n";
    } catch (const Http::ApiError& e) {
        std::cerr << "dense login: " << e.what() << "\n";
    } catch (const std::exception& e) {
        std::cerr << "dense login: " << e.what() << "\n";
    }
}

} // namespace dense::cli

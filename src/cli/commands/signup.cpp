// dense signup: email → emailed code → choose a password.
// Everything about proving the email lives here; the server decides how the code is
// sent and checked (the CLI only knows "a code was emailed" and "submit it").
#include "cli/commands.hpp"
#include "cli/commands/common.hpp"
#include "cli/prompt.hpp"
#include "http/client.hpp"

#include <iostream>

namespace {
using dense::cli::prompt;
using dense::cli::prompt_secret;

constexpr size_t kMinPasswordLen = 8;   // mirrors the server (api/auth.py); server is the source of truth
constexpr int    kMaxCodeTries   = 3;   // the verification service locks a code after this many wrong tries

std::string trim(const std::string& s) {
    const auto b = s.find_first_not_of(" \t\r\n");
    const auto e = s.find_last_not_of(" \t\r\n");
    return b == std::string::npos ? std::string{} : s.substr(b, e - b + 1);
}

// Ask for a new password twice. Empty string if it's unusable (already reported).
std::string ask_new_password() {
    const std::string pw = prompt_secret("Choose a password: ");
    if (pw.size() < kMinPasswordLen) {
        std::cerr << "dense signup: password must be at least " << kMinPasswordLen << " characters.\n";
        return {};
    }
    if (prompt_secret("Confirm password: ") != pw) {
        std::cerr << "dense signup: passwords don't match.\n";
        return {};
    }
    return pw;
}
} // namespace

namespace dense::cli {

using detail::ask_email;
using detail::save_session;

int handle_signup(const std::string& email_flag) {
    const std::string email = ask_email(email_flag);
    if (email.empty()) { std::cerr << "dense signup: email is required.\n"; return 1; }

    try {
        Http::DenseClient client;

        // 1. email a code (also rejects an address that already has an account)
        client.request_signup_code(email);
        std::cout << "We emailed a verification code to " << email << ".\n";

        // 2. the code, then the password. A wrong code only re-asks for the code.
        std::string code = trim(prompt("Code: "));
        if (code.empty()) { std::cerr << "dense signup: no code entered.\n"; return 1; }

        const std::string pw = ask_new_password();
        if (pw.empty()) return 1;

        for (int attempt = 1;; ++attempt) {
            try {
                auto auth = client.signup(email, code, pw);
                save_session(auth);
                std::cout << "Signed up and logged in as " << auth.email << ".\n"
                          << "Next: `dense server create <name>`.\n";
                return 0;
            } catch (const Http::ApiError& e) {
                if (e.status_code != 400) throw;              // 410 expired, 409 taken, … → outer handler
                std::cerr << "dense signup: " << e.what() << "\n";
                if (attempt >= kMaxCodeTries) {
                    std::cerr << "  Run `dense signup` again for a new code.\n";
                    return 1;
                }
                code = trim(prompt("Code: "));
                if (code.empty()) { std::cerr << "dense signup: no code entered.\n"; return 1; }
            }
        }
    } catch (const Http::ApiError& e) {
        std::cerr << "dense signup: " << e.what() << "\n";
        if (e.status_code == 409)
            std::cerr << "  (already have an account? run `dense login`)\n";
        else if (e.status_code == 410)
            std::cerr << "  Run `dense signup` again for a new code.\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "dense signup: " << e.what() << "\n";
        return 1;
    }
}

} // namespace dense::cli

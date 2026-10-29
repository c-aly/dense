// dense login
#include "cli/commands.hpp"
#include "cli/commands/common.hpp"
#include "cli/prompt.hpp"
#include "http/client.hpp"

#include <iostream>

namespace dense::cli {

using detail::ask_email;
using detail::save_session;

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

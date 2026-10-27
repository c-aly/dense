// dense push
#include "cli/commands.hpp"
#include "cli/commands/common.hpp"
#include "core/hash.hpp"
#include "core/scanner.hpp"
#include "http/client.hpp"
#include "storage/global.hpp"
#include "storage/local.hpp"

#include <fstream>
#include <iostream>

namespace {

std::string read_file(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) throw std::runtime_error("cannot read " + p.string());
    return {std::istreambuf_iterator<char>{f}, {}};
}

} // namespace

namespace dense::cli {

using detail::report_error;
using detail::require_login;

void handle_push() {
    const auto cwd = std::filesystem::current_path();

    auto local = Local::load(cwd);
    if (!local) {
        std::cerr << "dense push: not a dense directory — run `dense init` first.\n";
        return;
    }
    if (!local->connected()) {
        std::cerr << "dense push: source not connected — run `dense connect` first.\n";
        return;
    }

    auto global = require_login();
    if (!global) return;

    // merge global + local ignore patterns
    auto ignore = global->ignore;
    auto local_ignore = Local::load_ignore(cwd);
    ignore.insert(ignore.end(), local_ignore.begin(), local_ignore.end());

    std::cout << "Scanning " << cwd << " ...\n";
    ScanResult scan = scan_directory(cwd, ignore);
    std::cout << scan.stats.to_string() << "\n\n";

    if (scan.files.empty()) { std::cout << "Nothing to push.\n"; return; }

    Http::DenseClient client(global->user_token);
    int uploaded = 0, skipped = 0, failed = 0;

    for (const FileInfo& fi : scan.files) {
        try {
            std::string content = read_file(fi.abs_path);
            std::string hash    = hash_bytes(content);
            auto result = client.upload_file(
                local->source_id, fi.rel_path, fi.format, hash, content);

            if (result.skipped) {
                std::cout << "  skip  " << fi.rel_path.generic_string() << "\n";
                ++skipped;
            } else {
                std::cout << "  push  " << fi.rel_path.generic_string() << "\n";
                ++uploaded;
            }
        } catch (const Http::ApiError& e) {
            if (e.status_code == 401) {          // every remaining file would fail the same way
                report_error("push", e);
                ++failed;
                break;
            }
            std::cerr << "  fail  " << fi.rel_path.generic_string()
                      << " — " << e.what() << "\n";
            ++failed;
        } catch (const std::exception& e) {
            std::cerr << "  fail  " << fi.rel_path.generic_string()
                      << " — " << e.what() << "\n";
            ++failed;
        }
    }
    std::cout << "\nDone. " << uploaded << " uploaded, "
              << skipped << " skipped, " << failed << " failed.\n";
}

} // namespace dense::cli

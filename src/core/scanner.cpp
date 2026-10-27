#include "core/scanner.hpp"

#include <algorithm>
#include <string>
#include <system_error>
#include <unordered_set>

namespace fs = std::filesystem;

namespace {

// ── format detection ──────────────────────────────────────────────────────────

// Returns the lowercase extension (without dot) if the file is a supported text
// or document type. Returns "" to indicate the file should be skipped.
std::string format_for(const fs::path& p) {
    std::string ext = p.extension().string();
    if (ext.size() < 2 || ext[0] != '.') return "";    // no extension
    ext = ext.substr(1);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (ext == "markdown") return "md";   // normalise

    // supported: markup, code, config, documents
    // anything else (images, binaries, archives) is skipped
    static const std::unordered_set<std::string> supported = {
        // markup / text
        "md", "txt", "rst", "html", "htm", "xml", "csv",
        // code
        "py", "js", "ts", "jsx", "tsx", "go",
        "cpp", "cc", "cxx", "c", "h", "hpp", "hxx",
        "java", "rb", "rs", "swift", "kt", "cs", "php",
        "r", "scala", "lua", "pl", "sh", "bash", "zsh",
        "sql", "css", "scss", "sass", "vue", "svelte",
        // config / data
        "json", "yaml", "yml", "toml", "ini",
        // documents
        "pdf", "docx", "pptx", "xlsx", "odt", "epub", "rtf",
    };
    return supported.count(ext) ? ext : "";
}

// ── pattern matching ──────────────────────────────────────────────────────────

// Returns true if the entry (file or directory) matches any ignore pattern.
// Patterns are matched against the entry's filename only (not its full path).
//   "build"   → exact name match (works for dirs and files)
//   "*.log"   → extension match for files
bool matches_ignore(const fs::path& p, bool is_dir,
                    const std::vector<std::string>& patterns)
{
    const std::string name = p.filename().string();
    for (const auto& pat : patterns) {
        if (pat.size() > 2 && pat[0] == '*' && pat[1] == '.') {
            // extension glob: *.ext
            if (!is_dir && p.extension() == pat.substr(1)) return true;
        } else {
            if (name == pat) return true;
        }
    }
    return false;
}

} // namespace


// ── public ────────────────────────────────────────────────────────────────────

std::string ScanStats::to_string() const {
    return std::to_string(total)   + " file(s) found, " +
           std::to_string(skipped) + " skipped, " +
           std::to_string(failed)  + " error(s)";
}

ScanResult scan_directory(const fs::path& root,
                          const std::vector<std::string>& patterns_in)
{
    ScanResult result;

    // always skip .dense/ itself — prepend so it can't be overridden
    std::vector<std::string> patterns = {".dense"};
    patterns.insert(patterns.end(), patterns_in.begin(), patterns_in.end());

    // iterative DFS — more control than recursive_directory_iterator when
    // we need fine-grained per-entry error handling
    std::vector<fs::path> work = {root};

    while (!work.empty()) {
        fs::path dir = work.back();
        work.pop_back();

        // construct iterator; skip the entire directory on error (e.g. perm denied)
        std::error_code ec;
        fs::recursive_directory_iterator it(
            dir,
            fs::directory_options::skip_permission_denied,
            ec);
        if (ec) { ++result.stats.failed; continue; }

        for (; !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
            if (ec) { ++result.stats.failed; break; }

            const auto& entry = *it;
            std::error_code e2;

            // symlinks: skip entirely — avoids loops and out-of-tree traversal
            if (entry.is_symlink(e2) || e2) {
                it.disable_recursion_pending();
                continue;
            }

            const bool is_dir  = entry.is_directory(e2);
            if (e2) { ++result.stats.failed; it.disable_recursion_pending(); continue; }

            const bool is_file = !is_dir && entry.is_regular_file(e2);
            if (e2) { ++result.stats.failed; continue; }

            // check ignore patterns for both dirs and files
            if (matches_ignore(entry.path(), is_dir, patterns)) {
                if (is_dir)  it.disable_recursion_pending();   // prune subtree
                else         ++result.stats.skipped;
                continue;
            }

            if (!is_file) continue;

            // format check — skip binary/unsupported types
            std::string fmt = format_for(entry.path());
            if (fmt.empty()) { ++result.stats.skipped; continue; }

            fs::path rel = fs::relative(entry.path(), root, e2);
            if (e2) { ++result.stats.failed; continue; }

            uintmax_t sz = entry.file_size(e2);
            if (e2) sz = 0;

            result.files.push_back({
                .abs_path = entry.path(),
                .rel_path = std::move(rel),
                .format   = std::move(fmt),
                .size     = sz,
            });
            ++result.stats.total;
        }
        if (ec) ++result.stats.failed;
    }

    return result;
}

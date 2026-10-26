#include "core/scanner.hpp"
#include "config.hpp"
#include "core/hash.hpp"
#include "storage/database.hpp"
#include "storage/statement.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_set>
#include <vector>


namespace fs = std::filesystem;

namespace {
    // info on single file
    struct FileInfo {
        fs::path    abs;
        std::string rel;      // forward slashes; "" for a single-file source
        std::string format;
        int64_t     size;
        int64_t     mtime_ns;
    };


    // fetch format of file
    std::string format_for(const fs::path& p) {
        std::string ext = p.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
                    [](unsigned char c) { return std::tolower(c); });

        if (ext == ".md" || ext == ".markdown") return "md";
        if (ext == ".txt") return "txt";
        if (ext == ".rst") return "rst";
        return "";      // Returns "" for unsupported types.
    }


    // time since file last modified
    int64_t mtime_ns(const fs::path& p) {
        auto f_time = fs::last_write_time(p);
        auto sys_time = decltype(f_time)::clock::to_sys(f_time); // sys clock built-in converter
    
        auto t = sys_time.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(t).count();
    }


    // wrap into FileInfo struct
    FileInfo make_info(const fs::path& abs, std::string rel, std::string format) {
        return {abs, std::move(rel), std::move(format),
            static_cast<int64_t>(fs::file_size(abs)), mtime_ns(abs)};
    }


    // walk source dir, return supported files; counts everything else as skipped.
    // ignore_dirs: directory names (not paths) to skip entirely.
    std::vector<FileInfo> collect_files(const fs::path& root, const std::string& kind,
                                        const std::unordered_set<std::string>& ignore_dirs,
                                        ScanStats& stats) {
        std::vector<FileInfo> files;

        // make sure root exists and is the right type
        std::error_code ecr;
        bool ok = (kind == "dir") ? fs::is_directory(root, ecr) : fs::is_regular_file(root, ecr);
        if (ecr || !ok) throw std::runtime_error("source path missing or wrong type: " + root.string());

        // single-file source
        if (kind == "file") {
            std::string fmt = format_for(root);
            if (fmt.empty()) ++stats.skipped;
            else files.push_back(make_info(root, "", fmt));
            return files;
        }

        // directory source: recurse without following symlinks (default iterator behaviour)
        std::error_code ec;
        fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec);
        if (ec) throw std::runtime_error("cannot read " + root.string() + ": " + ec.message());

        for (; !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
            if (ec) break;

            const auto& entry = *it;
            std::error_code e2;

            // skip all symlinks (file or dir) — avoids loops and out-of-tree indexing
            if (entry.is_symlink(e2)) { it.disable_recursion_pending(); continue; }

            // directories: check ignore list; disable_recursion_pending prunes the subtree
            if (entry.is_directory(e2)) {
                if (ignore_dirs.count(entry.path().filename().string()))
                    it.disable_recursion_pending();
                continue;
            }

            if (!entry.is_regular_file(e2)) continue;

            std::string fmt = format_for(entry.path());
            if (fmt.empty()) { ++stats.skipped; continue; }

            try {
                files.push_back(make_info(entry.path(),
                    fs::relative(entry.path(), root).generic_string(), fmt));
            } catch (const fs::filesystem_error&) {
                ++stats.skipped;
            }
        }

        return files;
    }
}   // namespace end


// human readable ScanStats formatter
std::string ScanStats::to_string() const {
    return "added " + std::to_string(added) + ", changed " + std::to_string(changed) +
           ", removed " + std::to_string(removed) + " (" + std::to_string(unchanged) +
           " unchanged, " + std::to_string(skipped) + " skipped)";
}

// scan source into db, return stats
ScanStats scan_source(Database& db, int64_t source_id) {
    ScanStats stats;
    sqlite3* h = db.handle();

    // load source
    fs::path root;
    std::string kind;
    {
        Statement q(h, "SELECT path, kind FROM sources WHERE id = ?");
        q.bind_int(1, source_id);
        if (!q.step()) throw std::runtime_error("source not found");
        root = q.col_text(0);
        kind = q.col_text(1);
    }

    // load ignore list once per scan (reads dense.cfg on demand)
    const auto ignore_vec = Config::load_ignore_dirs();
    const std::unordered_set<std::string> ignore_dirs(ignore_vec.begin(), ignore_vec.end());

    std::vector<FileInfo> files = collect_files(root, kind, ignore_dirs, stats);

    // protect against silently destroying everything on collect_files failure
    if (files.empty()) {
        Statement cnt(h, "SELECT count(*) FROM documents WHERE source_id = ?");
        cnt.bind_int(1, source_id);
        cnt.step();
        if (cnt.col_int(0) > 0)
            throw std::runtime_error("refusing to remove all documents from " + root.string() +
                                     " (found no files); run `dense remove` if intended");
    }

    // prepared statements
    Statement sel(h, "SELECT id, size, mtime_ns, content_hash FROM documents "
        "WHERE source_id = ? AND rel_path = ?");
    Statement ins(h, "INSERT INTO documents(source_id, rel_path, size, mtime_ns, content_hash, "
        "format, status) VALUES (?, ?, ?, ?, ?, ?, 'pending')");
    Statement touch(h, "UPDATE documents SET size = ?, mtime_ns = ? WHERE id = ?");
    Statement upd(h, "UPDATE documents SET size = ?, mtime_ns = ?, content_hash = ?, format = ?, "
        "status = 'pending', summary = NULL, summary_model = NULL, "
        "indexed_at = NULL, error = NULL WHERE id = ?");
    Statement del_chunks(h, "DELETE FROM chunks WHERE document_id = ?");

    Transaction tx(db);     // wrap in atomic transaction (grab lock)
    std::unordered_set<std::string> seen;

    for (const FileInfo& f : files) {
        seen.insert(f.rel);     // track seen files
     
        bool found = false;
        int64_t doc_id = 0, old_size = 0, old_mtime = 0;
        std::string old_hash;

        // select all docs with matching source and relative path
        sel.bind_int(1, source_id).bind_text(2, f.rel);
        if (sel.step()) {
            found = true;
            doc_id = sel.col_int(0);
            old_size = sel.col_int(1);
            old_mtime = sel.col_int(2);
            old_hash = sel.col_text(3);
        }
        sel.reset();

        // if file unchanged, don't waste time hashing
        if (found && old_size == f.size && old_mtime == f.mtime_ns) {
            ++stats.unchanged;  
            continue;
        }

        // changed file
        std::string new_hash;
        try {
            new_hash = hash_file(f.abs);
        } catch (const std::exception&) {
            ++stats.skipped;             // unreadable; still in `seen`, so not treated as deleted
            continue;
        }

        // not found in db == new file
        if (!found) { 
            ins.bind_int(1, source_id).bind_text(2, f.rel).bind_int(3, f.size)
                .bind_int(4, f.mtime_ns).bind_text(5, new_hash).bind_text(6, f.format);
            ins.run();
            ++stats.added;

        // same hash means file 
        } else if (new_hash == old_hash) {
            touch.bind_int(1, f.size).bind_int(2, f.mtime_ns).bind_int(3, doc_id);
            touch.run();                 // touched but content identical
            ++stats.unchanged;

        // clean up stale doc info
        } else {
            del_chunks.bind_int(1, doc_id);
            del_chunks.run();            // fires the FTS delete trigger
            upd.bind_int(1, f.size).bind_int(2, f.mtime_ns).bind_text(3, new_hash)
               .bind_text(4, f.format).bind_int(5, doc_id);
            upd.run();  // clear stale stats, set to pending!
            ++stats.changed;
        }
    }

    // anything in DB but not on disk is deleted
    std::vector<int64_t> gone;      // collect deleted files under this source
    {
        Statement all(h, "SELECT id, rel_path FROM documents WHERE source_id = ?");
        all.bind_int(1, source_id);
        while (all.step())
            if (!seen.count(all.col_text(1))) gone.push_back(all.col_int(0));
    }
    
    Statement del_doc(h, "DELETE FROM documents WHERE id = ?");
    for (int64_t id : gone) {
        del_doc.bind_int(1, id);
        del_doc.run();                   // cascades to chunks and FTS (need CASCADE!)
        ++stats.removed;
    }

    tx.commit();
    return stats;
}


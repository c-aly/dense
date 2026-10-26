#include "core/sources.hpp"
#include "core/scanner.hpp"
#include "storage/database.hpp"
#include "storage/statement.hpp"

#include <ctime>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>


namespace fs = std::filesystem;

namespace {
    // true if `child` is strictly inside `parent` (both must be canonical)
    bool is_subpath(const fs::path& parent, const fs::path& child) {
        auto pit = parent.begin(), pend = parent.end();
        auto cit = child.begin(), cend = child.end();
        for (; pit != pend; ++pit, ++cit) {
            if (cit == cend || *pit != *cit) return false;
        }
        return cit != cend;     // child has extra components → strictly inside
    }
}


// ─── formatters ──────────────────────────────────────────────────────────────

std::string RemoveStats::to_string() const {
    return "removed " + std::to_string(documents) + " document(s), " +
           std::to_string(chunks) + " chunk(s)";
}

std::string SourceInfo::to_string() const {
    std::time_t t = static_cast<std::time_t>(added_at);
    char buf[32] = "unknown";
    if (std::tm* tm = std::localtime(&t))
        std::strftime(buf, sizeof(buf), "%Y-%m-%d", tm);
    return "[" + kind + "] " + path +
           " — " + std::to_string(doc_count) + " doc(s), added " + buf;
}


// ─── add_source ───────────────────────────────────────────────────────────────

ScanStats add_source(Database& db, const fs::path& raw_path) {
    std::error_code ec;
    fs::path abs = fs::canonical(raw_path, ec);
    if (ec) throw std::runtime_error("cannot resolve path '" + raw_path.string() + "': " + ec.message());

    std::string kind;
    if      (fs::is_directory(abs))    kind = "dir";
    else if (fs::is_regular_file(abs)) kind = "file";
    else throw std::runtime_error("path is not a file or directory: " + abs.string());

    sqlite3* h = db.handle();

    // ── overlap detection ────────────────────────────────────────────────────
    struct OldSource { int64_t id; fs::path root; std::string kind; };
    std::vector<OldSource> superseded;
    {
        Statement all(h, "SELECT id, path, kind FROM sources");
        while (all.step()) {
            fs::path p = all.col_text(1);
            if (p == abs) continue;                         // exact match → re-scan, skip checks
            if (is_subpath(abs, p))                         // p ⊂ abs → superseded
                superseded.push_back({all.col_int(0), p, all.col_text(2)});
            else if (is_subpath(p, abs))                    // abs ⊂ p → already covered
                throw std::runtime_error(
                    "path is already covered by source '" + p.string() + "'");
        }
    }

    // ── wrap structural changes in one transaction ───────────────────────────
    int64_t source_id;
    {
        Transaction tx(db);

        // re-key documents from superseded narrower sources to the new broader one.
        // we insert the new source first so we have an id to re-key into.
        {
            Statement ins(h,
                "INSERT OR IGNORE INTO sources(path, kind, added_at) "
                "VALUES (?, ?, strftime('%s', 'now'))");
            ins.bind_text(1, abs.string()).bind_text(2, kind);
            ins.run();
        }
        {
            Statement sel(h, "SELECT id FROM sources WHERE path = ?");
            sel.bind_text(1, abs.string());
            if (!sel.step()) throw std::runtime_error("source not found after insert: " + abs.string());
            source_id = sel.col_int(0);
        }

        for (const OldSource& old : superseded) {
            // collect documents to re-key
            struct DocRow { int64_t id; std::string rel; };
            std::vector<DocRow> docs;
            {
                Statement q(h, "SELECT id, rel_path FROM documents WHERE source_id = ?");
                q.bind_int(1, old.id);
                while (q.step()) docs.push_back({q.col_int(0), q.col_text(1)});
            }

            Statement upd(h, "UPDATE documents SET source_id = ?, rel_path = ? WHERE id = ?");
            for (const DocRow& doc : docs) {
                // absolute path of this document
                fs::path doc_abs = (old.kind == "dir")
                    ? old.root / doc.rel
                    : old.root;         // file source: rel_path is always ""

                // new rel_path relative to the broader source root
                std::error_code ec2;
                fs::path new_rel = fs::relative(doc_abs, abs, ec2);
                if (ec2) continue;      // shouldn't happen for well-formed subpath

                upd.bind_int(1, source_id)
                   .bind_text(2, new_rel.generic_string())
                   .bind_int(3, doc.id);
                upd.run();
            }

            // delete old (now empty) source — no cascade fires since docs were moved
            Statement del(h, "DELETE FROM sources WHERE id = ?");
            del.bind_int(1, old.id);
            del.run();
        }

        tx.commit();
    }

    // scan_source handles its own transaction; re-keyed docs match by (source_id, rel_path)
    // so unchanged files are detected as-is; only truly changed files get re-chunked
    return scan_source(db, source_id);
}


// ─── remove_source ────────────────────────────────────────────────────────────

RemoveStats remove_source(Database& db, const fs::path& raw_path) {
    std::error_code ec;
    fs::path abs = fs::canonical(raw_path, ec);
    if (ec) throw std::runtime_error("cannot resolve path '" + raw_path.string() + "': " + ec.message());

    sqlite3* h = db.handle();

    int64_t source_id;
    {
        Statement sel(h, "SELECT id FROM sources WHERE path = ?");
        sel.bind_text(1, abs.string());
        if (!sel.step()) throw std::runtime_error("source not tracked: " + abs.string());
        source_id = sel.col_int(0);
    }

    // snapshot counts before deletion (cascade makes post-hoc counting impossible)
    RemoveStats stats;
    {
        Statement cnt(h,
            "SELECT COUNT(*), COALESCE(SUM(c.n), 0) "
            "FROM documents d "
            "LEFT JOIN (SELECT document_id, COUNT(*) AS n FROM chunks GROUP BY document_id) c "
            "ON c.document_id = d.id "
            "WHERE d.source_id = ?");
        cnt.bind_int(1, source_id);
        if (cnt.step()) {
            stats.documents = static_cast<int>(cnt.col_int(0));
            stats.chunks    = static_cast<int>(cnt.col_int(1));
        }
    }

    // delete source — cascades: source → documents → chunks → FTS trigger
    {
        Statement del(h, "DELETE FROM sources WHERE id = ?");
        del.bind_int(1, source_id);
        del.run();
    }

    return stats;
}


// ─── list_sources ─────────────────────────────────────────────────────────────

std::vector<SourceInfo> list_sources(Database& db) {
    std::vector<SourceInfo> out;
    Statement q(db.handle(),
        "SELECT s.id, s.path, s.kind, s.added_at, COUNT(d.id) "
        "FROM sources s LEFT JOIN documents d ON d.source_id = s.id "
        "GROUP BY s.id ORDER BY s.added_at");
    while (q.step()) {
        out.push_back({
            q.col_int(0),
            q.col_text(1),
            q.col_text(2),
            q.col_int(3),
            static_cast<int>(q.col_int(4))
        });
    }
    return out;
}

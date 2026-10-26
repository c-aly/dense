#include "storage/database.hpp"
#include <filesystem>
#include <sqlite3.h>
#include <stdexcept>
#include <string>


// TODO: add FTS index on files? Or at least somehow permit file search.
// TODO: this might be too heavy, we're storing full filetext in chunks,
//       could just fetch it when needed by lines + file path

namespace {
    // SQL migrations, always add never remove
    const char* const MIGRATIONS[] = {
        // migratioon 1: create key tables
        R"SQL(
            CREATE TABLE sources (
            id        INTEGER PRIMARY KEY,
            path      TEXT NOT NULL UNIQUE,        -- canonical absolute path
            kind      TEXT NOT NULL,               -- 'dir' | 'file'
            added_at  INTEGER NOT NULL
            );

            CREATE TABLE documents (
            id            INTEGER PRIMARY KEY,
            source_id     INTEGER NOT NULL REFERENCES sources(id) ON DELETE CASCADE,
            rel_path      TEXT NOT NULL,           -- relative to source root ('' for a file source)
            size          INTEGER NOT NULL,
            mtime_ns      INTEGER NOT NULL,
            content_hash  TEXT NOT NULL,
            format        TEXT NOT NULL,
            status        TEXT NOT NULL DEFAULT 'pending',  -- pending|chunked|summarized|embedded|error
            summary       TEXT,
            summary_model TEXT,
            indexed_at    INTEGER,
            error         TEXT,
            UNIQUE(source_id, rel_path)
            );

            CREATE TABLE chunks (
            id           INTEGER PRIMARY KEY,       -- TODO: do we need autoincrement?
            document_id  INTEGER NOT NULL REFERENCES documents(id) ON DELETE CASCADE,
            ordinal      INTEGER NOT NULL,
            text         TEXT NOT NULL,
            heading_path TEXT NOT NULL DEFAULT '',
            start_line   INTEGER,
            end_line     INTEGER,
            token_count  INTEGER NOT NULL,
            text_hash    TEXT NOT NULL,
            title        TEXT NOT NULL DEFAULT ''
            );
            CREATE INDEX chunks_doc  ON chunks(document_id, ordinal);   -- FTS indices
            CREATE INDEX chunks_hash ON chunks(text_hash);

            CREATE VIRTUAL TABLE chunks_fts USING fts5(
            text, heading_path, content='chunks', content_rowid='id',
            tokenize = 'porter unicode61 remove_diacritics 2'
            );

            CREATE TRIGGER chunks_ai AFTER INSERT ON chunks BEGIN
            INSERT INTO chunks_fts(rowid, text, heading_path)
            VALUES (new.id, new.text, new.heading_path);
            END;
            CREATE TRIGGER chunks_ad AFTER DELETE ON chunks BEGIN
            INSERT INTO chunks_fts(chunks_fts, rowid, text, heading_path)
            VALUES ('delete', old.id, old.text, old.heading_path);
            END;

            CREATE TABLE embeddings (
            text_hash TEXT NOT NULL,
            model     TEXT NOT NULL,
            dim       INTEGER NOT NULL,
            vector    BLOB NOT NULL,
            PRIMARY KEY (text_hash, model)
            );
        )SQL"
    };

    // number of migrations thus far
    constexpr int NUM_MIGRATIONS = std::size(MIGRATIONS);
}


// construct Database wrapper
Database::Database(const std::filesystem::path& file) {
    std::filesystem::create_directories(file.parent_path());

    // try to open connection
    if (sqlite3_open(file.string().c_str(), &db_) != SQLITE_OK) {
        std::string msg = sqlite3_errmsg(db_);
        sqlite3_close(db_);
        throw std::runtime_error("cannot open db: " + msg);
    }

    // set PRRAGMAs, run migration
    sqlite3_busy_timeout(db_, 5000);        // wait if locked, don't fail
    exec("PRAGMA journal_mode = WAL");
    exec("PRAGMA synchronous = NORMAL");
    exec("PRAGMA foreign_keys = ON");
    migrate();                                 // apply any migrations needed
}


// close on destruct if open
Database::~Database() { if (db_) sqlite3_close(db_); }


// execute sql wrapper
void Database::exec(const std::string& sql) {
    char* err = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
        std::string msg = err ? err : "unknown error";
        sqlite3_free(err);
        throw std::runtime_error("sqlite: " + msg);
    }
}


// apply migrations
void Database::migrate() {
    auto current_version = [&] {
        sqlite3_stmt* st = nullptr;
        sqlite3_prepare_v2(db_, "PRAGMA user_version", -1, &st, nullptr);
        sqlite3_step(st);
        int v = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        return v;
    };

    exec("BEGIN IMMEDIATE");     // grab write lock
    try {
        for (int v = current_version(); v < NUM_MIGRATIONS; ++v) {
            exec(MIGRATIONS[v]);
            exec("PRAGMA user_version = " + std::to_string(v + 1));
        }
        exec("COMMIT");
    } catch (...) {
        exec("ROLLBACK");
        throw;
    }
}

#pragma once
#include <sqlite3.h>
#include <filesystem>
#include <string>


// sqlite database connection wrapper
class Database {
    public:
        explicit Database(const std::filesystem::path& file);
        ~Database();
        Database(const Database&) = delete;
        Database& operator=(const Database&) = delete;

        sqlite3* handle() const { return db_; }
        void exec(const std::string& sql);

    private:
        void migrate();
        sqlite3* db_ = nullptr;
};


// transaction wrapper, attempt rollback on failure
class Transaction {
    public:
        explicit Transaction(Database& db) : db_(db) { db_.exec("BEGIN IMMEDIATE"); }
        ~Transaction() { if (!done_) try { db_.exec("ROLLBACK"); } catch (...) {} }
        void commit() { db_.exec("COMMIT"); done_ = true; }
        Transaction(const Transaction&) = delete;
        Transaction& operator=(const Transaction&) = delete;
    private:
        Database& db_;
        bool done_ = false;
};

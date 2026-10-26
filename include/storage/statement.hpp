#pragma once
#include <sqlite3.h>
#include <cstdint>
#include <string>


// SQL statement wrapper
class Statement {
    public:
        Statement(sqlite3* db, const char* sql);
        ~Statement();
        Statement(const Statement&) = delete;
        Statement& operator=(const Statement&) = delete;

        Statement& bind_int(int idx, int64_t v);                // 1-based
        Statement& bind_text(int idx, const std::string& v);

        bool step();      // true = row available, false = done
        void run();       // step to completion, then reset (for INSERT/UPDATE/DELETE)
        void reset();     // rewind so it can be re-executed with new bindings

        int64_t     col_int(int idx) const;                     // 0-based
        std::string col_text(int idx) const;

    private:
        sqlite3* db_;
        sqlite3_stmt* st_ = nullptr;

};

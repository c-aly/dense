#include "storage/statement.hpp"

#include <sqlite3.h>
#include <stdexcept>


// construct statement
Statement::Statement(sqlite3* db, const char* sql) : db_(db) {
    if (sqlite3_prepare_v2(db_, sql, -1, &st_, nullptr) != SQLITE_OK) {
        throw std::runtime_error(
            std::runtime_error(std::string("stmt prep failed: ") + sqlite3_errmsg(db_)));
    }
}

Statement::~Statement() { sqlite3_finalize(st_); }


// bind int or text (compile statement upfront, swap in binding)
Statement& Statement::bind_int(int idx, int64_t v) {
    sqlite3_bind_int64(st_, idx, v);
    return *this;
}

Statement& Statement::bind_text(int idx, const std::string& v) {
    sqlite3_bind_text(st_, idx, v.c_str(), static_cast<int>(v.size()), SQLITE_TRANSIENT);
    return *this;
}


// step to next row
bool Statement::step() {
    int rc = sqlite3_step(st_);
    if (rc == SQLITE_ROW) return true;
    if (rc == SQLITE_DONE) return false;
    throw std::runtime_error(
        std::string("step failed: ") + sqlite3_errmsg(db_));
}

// convenience
void Statement::run() { try { step(); } catch (...) { reset(); } reset(); }

void Statement::reset() { sqlite3_reset(st_); }

int64_t Statement::col_int(int idx) const {
    return sqlite3_column_int64(st_, idx); 
}

std::string Statement::col_text(int idx) const {
    const unsigned char* p = sqlite3_column_text(st_, idx);
    return p ? reinterpret_cast<const char*>(p) : "";
}
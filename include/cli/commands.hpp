#pragma once

#include "storage/database.hpp"

#include <string>
#include <filesystem>


namespace dense::cli {
    void handle_add(Database& db, const std::string& path);
    void handle_remove(Database& db, const std::string& path);
    void handle_list(Database& db);
    void handle_run(int port);
}

#include "cli/commands.hpp"
#include "core/scanner.hpp"
#include "core/sources.hpp"
#include "storage/database.hpp"

#include <iostream>
#include <string>


namespace dense::cli {

    void handle_add(Database& db, const std::string& path) {
        try {
            ScanStats stats = add_source(db, path);
            std::cout << "Added source: " << path << "\n";
            std::cout << stats.to_string() << "\n";
        } catch (const std::exception& e) {
            std::cerr << "dense add: " << e.what() << "\n";
        }
    }

    void handle_remove(Database& db, const std::string& path) {
        try {
            RemoveStats stats = remove_source(db, path);
            std::cout << "Removed source: " << path << "\n";
            std::cout << stats.to_string() << "\n";
        } catch (const std::exception& e) {
            std::cerr << "dense remove: " << e.what() << "\n";
        }
    }

    void handle_list(Database& db) {
        try {
            auto sources = list_sources(db);
            if (sources.empty()) { std::cout << "No sources tracked.\n"; return; }
            for (const auto& s : sources) std::cout << s.to_string() << "\n";
        } catch (const std::exception& e) {
            std::cerr << "dense list: " << e.what() << "\n";
        }
    }

    void handle_run(int port) {
        // placeholder — local MCP server is a future feature
        std::cout << "Starting dense MCP on port " << port << '\n';
    }

}

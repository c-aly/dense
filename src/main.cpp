#include "CLI/CLI.hpp"
#include "cli/commands.hpp"
#include "storage/database.hpp"
#include "config.hpp"

#include <CLI/CLI.hpp>
#include <string>


int main(int argc, char** argv) {

    Database db(Config::db_path());

    CLI::App app("dense: local file tracker for dense info servers");
    app.require_subcommand(1);


    // dense add <path>
    std::filesystem::path add_path = ".";
    auto* add_cmd = app.add_subcommand("add", "Add directory to dense");
    add_cmd->add_option("path", add_path, "Target directory")->check(CLI::ExistingDirectory);

    // dense remove <path>
    std::filesystem::path rem_path = ".";
    auto* rem_cmd = app.add_subcommand("remove", "Remove source directory from dense");
    rem_cmd->add_option("path", rem_path, "Target directory")->check(CLI::ExistingDirectory);

    // dense list
    auto* list_cmd = app.add_subcommand("list", "List all tracked source directories");

    // dense run
    int mcp_port = 8080;
    auto* run_cmd = app.add_subcommand("run", "Start local MCP server");
    run_cmd->add_option("-p,--port", mcp_port, "Port");


    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    if      (add_cmd->parsed())  dense::cli::handle_add(db, add_path);
    else if (rem_cmd->parsed())  dense::cli::handle_remove(db, rem_path);
    else if (list_cmd->parsed()) dense::cli::handle_list(db);
    else if (run_cmd->parsed())  dense::cli::handle_run(mcp_port);

    return 0;
}

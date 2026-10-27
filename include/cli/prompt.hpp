#pragma once
#include <string>

namespace dense::cli {

// Print `label` and read a line from stdin.
std::string prompt(const std::string& label);

// Same, but without echoing the input (passwords). Falls back to a plain read
// when stdin isn't a terminal (e.g. piped).
std::string prompt_secret(const std::string& label);

} // namespace dense::cli

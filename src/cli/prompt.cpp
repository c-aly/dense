#include "cli/prompt.hpp"

#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace dense::cli {

std::string prompt(const std::string& label) {
    std::cout << label << std::flush;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

std::string prompt_secret(const std::string& label) {
    std::cout << label << std::flush;
    std::string line;

#ifdef _WIN32
    HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    const bool tty = GetConsoleMode(in, &mode) != 0;
    if (tty) SetConsoleMode(in, mode & ~ENABLE_ECHO_INPUT);
    std::getline(std::cin, line);
    if (tty) { SetConsoleMode(in, mode); std::cout << "\n"; }
#else
    termios old{};
    const bool tty = isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &old) == 0;
    if (tty) {
        termios quiet = old;
        quiet.c_lflag &= ~static_cast<tcflag_t>(ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &quiet);
    }
    std::getline(std::cin, line);
    if (tty) { tcsetattr(STDIN_FILENO, TCSANOW, &old); std::cout << "\n"; }
#endif

    return line;
}

} // namespace dense::cli

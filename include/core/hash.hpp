// include/core/hash.hpp
#pragma once
#include <filesystem>
#include <string>
#include <string_view>


std::string hash_bytes(std::string_view data);              // 32-char hex
std::string hash_file(const std::filesystem::path& file);   // streamed, constant memory

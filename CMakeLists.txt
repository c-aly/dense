cmake_minimum_required(VERSION 3.20)
project(dense VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(CLI11       CONFIG REQUIRED)
find_package(xxHash      CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(httplib     CONFIG REQUIRED)

# cmark-gfm and sqlite3 removed — parsing/storage moved to server-side
# collect all sources automatically — move a file out of src/ to exclude it
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(dense ${SOURCES})

target_include_directories(dense PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} include)

target_link_libraries(dense PRIVATE
    CLI11::CLI11
    xxHash::xxhash
    nlohmann_json::nlohmann_json
    httplib::httplib
)

cmake_minimum_required(VERSION 3.20)
project(dense VERSION 0.1.0 LANGUAGES CXX)

# Ensure modern C++23 features are enabled
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find our package dependencies
find_package(SQLite3 REQUIRED)
# find_package(nlohmann_json REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(xxHash CONFIG REQUIRED)
# cmark-gfm removed — markdown parser moved to server-side (misc/)

# Collect all source files automatically
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Define our executable binary name
add_executable(dense ${SOURCES})

# Include header paths
target_include_directories(dense PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} include)

# Link external libraries to our binary
target_link_libraries(dense PRIVATE 
    SQLite3
    # nlohmann_json::nlohmann_json
    CLI11::CLI11
    xxHash::xxhash
)

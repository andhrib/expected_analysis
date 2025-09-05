cmake_minimum_required(VERSION 3.10)
project(ExceptionHandlingProject VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(BUILD_TESTING "Build tests" ON)

include_directories(include)

# --- Main Executable ---
find_package(Threads REQUIRED)
find_package(benchmark REQUIRED PATHS "../benchmark-main/build")

add_executable(app "src/main.cpp"                
                   "src/config.cpp"
                   "src/connection.cpp"
                   "src/query.cpp"
                   "src/server.cpp"
)
target_link_libraries(app PRIVATE Threads::Threads  benchmark::benchmark benchmark::benchmark_main)

# This command will run at build time to copy files.
add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/configs $<TARGET_FILE_DIR:app>/configs
    COMMENT "Copying config files to $<TARGET_FILE_DIR:app>/configs"
)
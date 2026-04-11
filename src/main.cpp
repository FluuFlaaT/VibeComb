#include <CLI/CLI.hpp>

int main(int argc, char** argv) {
    CLI::App app{"VibeComb", "vibecomb"};

    std::string data_dir;
    app.add_option("-d,--data-dir", data_dir, "Data directory");

    bool verbose = false;
    app.add_flag("-v,--verbose", verbose, "Verbose output");

    CLI11_PARSE(app, argc, argv);

    return 0;
}

#ifndef FS_HPP
#define FS_HPP
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
static std::string read_file(
    const std::string& filename
)
{
    std::ifstream f(filename);
    if (!f.is_open()) {
        throw std::string(
            "error reading file: "
        ) + filename;
    }
    std::ostringstream stream;
    stream << f.rdbuf();
    return stream.str();
}
static std::string normalize_path(
    const std::string& filename
)
{
    return std::filesystem::path(filename)
        .lexically_normal()
        .string();
}
static std::string module_path(
    const std::string& module,
    const std::string& importer
)
{
    std::filesystem::path importer_path(
        importer
    );
    std::filesystem::path result =
        importer_path.parent_path()
        / (module + ".hc");
    return normalize_path(
        result.string()
    );
}
#endif
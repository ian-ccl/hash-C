#ifndef TYPES_HPP
#define TYPES_HPP

#include "config.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <memory>
#include <utility>
#include <variant>
#include <cstdio>
#include <fstream>
#include <functional>
class tree {
    public:
        struct node {
            std::string value;
            std::string kind;

            node* parent = nullptr;
            std::vector<std::unique_ptr<node>> children;

            node(
                const std::string& value,
                node* parent = nullptr,
                const std::string& kind = "identifier"
            )
                : value(value),
                kind(kind),
                parent(parent)
            {}

            node(
                std::string&& value,
                node* parent = nullptr,
                const std::string& kind = "identifier"
            )
                : value(std::move(value)),
                kind(kind),
                parent(parent)
            {}
        };

    private:
        std::unique_ptr<node> root_;

    public:
        tree() = default;

        explicit tree(std::string value)
            : root_(std::make_unique<node>(std::move(value))) {}

        tree(const tree&) = delete;
        tree& operator=(const tree&) = delete;

        tree(tree&&) noexcept = default;
        tree& operator=(tree&&) noexcept = default;

        node* root() noexcept {
            return root_.get();
        }

        const node* root() const noexcept {
            return root_.get();
        }

        template<class... Args>
        node& emplace_root(Args&&... args) {
            root_ = std::make_unique<node>(
                std::string(std::forward<Args>(args)...)
            );

            return *root_;
        }

        bool empty() const noexcept {
            return !root_;
        }
};

struct ArgsInfo {
    ArgsInfo(const ArgsInfo&) = default;
    ArgsInfo(ArgsInfo&&) = default;
    ArgsInfo& operator=(const ArgsInfo&) = default;
    ArgsInfo& operator=(ArgsInfo&&) = default;

    struct Out {
        enum As {
            Cpp,
            Object,
            Executable
        };

        As as = As::Executable;
        std::string name = OnWin ? "a.exe" : "a.out";

        Out(As out_as = As::Executable, std::string name = OnWin ? "a.exe" : "a.out") : as(out_as), name(name) {}
    };

    ArgsInfo(
        std::string main = "main.hc",
        Out output = Out(),
        std::string cpp_compiler = "g++",
        std::vector<std::string> cpp_flags = {}
    )
        : main_file(main), out(output), ccpp(cpp_compiler), cpp_flags(cpp_flags) {}

    std::string main_file;
    Out out;
    std::string ccpp = "g++";
    std::vector<std::string> cpp_flags;
};


struct args_t {
    char** data;
    int len;

    args_t(char** argv, int argc)
        : data(argv), len(argc) {}

    args_t(const args_t&) = default;
    args_t(args_t&&) = default;

    char* operator[](int idx) {
        return data[idx];
    }

    args_t& operator=(const args_t&) = default;
    args_t& operator=(args_t&&) = default;
};


struct Module {
    Module(
        std::string api_defs = "",
        std::string file_code = "",
        std::string file_name = ""
    )
        : api(api_defs), code(file_code), filename(file_name) {}

    std::string api;

    std::string code;

    std::string filename;
};


struct program_t {
    program_t(
        std::unordered_map<std::string, Module> modules,
        ArgsInfo& arguments,
        std::vector<std::string> functions,
        std::unordered_set<std::string> Loading
    )
        : Modules(modules), args(arguments), funcs(functions),
          loading(Loading) {}

    std::unordered_map<std::string, Module> Modules;

    ArgsInfo& args;

    std::vector<std::string> funcs;

    std::unordered_set<std::string> loading;
};


struct Token {
    std::string kind;
    std::string val;
    std::vector<std::string> more_info;

    Token(
        std::string kind = "",
        std::string value = "",
        std::vector<std::string> more_information = {}
    )
        : kind(std::move(kind)),
          val(std::move(value)),
          more_info(std::move(more_information)) {}

    Token(Token&&) = default;
    Token(const Token&) = default;
    Token& operator=(Token&&) = default;
    Token& operator=(const Token&) = default;
};


struct ParsedToken {

    std::variant<tree, std::string> val;

    std::vector<std::string> more_info;

    ParsedToken(
        tree&& value,
        std::vector<std::string> more_information = {}
    )
        : val(std::move(value)),
          more_info(std::move(more_information)) {}

    ParsedToken(
        std::string value,
        std::vector<std::string> more_information = {}
    )
        : val(value),
          more_info(std::move(more_information)) {}

    ParsedToken(
        const Token &t
    )
        : val(t.val),
          more_info(t.more_info) {}

    ParsedToken(ParsedToken&&) = default;
    ParsedToken(const ParsedToken&) = delete;
    ParsedToken& operator=(ParsedToken&&) = default;
    ParsedToken& operator=(const ParsedToken&) = delete;

    bool are_tree() const {
        return std::holds_alternative<tree>(val);
    }

    bool are_token() const {
        return std::holds_alternative<std::string>(val);
    }

    std::string& gets() {
        return std::get<std::string>(val);
    }

    const std::string& gets() const {
        return std::get<std::string>(val);
    }

    tree& gett() {
        return std::get<tree>(val);
    }

    const tree& gett() const {
        return std::get<tree>(val);
    }
};

struct ProcessResult {
    std::string stdout;
    std::string stderr;
    int exit_code;
};
#ifdef _WIN32
#define popen  _popen
#define pclose _pclose
#endif
static inline ProcessResult shell(std::string cmd) {
    const std::string err_file = ".__stderr.tmp";

    std::string command =
        cmd + " 2> " + err_file;

    FILE* pipe = popen(command.c_str(), "r");

    if (!pipe)
        return {"", "failed to open process", -1};

    std::string out;
    char buffer[4096];

    while (fgets(buffer, sizeof(buffer), pipe))
        out += buffer;

    int exit_code = pclose(pipe);

    std::string err;
    std::ifstream file(err_file, std::ios::binary);

    if (file) {
        err.assign(
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()
        );
    }

    std::remove(err_file.c_str());

    return {out, err, exit_code};
}


struct TemplateInfo {
    size_t args;
    std::string name;
    std::string code;
    bool is_strsafe : 1 {false};
    bool is_type_only : 1 {false};
};

using Templates = std::unordered_map<std::string, TemplateInfo>;

enum class todo_id {
    OTHER_ID = 0,
    TYPE_ID = 1,
};

struct todo {
    std::vector<std::string> args = {};
    std::function<bool(const std::vector<std::string>&)> func;
    todo_id id = todo_id::OTHER_ID;

    bool operator()() const {
        return func(args);
    }
};

#endif

#ifndef TYPES_HPP
#define TYPES_HPP

#include "../libs/transcode/transcode.hpp"
#include <memory>
#include <unordered_set>
#include <functional>
#include <vector>
#include <fstream>
#include <utility>
namespace c {
    // ============================================================
    // #C TYPES
    // ============================================================
    
    struct TemplateInfo {
        size_t args{0};
        std::string name;
        std::string code;
        bool is_strsafe : 1 {false};
        bool is_type_only : 1 {false};
        inline bool operator==(const TemplateInfo& other) const { return name == other.name; }
        inline bool operator==(std::string_view text) const { return name == text; }
    };
}
template <>
struct std::hash<c::TemplateInfo> {
    using is_transparent = void; // <-- El truco mágico para poder buscar con string_view
    
    size_t operator()(const c::TemplateInfo& obj) const { 
        return std::hash<std::string>{}(obj.name); 
    }
    size_t operator()(std::string_view txt) const { 
        return std::hash<std::string_view>{}(txt); 
    }
};

namespace c {

    struct TemplateInfoEqual {
        using is_transparent = void;

        bool operator()(
            const TemplateInfo& a,
            const TemplateInfo& b
        ) const {
            return a.name == b.name;
        }

        bool operator()(
            const TemplateInfo& a,
            std::string_view b
        ) const {
            return a.name == b;
        }

        bool operator()(
            std::string_view a,
            const TemplateInfo& b
        ) const {
            return a == b.name;
        }
    };

    using Templates = tc::Set<TemplateInfo, std::hash<TemplateInfo>, TemplateInfoEqual>;

    enum class todo_id {
        OTHER_ID = 0,
        TYPE_ID = 1,
    };

    struct todo {
        std::vector<std::string> args = {};
        std::function<bool(const std::vector<std::string>&)> func = [](const std::vector<std::string>&) {return false;};
        todo_id id = todo_id::OTHER_ID;

        bool operator()() const {
            return func(args);
        }
    };

    inline std::vector<todo>& todos() {
        static std::vector<todo> todos;
        return todos;
    }

    // ------------------------------------------------------------
    // Tokens
    // ------------------------------------------------------------

    enum class TokenKind {
        End,

        Identifier,
        Integer,
        Float,
        String,
        Character,

        Fn,
        Struct,
        CStruct,
        Typedef,
        Enum,

        If,
        Else,
        Elif,
        While,
        Do,
        For,
        Switch,
        Case,
        Default,
        Defer,

        Break,
        Continue,
        Return,

        True,
        False,
        Nullptr,

        Plus,
        Minus,
        Star,
        Slash,
        Percent,

        PlusPlus,
        MinusMinus,

        Equal,
        PlusEqual,
        MinusEqual,
        StarEqual,
        SlashEqual,
        PercentEqual,

        EqualEqual,
        NotEqual,

        Less,
        LessEqual,
        Greater,
        GreaterEqual,

        LogicalAnd,
        LogicalOr,

        BitAnd,
        BitOr,
        BitXor,
        BitNot,
        LogicalNot,

        Arrow,

        Question,
        Colon,

        Dot,
        Comma,
        Semicolon,

        LParen,
        RParen,
        LBrace,
        RBrace,
        LBracket,
        RBracket,

        Cpp,

        Make
    };

    struct Token {
        TokenKind kind;
        tc::String text;
        size_t position = 0;

        tc::Position source_position(const tc::Source& source) const noexcept {
            return source.position(position);
        }
    };

    // ------------------------------------------------------------
    // Types
    // ------------------------------------------------------------

    enum class TypeKind {
        Int,
        UInt,
        Float,
        Bool,
        Char,
        Void,

        Named,

        Pointer,
        Slice,
        Array,

        Const
    };

    struct Type {
        TypeKind kind = TypeKind::Void;

        tc::String name;

        std::shared_ptr<Type> base;

        size_t array_size = 0;

        static Type primitive(TypeKind kind) {
            Type result;
            result.kind = kind;
            return result;
        }

        static Type named(tc::String name) {
            Type result;
            result.kind = TypeKind::Named;
            result.name = std::move(name);
            return result;
        }

        static Type pointer(Type base) {
            Type result;
            result.kind = TypeKind::Pointer;
            result.base = std::make_unique<Type>(std::move(base));
            return result;
        }

        static Type slice(Type base) {
            Type result;
            result.kind = TypeKind::Slice;
            result.base = std::make_unique<Type>(std::move(base));
            return result;
        }

        static Type array(Type base, size_t size) {
            Type result;
            result.kind = TypeKind::Array;
            result.base = std::make_unique<Type>(std::move(base));
            result.array_size = size;
            return result;
        }

        static Type constant(Type base) {
            Type result;
            result.kind = TypeKind::Const;
            result.base = std::make_unique<Type>(std::move(base));
            return result;
        }
    };

    // ------------------------------------------------------------
    // AST
    // ------------------------------------------------------------

    struct Node {
        virtual ~Node() = default;
    };

    struct Expr : Node {
    };

    struct Stmt : Node {
    };

    struct Decl : Node {
        tc::String name;

        explicit Decl(tc::String name)
            : name(std::move(name)) {
        }
    };


    // ------------------------------------------------------------
    // Expressions
    // ------------------------------------------------------------

    struct LiteralExpr : Expr {
        tc::String value;

        explicit LiteralExpr(tc::String value)
            : value(std::move(value)) {
        }
    };
    struct MakeExpr : Expr {
        c::Type type;
        std::vector<std::unique_ptr<Expr>> args;

        explicit MakeExpr(c::Type type, std::vector<std::unique_ptr<Expr>> args = {})
            : type(type), args(std::move(args)) {
        }
    };

    struct NameExpr : Expr {
        tc::String name;

        explicit NameExpr(tc::String name)
            : name(std::move(name)) {
        }
    };

    struct UnaryExpr : Expr {
        TokenKind op;
        std::unique_ptr<Expr> operand;

        UnaryExpr(
            TokenKind op,
            std::unique_ptr<Expr> operand
        )
            : op(op),
            operand(std::move(operand)) {
        }
    };

    struct BinaryExpr : Expr {
        TokenKind op;

        std::unique_ptr<Expr> left;
        std::unique_ptr<Expr> right;

        BinaryExpr(
            std::unique_ptr<Expr> left,
            TokenKind op,
            std::unique_ptr<Expr> right
        )
            : op(op),
            left(std::move(left)),
            right(std::move(right)) {
        }
    };

    struct CallExpr : Expr {
        std::unique_ptr<Expr> callee;
        std::vector<std::unique_ptr<Expr>> args;

        explicit CallExpr(std::unique_ptr<Expr> callee)
            : callee(std::move(callee)) {
        }
    };

    struct IndexExpr : Expr {
        std::unique_ptr<Expr> object;
        std::unique_ptr<Expr> index;

        IndexExpr(
            std::unique_ptr<Expr> object,
            std::unique_ptr<Expr> index
        )
            : object(std::move(object)),
            index(std::move(index)) {
        }
    };

    struct MemberExpr : Expr {
        std::unique_ptr<Expr> object;
        tc::String member;
        bool pointer = false;

        MemberExpr(
            std::unique_ptr<Expr> object,
            tc::String member,
            bool pointer
        )
            : object(std::move(object)),
            member(std::move(member)),
            pointer(pointer) {
        }
    };

    struct ConditionalExpr : Expr {
        std::unique_ptr<Expr> condition;
        std::unique_ptr<Expr> when_true;
        std::unique_ptr<Expr> when_false;

        ConditionalExpr(
            std::unique_ptr<Expr> condition,
            std::unique_ptr<Expr> when_true,
            std::unique_ptr<Expr> when_false
        )
            : condition(std::move(condition)),
            when_true(std::move(when_true)),
            when_false(std::move(when_false)) {
        }
    };

    // ------------------------------------------------------------
    // Statements
    // ------------------------------------------------------------

    struct BlockStmt : Stmt {
        std::vector<std::unique_ptr<Stmt>> statements;
    };

    struct ExprStmt : Stmt {
        std::unique_ptr<Expr> expression;

        explicit ExprStmt(std::unique_ptr<Expr> expression)
            : expression(std::move(expression)) {
        }
    };

    struct ReturnStmt : Stmt {
        std::unique_ptr<Expr> value;

        explicit ReturnStmt(std::unique_ptr<Expr> value)
            : value(std::move(value)) {
        }
    };

    struct BreakStmt : Stmt {
    };

    struct ContinueStmt : Stmt {
    };

    struct VariableDeclStmt : Stmt {
        Type type;
        tc::String name;
        std::unique_ptr<Expr> initializer;

        VariableDeclStmt(
            Type type,
            tc::String name,
            std::unique_ptr<Expr> initializer
        )
            : type(std::move(type)),
            name(std::move(name)),
            initializer(std::move(initializer)) {
        }
    };

    struct IfStmt : Stmt {
        std::unique_ptr<Expr> condition;

        std::unique_ptr<Stmt> then_branch;
        std::unique_ptr<Stmt> else_branch;
    };

    struct WhileStmt : Stmt {
        std::unique_ptr<Expr> condition;
        std::unique_ptr<Stmt> body;
    };

    struct DoStmt : Stmt {
        std::unique_ptr<Stmt> body;
        std::unique_ptr<Expr> condition;
    };

    struct ForStmt : Stmt {
        std::unique_ptr<Stmt> initialization;
        std::unique_ptr<Expr> condition;
        std::unique_ptr<Expr> increment;

        std::unique_ptr<Stmt> body;
    };

    struct SwitchCase {
        std::unique_ptr<Expr> value;
        std::vector<std::unique_ptr<Stmt>> body;
    };

    struct SwitchStmt : Stmt {
        std::unique_ptr<Expr> value;

        std::vector<SwitchCase> cases;
        std::vector<std::unique_ptr<Stmt>> default_body;
    };

    // ------------------------------------------------------------
    // Declarations
    // ------------------------------------------------------------

    struct Variable {
        Type type;
        tc::String name;
        std::unique_ptr<Expr> initializer;
    };

    struct FunctionDecl : Decl {
        std::vector<Variable> parameters;
        Type return_type;
        std::unique_ptr<BlockStmt> body;

        FunctionDecl(
            tc::String name,
            Type return_type
        )
            : Decl(std::move(name)),
            return_type(std::move(return_type)) {
        }
    };

    struct StructDecl : Decl {
        bool copyable;

        std::vector<Variable> fields;

        StructDecl(
            tc::String name,
            bool copyable
        )
            : Decl(std::move(name)),
            copyable(copyable) {
        }
    };

    struct TypedefDecl : Decl {
        Type type;

        TypedefDecl(
            tc::String name,
            Type type
        )
            : Decl(std::move(name)),
            type(std::move(type)) {
        }
    };

    struct EnumDecl : Decl {
        std::vector<tc::String> values;

        explicit EnumDecl(tc::String name)
            : Decl(std::move(name)) {
        }
    };

    struct DeferStmt : Stmt {
        std::unique_ptr<BlockStmt> body;
        explicit DeferStmt(std::unique_ptr<BlockStmt> bdy) : Stmt(), body(std::move(bdy)) {}
    };

    struct Cpp : Decl {
        tc::String& code = this->name;
        explicit Cpp(tc::String Code)
            : Decl(std::move(Code)), code(name) {
        }
    };

    struct ModuleSource {
        std::string api;
        std::string implementation;
    };

    struct Module {
        std::string name;
        std::string api;
        std::string implementation;
        std::string filename;
    };

    struct LoadedModule {
        std::string name;
        std::string api;
        std::vector<std::string> imports;
        std::string filename;
        std::string code;
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
}

#endif // TYPES_HPP
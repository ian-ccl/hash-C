#ifndef CODEGEN_HPP
#define CODEGEN_HPP
#include <sstream>
#include "../libs/transcode/transcode.hpp"
#include "types.hpp"
#include <vector>
#include <memory>
#include "mangle.hpp"
namespace c
{

    // ============================================================
    // CODE GENERATOR
    // ============================================================
    static inline bool first_file = true;

    class Codegen
    {
        std::ostringstream out;

    public:
        std::string generate(
            const std::vector<std::unique_ptr<Decl>> &declarations)
        {
            for (const auto &decl : declarations)
                declaration(*decl);

            tc::String code = out.str();
            code = "#include \"std.tmp.hpp\"\n" + code;
            if (first_file) {
                code += R"(
int main(int argc, char** argv) {
    hc::slice<char>* args = nullptr;
    try {
        args = new hc::slice<char>[argc];
        for (int i = 0; i < argc; ++i) {
            std::size_t len = 0;

            while (argv[i][len] != '\0')
                ++len;

            args[i] = {
                argv[i],
                len
            };
        }

        _I4main(
            hc::slice<hc::slice<char>>{
                args,
                static_cast<std::size_t>(argc)
            }
        );
    } catch(int x) {return x;}
    delete[] args;
    return 0;
}
)";
                first_file = false;
            }
            return code;
        }

    private:
        void line(const std::string &text = "")
        {
            out << text;
        }

        std::string type(const Type &_type)
        {
            switch (_type.kind)
            {
            case TypeKind::Int:
                return "std::ptrdiff_t";

            case TypeKind::UInt:
                return "std::size_t";

            case TypeKind::Float:
                return "double";

            case TypeKind::Bool:
                return "bool";

            case TypeKind::Char:
                return "char";

            case TypeKind::Void:
                return "void";

            case TypeKind::Named:
                if (_type.name == "int")
                    return "hc::ptrdiff_t";

                if (_type.name == "uint")
                    return "hc::size_t";

                if (_type.name == "float")
                    return "double";

                if (_type.name == "bool")
                    return "bool";

                if (_type.name == "char")
                    return "char";

                if (_type.name == "void")
                    return "void";

                if (_type.name == "ubyte")
                    return "hc::ubyte";
                if (_type.name == "ibyte")
                    return "hc::ibyte";
                if (auto name = Mangler{}.mangle(_type.name))
                {
                    return *name;
                }
                return "";

            case TypeKind::Pointer:
                return "hc::ptr<" + (type(_type.base ? *(_type.base) : Type{})) + ">";

            case TypeKind::Slice:
                return "hc::slice<" +
                       type(*(_type.base)) +
                       ">";

            case TypeKind::Array:
                return "hc::array<" +
                       type(*(_type.base)) +
                       ", " +
                       std::to_string(_type.array_size) +
                       ">";
            case TypeKind::Const:
                return "hc::constant<" + type(*(_type.base)) + ">";
            }

            return "void";
        }

        void declaration(const Decl &decl)
        {
            if (auto *fn =
                    dynamic_cast<const FunctionDecl *>(&decl))
            {
                function(*fn);
                return;
            }

            if (auto *st =
                    dynamic_cast<const StructDecl *>(&decl))
            {
                structure(*st);
                return;
            }

            if (auto *td =
                    dynamic_cast<const TypedefDecl *>(&decl))
            {
                typedef_decl(*td);
                return;
            }

            if (auto *en =
                    dynamic_cast<const EnumDecl *>(&decl))
            {
                enumeration(*en);
                return;
            }

            if (auto *cppdecl = dynamic_cast<const Cpp *>(&decl))
            {
                cpp(*cppdecl);
                return;
            }
        }

        void function(const FunctionDecl &fn)
        {
            Mangler mangler;

            tc::Opt<tc::String> name =
                mangler.mangle(fn.name);

            if (!name)
                return;

            out << "extern \"C\" auto "
                << (*name == "main" ? "_I4main" : *name)
                << "(";

            for (size_t i = 0; i < fn.parameters.size(); ++i)
            {
                if (i)
                    out << ", ";

                Mangler mangler;
                tc::Opt<tc::String> name = mangler.mangle(fn.parameters[i].name);

                if (!name)
                {
                    name = "";
                }

                out << type(fn.parameters[i].type)
                    << " "
                    << *name;
            }

            out << ") -> "
                << type(fn.return_type);

            if (fn.body == nullptr)
            {
                line(";");
                return;
            }

            out << " {\n";

            for (const auto &statement :
                 fn.body->statements)
            {
                statement_node(*statement);
            }

            line("}");
        }

        void structure(const StructDecl &st)
        {
            Mangler mangler;
            tc::String stname;
            
            if (auto name = mangler.mangle(st.name))
                line(
                    "struct " +
                    (stname = *name)+
                    (st.copyable ? " {" : " : hc::NonCopyable {"));
            else
                line(
                    "struct {");

            for (const Variable &field : st.fields)
            {
                line(
                    type(field.type) +
                    " " +
                    field.name +
                    ";");
            }
            out << stname << '(';
            const c::Variable* back = &st.fields.back();
            for (const Variable &field : st.fields)
            {
                out << type(field.type) << " " << field.name;
                if (&field != back) out << ',';
            }
            out << ") : ";
            for (const Variable &field : st.fields)
            {
                out << field.name << '(' << field.name << ')';
                if (&field != back) out << ',';
            }
            out << "{}";
            out << stname << "() = default;";
            line("};");
        }

        void typedef_decl(const TypedefDecl &decl)
        {
            Mangler mangler;
            if (auto name = mangler.mangle(decl.name))

                line(
                    "using " +
                    *name +
                    " = " +
                    type(decl.type) +
                    ";");
            else
                line(
                    "using = " + type(decl.type) + ";");
        }

        void enumeration(const EnumDecl &decl)
        {
            Mangler mangler;
            if (auto name = mangler.mangle(decl.name))

                line(
                    "enum " +
                    *name +
                    " {");
            else
                line(
                    "enum {");

            for (size_t i = 0; i < decl.values.size(); ++i)
            {
                tc::String text =
                    decl.values[i];

                if (i + 1 < decl.values.size())
                    text += ",";

                line(text);
            }

            line("};");
        }

        void defer(const DeferStmt &decl)
        {
            static size_t defer_no = 0;
            tc::String no = std::to_string(defer_no);
            line(
                "hc::Defer _I5defer" + std::to_string(no.size()) + no + "4impl6detail2hc = [&](){");

            for (const auto &statement :
                 decl.body->statements)
            {
                statement_node(*statement);
            }

            line("};");

            defer_no++;
        }

        void cpp(const Cpp &cpp)
        {
            out
                << cpp.code;
        }

        void statement_node(const Stmt &stmt)
        {
            if (auto *block =
                    dynamic_cast<const BlockStmt *>(&stmt))
            {
                line("{");

                for (const auto &child :
                     block->statements)
                {
                    statement_node(*child);
                }

                line("}");
                return;
            }

            if (auto *expr =
                    dynamic_cast<const ExprStmt *>(&stmt))
            {
                line(
                    expression(*expr->expression) +
                    ";");
                return;
            }

            if (auto *ret =
                    dynamic_cast<const ReturnStmt *>(&stmt))
            {
                if (ret->value)
                {
                    line(
                        "return " +
                        expression(*ret->value) +
                        ";");
                }
                else
                {
                    line("return;");
                }

                return;
            }

            if (auto *def = dynamic_cast<const DeferStmt *>(&stmt))
            {
                defer(*def);
                return;
            }

            if (dynamic_cast<const BreakStmt *>(&stmt))
            {
                line("break;");
                return;
            }

            if (dynamic_cast<const ContinueStmt *>(&stmt))
            {
                line("continue;");
                return;
            }

            if (auto *var =
                    dynamic_cast<const VariableDeclStmt *>(&stmt))
            {
                std::string result =
                    type(var->type) +
                    " " +
                    var->name;

                if (var->initializer)
                {
                    result +=
                        " = " +
                        expression(*var->initializer);
                }

                result += ";";

                line(result);
                return;
            }

            if (auto *ifs =
                    dynamic_cast<const IfStmt *>(&stmt))
            {
                line(
                    "if (" +
                    expression(*ifs->condition) +
                    ")");

                statement_node(*ifs->then_branch);

                if (ifs->else_branch)
                {
                    line("else");
                    statement_node(*ifs->else_branch);
                }

                return;
            }

            if (auto *wh =
                    dynamic_cast<const WhileStmt *>(&stmt))
            {
                line(
                    "while (" +
                    expression(*wh->condition) +
                    ")");

                statement_node(*wh->body);
                return;
            }

            if (auto *ds =
                    dynamic_cast<const DoStmt *>(&stmt))
            {
                line("do");
                statement_node(*ds->body);

                line(
                    "while (" +
                    expression(*ds->condition) +
                    ");");

                return;
            }

            if (auto *fs =
                    dynamic_cast<const ForStmt *>(&stmt))
            {
                line(
                    "for (" +
                    for_initialization(fs->initialization) +
                    "; " +
                    (fs->condition
                         ? expression(*fs->condition)
                         : "") +
                    "; " +
                    (fs->increment
                         ? expression(*fs->increment)
                         : "") +
                    ")");

                statement_node(*fs->body);
                return;
            }
        }

        std::string for_initialization(
            const std::unique_ptr<Stmt> &stmt)
        {
            if (!stmt)
                return "";

            if (auto *expr =
                    dynamic_cast<const ExprStmt *>(stmt.get()))
            {
                return expression(*expr->expression);
            }

            if (auto *var =
                    dynamic_cast<const VariableDeclStmt *>(
                        stmt.get()))
            {
                std::string result =
                    type(var->type) +
                    " " +
                    var->name;

                if (var->initializer)
                {
                    result +=
                        " = " +
                        expression(*var->initializer);
                }

                return result;
            }

            return "";
        }

        std::string expression(const Expr &expr)
        {
            if (auto *literal =
                    dynamic_cast<const LiteralExpr *>(&expr))
            {
                return literal->value;
            }

            if (auto *name =
                    dynamic_cast<const NameExpr *>(&expr))
            {
                Mangler mangler;
                tc::Opt<tc::String> mname = mangler.mangle(name->name);
                return mname ? *mname : "";
            }

            if (auto *unary =
                    dynamic_cast<const UnaryExpr *>(&expr))
            {
                return unary_operator(unary->op) +
                       expression(*unary->operand);
            }

            if (auto *binary =
                    dynamic_cast<const BinaryExpr *>(&expr))
            {
                return "(" +
                       expression(*binary->left) +
                       " " +
                       binary_operator(binary->op) +
                       " " +
                       expression(*binary->right) +
                       ")";
            }

            if (auto *call =
                    dynamic_cast<const CallExpr *>(&expr))
            {
                std::string result =
                    expression(*call->callee) +
                    "(";

                for (size_t i = 0; i < call->args.size(); ++i)
                {
                    if (i)
                        result += ", ";

                    result +=
                        expression(*call->args[i]);
                }

                result += ")";

                return result;
            }

            if (auto *index =
                    dynamic_cast<const IndexExpr *>(&expr))
            {
                return expression(*index->object) +
                       "[" +
                       expression(*index->index) +
                       "]";
            }

            if (auto *member =
                    dynamic_cast<const MemberExpr *>(&expr))
            {
                return expression(*member->object) +
                       (member->pointer
                            ? "->"
                            : ".") +
                       member->member;
            }

            if (auto *conditional =
                    dynamic_cast<const ConditionalExpr *>(&expr))
            {
                return "(" +
                       expression(*conditional->condition) +
                       " ? " +
                       expression(*conditional->when_true) +
                       " : " +
                       expression(*conditional->when_false) +
                       ")";
            }
            if (auto *make = dynamic_cast<const MakeExpr *>(&expr))
            {
                std::string res;
                res += type(make->type);
                res += '(';
                for (size_t i = 0; i < make->args.size(); i++)
                {
                    const Expr &expr = *(make->args[i]);
                    res += expression(expr);
                    if (i + 1 != make->args.size())
                        res += ", ";
                }
                res += ")";
                return res;
            }

            return "";
        }

        static std::string unary_operator(TokenKind kind)
        {
            switch (kind)
            {
            case TokenKind::Plus:
                return "+";

            case TokenKind::Minus:
                return "-";

            case TokenKind::LogicalNot:
                return "!";

            case TokenKind::BitNot:
                return "~";

            case TokenKind::Star:
                return "*";

            case TokenKind::BitAnd:
                return "&";

            case TokenKind::PlusPlus:
                return "++";

            case TokenKind::MinusMinus:
                return "--";

            default:
                return "";
            }
        }

        static std::string binary_operator(TokenKind kind)
        {
            switch (kind)
            {
            case TokenKind::Plus:
                return "+";

            case TokenKind::Minus:
                return "-";

            case TokenKind::Star:
                return "*";

            case TokenKind::Slash:
                return "/";

            case TokenKind::Percent:
                return "%";

            case TokenKind::Equal:
                return "=";

            case TokenKind::PlusEqual:
                return "+=";

            case TokenKind::MinusEqual:
                return "-=";

            case TokenKind::StarEqual:
                return "*=";

            case TokenKind::SlashEqual:
                return "/=";

            case TokenKind::PercentEqual:
                return "%=";

            case TokenKind::EqualEqual:
                return "==";

            case TokenKind::NotEqual:
                return "!=";

            case TokenKind::Less:
                return "<";

            case TokenKind::LessEqual:
                return "<=";

            case TokenKind::Greater:
                return ">";

            case TokenKind::GreaterEqual:
                return ">=";

            case TokenKind::LogicalAnd:
                return "&&";

            case TokenKind::LogicalOr:
                return "||";

            case TokenKind::BitAnd:
                return "&";

            case TokenKind::BitOr:
                return "|";

            case TokenKind::BitXor:
                return "^";

            default:
                return "";
            }
        }
    };
}
#endif
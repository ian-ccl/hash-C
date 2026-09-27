#ifndef PARSE_HPP
#define PARSE_HPP 

#include "../libs/transcode/transcode.hpp"
#include "types.hpp"

#include <vector>
#include <iostream>

namespace c {
    // ============================================================
    // PARSER
    // ============================================================

    class Parser {
        const std::vector<Token>& tokens;
        std::unordered_set<tc::String>& types;
        size_t pos = 0;

    public:
        explicit Parser(
            const std::vector<Token>& tokens,
            std::unordered_set<tc::String>& types
        )
            : tokens(tokens), types(types) {
        }

        tc::Opt<std::vector<std::unique_ptr<Decl>>> parse() {
            std::vector<std::unique_ptr<Decl>> result;

            while (!check(TokenKind::End)) {
                tc::Opt<std::unique_ptr<Decl>> decl =
                    declaration();

                if (!decl) {
                    error("expected declaration");
                    return tc::NullOpt;
                }

                result.push_back(std::move(*decl));
            }

            return result;
        }

    private:

        const Token& current() const {
            return tokens[pos];
        }

        bool check(TokenKind kind) const {
            return current().kind == kind;
        }

        bool match(TokenKind kind) {
            if (!check(kind))
                return false;

            ++pos;
            return true;
        }

        bool expect(TokenKind kind) {
            if (!match(kind)) {
                error_expected(kind);
                return false;
            }

            return true;
        }

        void error(const char* message) const {
            std::cerr
                << "parse error at token " << pos
                << ": " << message
                << " (got `" << current().text << "`)\n";
        }

        void error_expected(TokenKind kind) const {
            std::cerr
                << "parse error at token " << pos
                << ": expected "
                << token_name(kind)
                << ", got "
                << token_name(current().kind)
                << " (`" << current().text << "`)\n";
        }

        static const char* token_name(TokenKind kind) {
            switch (kind) {
                case TokenKind::End:         return "end of file";
                case TokenKind::Identifier:  return "identifier";
                case TokenKind::Fn:          return "`fn`";
                case TokenKind::Struct:      return "`struct`";
                case TokenKind::CStruct:     return "`cstruct`";
                case TokenKind::Typedef:     return "`typedef`";
                case TokenKind::Enum:        return "`enum`";
                case TokenKind::Defer:       return "`defer`";
                case TokenKind::Cpp:         return "`@c++`";
                case TokenKind::LParen:       return "`(`";
                case TokenKind::RParen:       return "`)`";
                case TokenKind::LBrace:       return "`{`";
                case TokenKind::RBrace:       return "`}`";
                case TokenKind::LBracket:     return "`[`";
                case TokenKind::RBracket:     return "`]`";
                case TokenKind::Semicolon:    return "`;`";
                case TokenKind::Comma:        return "`,`";
                case TokenKind::Colon:        return "`:`";
                case TokenKind::Dot:          return "`.`";
                case TokenKind::Arrow:        return "`->`";
                case TokenKind::Star:         return "`*`";
                case TokenKind::Plus:         return "`+`";
                case TokenKind::Minus:        return "`-`";
                case TokenKind::Slash:        return "`/`";
                case TokenKind::Percent:      return "`%`";
                case TokenKind::Equal:        return "`=`";
                case TokenKind::EqualEqual:   return "`==`";
                case TokenKind::NotEqual:     return "`!=`";
                case TokenKind::Less:         return "`<`";
                case TokenKind::LessEqual:    return "`<=`";
                case TokenKind::Greater:      return "`>`";
                case TokenKind::GreaterEqual: return "`>=`";
                case TokenKind::LogicalOr:    return "`||`";
                case TokenKind::LogicalAnd:   return "`&&`";
                case TokenKind::BitOr:        return "`|`";
                case TokenKind::BitXor:       return "`^`";
                case TokenKind::BitAnd:       return "`&`";
                case TokenKind::LogicalNot:   return "`!`";
                case TokenKind::BitNot:       return "`~`";
                case TokenKind::PlusEqual:    return "`+=`";
                case TokenKind::MinusEqual:   return "`-=`";
                case TokenKind::StarEqual:    return "`*=`";
                case TokenKind::SlashEqual:   return "`/=`";
                case TokenKind::PercentEqual: return "`%=`";
                case TokenKind::PlusPlus:     return "`++`";
                case TokenKind::MinusMinus:   return "`--`";
                case TokenKind::Question:     return "`?`";
                case TokenKind::Return:       return "`return`";
                case TokenKind::Break:        return "`break`";
                case TokenKind::Continue:     return "`continue`";
                case TokenKind::If:           return "`if`";
                case TokenKind::Else:         return "`else`";
                case TokenKind::While:        return "`while`";
                case TokenKind::Do:           return "`do`";
                case TokenKind::For:          return "`for`";
                case TokenKind::Make:         return "`make`";
                case TokenKind::Integer:      return "integer";
                case TokenKind::Float:        return "float";
                case TokenKind::String:       return "string";
                case TokenKind::Character:    return "character";
                case TokenKind::True:         return "`true`";
                case TokenKind::False:        return "`false`";
                case TokenKind::Nullptr:      return "`nullptr`";
            }

            return "unknown token";
        }

        tc::Opt<std::unique_ptr<Decl>> declaration() {
            if (check(TokenKind::Fn))
                return function();

            if (check(TokenKind::Struct))
                return structure(false);

            if (check(TokenKind::CStruct))
                return structure(true);

            if (check(TokenKind::Typedef))
                return typedef_decl();

            if (check(TokenKind::Enum))
                return enumeration();

            if (check(TokenKind::Cpp))
                return cpp();

            return tc::NullOpt;
        }

        tc::Opt<tc::String> qualified_name() {
            if (!check(TokenKind::Identifier)) {
                error_expected(TokenKind::Identifier);
                return tc::NullOpt;
            }

            tc::String result = current().text;
            ++pos;

            while (match(TokenKind::Colon)) {
                if (!check(TokenKind::Identifier)) {
                    error_expected(TokenKind::Identifier);
                    return tc::NullOpt;
                }

                result += ':';
                result += current().text;
                ++pos;
            }

            return result;
        }

        tc::Opt<std::unique_ptr<Decl>> function() {
            ++pos;

            tc::Opt<tc::String> optname = qualified_name();

            if (!optname)
                return tc::NullOpt;

            tc::String name = *optname;

            if (!expect(TokenKind::LParen))
                return tc::NullOpt;

            std::vector<Variable> params;

            if (!check(TokenKind::RParen)) {
                while (true) {
                    tc::Opt<Type> type = parse_type();

                    if (!type)
                        return tc::NullOpt;

                    if (!check(TokenKind::Identifier)) {
                        error_expected(TokenKind::Identifier);
                        return tc::NullOpt;
                    }

                    tc::String param_name = current().text;
                    ++pos;

                    Variable parameter;
                    parameter.type = std::move(*type);
                    parameter.name = std::move(param_name);

                    params.push_back(std::move(parameter));

                    if (!match(TokenKind::Comma))
                        break;
                }
            }

            if (!expect(TokenKind::RParen))
                return tc::NullOpt;

            tc::Opt<Type> return_type = parse_type();

            if (!return_type)
                return tc::NullOpt;

            std::unique_ptr<FunctionDecl> result =
                std::make_unique<FunctionDecl>(
                    std::move(name),
                    std::move(*return_type)
                );

            result->parameters = std::move(params);

            if (match(TokenKind::Semicolon)) {
                result->body = nullptr;
                return std::unique_ptr<Decl>(std::move(result));
            }

            tc::Opt<std::unique_ptr<BlockStmt>> body =
                block();

            if (!body)
                return tc::NullOpt;

            result->body = std::move(*body);

            return std::unique_ptr<Decl>(std::move(result));
        }

        tc::Opt<std::unique_ptr<Decl>> structure(bool copyable) {
            ++pos;

            tc::Opt<tc::String> optname = qualified_name();

            if (!optname)
                return tc::NullOpt;

            tc::String name = *optname;
            types.insert(name);

            if (!expect(TokenKind::LBrace))
                return tc::NullOpt;

            std::unique_ptr<StructDecl> result =
                std::make_unique<StructDecl>(
                    std::move(name),
                    copyable
                );

            while (!check(TokenKind::RBrace)) {
                tc::Opt<Type> type = parse_type();

                if (!type)
                    return tc::NullOpt;

                if (!check(TokenKind::Identifier)) {
                    error_expected(TokenKind::Identifier);
                    return tc::NullOpt;
                }

                tc::String field_name = current().text;
                ++pos;

                if (!expect(TokenKind::Semicolon))
                    return tc::NullOpt;

                Variable field;
                field.type = std::move(*type);
                field.name = std::move(field_name);

                result->fields.push_back(std::move(field));
            }

            ++pos;

            match(TokenKind::Semicolon);

            return std::unique_ptr<Decl>(std::move(result));
        }

        tc::Opt<std::unique_ptr<Decl>> typedef_decl() {
            ++pos;

            tc::Opt<tc::String> optname = qualified_name();

            if (!optname)
                return tc::NullOpt;

            tc::String name = *optname;
            types.insert(name);

            tc::Opt<Type> type = parse_type();

            if (!type)
                return tc::NullOpt;

            if (!expect(TokenKind::Semicolon))
                return tc::NullOpt;

            std::unique_ptr<TypedefDecl> result =
                std::make_unique<TypedefDecl>(
                    std::move(name),
                    std::move(*type)
                );

            return std::unique_ptr<Decl>(std::move(result));
        }

        tc::Opt<std::unique_ptr<Decl>> enumeration() {
            ++pos;

            tc::Opt<tc::String> optname = qualified_name();

            if (!optname)
                return tc::NullOpt;

            tc::String name = *optname;
            types.insert(name);

            if (!expect(TokenKind::LBrace))
                return tc::NullOpt;

            std::unique_ptr<EnumDecl> result =
                std::make_unique<EnumDecl>(
                    std::move(name)
                );

            while (!check(TokenKind::RBrace)) {
                if (!check(TokenKind::Identifier)) {
                    error_expected(TokenKind::Identifier);
                    return tc::NullOpt;
                }

                result->values.push_back(current().text);
                ++pos;

                if (!match(TokenKind::Comma))
                    break;
            }

            if (!expect(TokenKind::RBrace))
                return tc::NullOpt;

            match(TokenKind::Semicolon);

            return std::unique_ptr<Decl>(std::move(result));
        }

        tc::Opt<std::unique_ptr<Stmt>> defer() {
            ++pos;

            tc::Opt<std::unique_ptr<BlockStmt>> body =
                block();

            if (!body)
                return tc::NullOpt;

            std::unique_ptr<DeferStmt> result =
                std::make_unique<DeferStmt>(
                    std::move(*body)
                );

            return std::unique_ptr<Stmt>(std::move(result));
        }

        tc::Opt<std::unique_ptr<Decl>> cpp() {
            tc::String code = current().text;
            ++pos;

            return std::unique_ptr<Decl>(
                new Cpp{std::move(code)}
            );
        }

        tc::Opt<Type> parse_type() {
            Type type;

            tc::Opt<tc::String> name = qualified_name();

            if (!name)
                return tc::NullOpt;

            type = Type::named(std::move(*name));

            while (true) {
                if (match(TokenKind::Star)) {
                    type = Type::pointer(std::move(type));
                    continue;
                }

                if (match(TokenKind::LBracket)) {
                    if (match(TokenKind::RBracket)) {
                        type = Type::slice(std::move(type));
                        continue;
                    }

                    if (!check(TokenKind::Integer)) {
                        error_expected(TokenKind::Integer);
                        return tc::NullOpt;
                    }

                    size_t size =
                        std::strtoull(
                            current().text.c_str(),
                            nullptr,
                            0
                        );

                    ++pos;

                    if (!expect(TokenKind::RBracket))
                        return tc::NullOpt;

                    type = Type::array(
                        std::move(type),
                        size
                    );

                    continue;
                }

                if (match(TokenKind::LogicalNot)) {
                    type = Type::constant(std::move(type));
                    continue;
                }

                break;
            }

            return type;
        }

        tc::Opt<std::unique_ptr<BlockStmt>> block() {
            if (!expect(TokenKind::LBrace))
                return tc::NullOpt;

            std::unique_ptr<BlockStmt> result =
                std::make_unique<BlockStmt>();

            while (!check(TokenKind::RBrace)) {
                if (check(TokenKind::End)) {
                    error("unterminated block");
                    return tc::NullOpt;
                }

                tc::Opt<std::unique_ptr<Stmt>> statement =
                    statement_node();

                if (!statement)
                    return tc::NullOpt;

                result->statements.push_back(
                    std::move(*statement)
                );
            }

            ++pos;

            return result;
        }

        tc::Opt<std::unique_ptr<Stmt>> statement_node() {
            if (check(TokenKind::LBrace)) {
                tc::Opt<std::unique_ptr<BlockStmt>> b =
                    block();

                if (!b)
                    return tc::NullOpt;

                return std::unique_ptr<Stmt>(std::move(*b));
            }

            if (match(TokenKind::Return)) {
                std::unique_ptr<Expr> value;

                if (!check(TokenKind::Semicolon))
                    value = expression();

                if (!expect(TokenKind::Semicolon))
                    return tc::NullOpt;

                return std::make_unique<ReturnStmt>(
                    std::move(value)
                );
            }

            if (match(TokenKind::Break)) {
                if (!expect(TokenKind::Semicolon))
                    return tc::NullOpt;

                return std::make_unique<BreakStmt>();
            }

            if (match(TokenKind::Continue)) {
                if (!expect(TokenKind::Semicolon))
                    return tc::NullOpt;

                return std::make_unique<ContinueStmt>();
            }

            if (check(TokenKind::If))
                return if_statement();

            if (check(TokenKind::While))
                return while_statement();

            if (check(TokenKind::Do))
                return do_statement();

            if (check(TokenKind::For))
                return for_statement();
            if (check(TokenKind::Defer))
                return defer();

            if (looks_like_type()) {
                size_t saved = pos;

                tc::Opt<Type> type = parse_type();

                if (
                    type &&
                    check(TokenKind::Identifier)
                ) {
                    tc::String name = current().text;
                    ++pos;

                    std::unique_ptr<Expr> initializer;

                    if (match(TokenKind::Equal))
                        initializer = expression();

                    if (!expect(TokenKind::Semicolon))
                        return tc::NullOpt;

                    return std::make_unique<VariableDeclStmt>(
                        std::move(*type),
                        std::move(name),
                        std::move(initializer)
                    );
                }

                pos = saved;
            }

            std::unique_ptr<Expr> expr = expression();

            if (!expr) {
                error("expected statement or expression");
                return tc::NullOpt;
            }

            if (!expect(TokenKind::Semicolon))
                return tc::NullOpt;

            return std::make_unique<ExprStmt>(
                std::move(expr)
            );
        }

        bool looks_like_type() const {
            if (!check(TokenKind::Identifier))
                return false;

            const tc::String& name = current().text;

            return types.contains(name);
        }

        tc::Opt<std::unique_ptr<Stmt>> if_statement() {
            ++pos;

            if (!expect(TokenKind::LParen))
                return tc::NullOpt;

            std::unique_ptr<Expr> condition =
                expression();

            if (!condition) {
                error("expected expression in if condition");
                return tc::NullOpt;
            }

            if (!expect(TokenKind::RParen))
                return tc::NullOpt;

            tc::Opt<std::unique_ptr<Stmt>> then_branch =
                statement_node();

            if (!then_branch)
                return tc::NullOpt;

            std::unique_ptr<IfStmt> result =
                std::make_unique<IfStmt>();

            result->condition = std::move(condition);
            result->then_branch = std::move(*then_branch);

            if (match(TokenKind::Else)) {
                tc::Opt<std::unique_ptr<Stmt>> else_branch =
                    statement_node();

                if (!else_branch)
                    return tc::NullOpt;

                result->else_branch =
                    std::move(*else_branch);
            }

            return std::unique_ptr<Stmt>(std::move(result));
        }

        tc::Opt<std::unique_ptr<Stmt>> while_statement() {
            ++pos;

            if (!expect(TokenKind::LParen))
                return tc::NullOpt;

            std::unique_ptr<Expr> condition =
                expression();

            if (!condition) {
                error("expected expression in while condition");
                return tc::NullOpt;
            }

            if (!expect(TokenKind::RParen))
                return tc::NullOpt;

            tc::Opt<std::unique_ptr<Stmt>> body =
                statement_node();

            if (!body)
                return tc::NullOpt;

            std::unique_ptr<WhileStmt> result =
                std::make_unique<WhileStmt>();

            result->condition = std::move(condition);
            result->body = std::move(*body);

            return std::unique_ptr<Stmt>(std::move(result));
        }

        tc::Opt<std::unique_ptr<Stmt>> do_statement() {
            ++pos;

            tc::Opt<std::unique_ptr<Stmt>> body =
                statement_node();

            if (!body)
                return tc::NullOpt;

            if (!expect(TokenKind::While))
                return tc::NullOpt;

            if (!expect(TokenKind::LParen))
                return tc::NullOpt;

            std::unique_ptr<Expr> condition =
                expression();

            if (!condition) {
                error("expected expression in do-while condition");
                return tc::NullOpt;
            }

            if (!expect(TokenKind::RParen))
                return tc::NullOpt;

            if (!expect(TokenKind::Semicolon))
                return tc::NullOpt;

            std::unique_ptr<DoStmt> result =
                std::make_unique<DoStmt>();

            result->body = std::move(*body);
            result->condition = std::move(condition);

            return std::unique_ptr<Stmt>(std::move(result));
        }

        tc::Opt<std::unique_ptr<Stmt>> for_statement() {
            ++pos;

            if (!expect(TokenKind::LParen))
                return tc::NullOpt;

            std::unique_ptr<Stmt> initialization;

            if (!check(TokenKind::Semicolon)) {
                if (looks_like_type()) {
                    tc::Opt<Type> type = parse_type();

                    if (!type) {
                        error("invalid type in for initialization");
                        return tc::NullOpt;
                    }

                    if (!check(TokenKind::Identifier)) {
                        error_expected(TokenKind::Identifier);
                        return tc::NullOpt;
                    }

                    tc::String name = current().text;
                    ++pos;

                    std::unique_ptr<Expr> initializer;

                    if (match(TokenKind::Equal))
                        initializer = expression();

                    initialization =
                        std::make_unique<VariableDeclStmt>(
                            std::move(*type),
                            std::move(name),
                            std::move(initializer)
                        );
                }
                else {
                    std::unique_ptr<Expr> expr =
                        expression();

                    if (!expr) {
                        error("invalid for initialization");
                        return tc::NullOpt;
                    }

                    initialization =
                        std::make_unique<ExprStmt>(
                            std::move(expr)
                        );
                }
            }

            if (!expect(TokenKind::Semicolon))
                return tc::NullOpt;

            std::unique_ptr<Expr> condition;

            if (!check(TokenKind::Semicolon)) {
                condition = expression();

                if (!condition) {
                    error("invalid for condition");
                    return tc::NullOpt;
                }
            }

            if (!expect(TokenKind::Semicolon))
                return tc::NullOpt;

            std::unique_ptr<Expr> increment;

            if (!check(TokenKind::RParen)) {
                increment = expression();

                if (!increment) {
                    error("invalid for increment");
                    return tc::NullOpt;
                }
            }

            if (!expect(TokenKind::RParen))
                return tc::NullOpt;

            tc::Opt<std::unique_ptr<Stmt>> body =
                statement_node();

            if (!body)
                return tc::NullOpt;

            std::unique_ptr<ForStmt> result =
                std::make_unique<ForStmt>();

            result->initialization =
                std::move(initialization);

            result->condition =
                std::move(condition);

            result->increment =
                std::move(increment);

            result->body =
                std::move(*body);

            return std::unique_ptr<Stmt>(std::move(result));
        }

        // --------------------------------------------------------
        // Expressions
        // --------------------------------------------------------

        std::unique_ptr<Expr> expression() {
            return assignment();
        }

        std::unique_ptr<Expr> assignment() {
            std::unique_ptr<Expr> left =
                conditional();

            if (!left)
                return nullptr;

            if (
                check(TokenKind::Equal) ||
                check(TokenKind::PlusEqual) ||
                check(TokenKind::MinusEqual) ||
                check(TokenKind::StarEqual) ||
                check(TokenKind::SlashEqual) ||
                check(TokenKind::PercentEqual)
            ) {
                TokenKind op = current().kind;
                ++pos;

                std::unique_ptr<Expr> right =
                    assignment();

                if (!right) {
                    error("expected expression after assignment operator");
                    return nullptr;
                }

                return std::make_unique<BinaryExpr>(
                    std::move(left),
                    op,
                    std::move(right)
                );
            }

            return left;
        }

        std::unique_ptr<Expr> conditional() {
            std::unique_ptr<Expr> condition =
                logical_or();

            if (!condition)
                return nullptr;

            if (!match(TokenKind::Question))
                return condition;

            std::unique_ptr<Expr> when_true =
                expression();

            if (!when_true) {
                error("expected expression after `?`");
                return nullptr;
            }

            if (!expect(TokenKind::Colon))
                return nullptr;

            std::unique_ptr<Expr> when_false =
                conditional();

            if (!when_false) {
                error("expected expression after `:` in conditional");
                return nullptr;
            }

            return std::make_unique<ConditionalExpr>(
                std::move(condition),
                std::move(when_true),
                std::move(when_false)
            );
        }

        std::unique_ptr<Expr> logical_or() {
            return binary_level(
                &Parser::logical_and,
                TokenKind::LogicalOr
            );
        }

        std::unique_ptr<Expr> logical_and() {
            return binary_level(
                &Parser::bit_or,
                TokenKind::LogicalAnd
            );
        }

        std::unique_ptr<Expr> bit_or() {
            return binary_level(
                &Parser::bit_xor,
                TokenKind::BitOr
            );
        }

        std::unique_ptr<Expr> bit_xor() {
            return binary_level(
                &Parser::bit_and,
                TokenKind::BitXor
            );
        }

        std::unique_ptr<Expr> bit_and() {
            return binary_level(
                &Parser::equality,
                TokenKind::BitAnd
            );
        }

        std::unique_ptr<Expr> equality() {
            std::unique_ptr<Expr> left =
                comparison();

            while (
                check(TokenKind::EqualEqual) ||
                check(TokenKind::NotEqual)
            ) {
                TokenKind op = current().kind;
                ++pos;

                std::unique_ptr<Expr> right =
                    comparison();

                if (!right) {
                    error("expected expression after equality operator");
                    return nullptr;
                }

                left =
                    std::make_unique<BinaryExpr>(
                        std::move(left),
                        op,
                        std::move(right)
                    );
            }

            return left;
        }

        std::unique_ptr<Expr> comparison() {
            std::unique_ptr<Expr> left =
                additive();

            while (
                check(TokenKind::Less) ||
                check(TokenKind::LessEqual) ||
                check(TokenKind::Greater) ||
                check(TokenKind::GreaterEqual)
            ) {
                TokenKind op = current().kind;
                ++pos;

                std::unique_ptr<Expr> right =
                    additive();

                if (!right) {
                    error("expected expression after comparison operator");
                    return nullptr;
                }

                left =
                    std::make_unique<BinaryExpr>(
                        std::move(left),
                        op,
                        std::move(right)
                    );
            }

            return left;
        }

        std::unique_ptr<Expr> additive() {
            std::unique_ptr<Expr> left =
                multiplicative();

            while (
                check(TokenKind::Plus) ||
                check(TokenKind::Minus)
            ) {
                TokenKind op = current().kind;
                ++pos;

                std::unique_ptr<Expr> right =
                    multiplicative();

                if (!right) {
                    error("expected expression after additive operator");
                    return nullptr;
                }

                left =
                    std::make_unique<BinaryExpr>(
                        std::move(left),
                        op,
                        std::move(right)
                    );
            }

            return left;
        }

        std::unique_ptr<Expr> multiplicative() {
            std::unique_ptr<Expr> left =
                unary();

            while (
                check(TokenKind::Star) ||
                check(TokenKind::Slash) ||
                check(TokenKind::Percent)
            ) {
                TokenKind op = current().kind;
                ++pos;

                std::unique_ptr<Expr> right =
                    unary();

                if (!right) {
                    error("expected expression after multiplicative operator");
                    return nullptr;
                }

                left =
                    std::make_unique<BinaryExpr>(
                        std::move(left),
                        op,
                        std::move(right)
                    );
            }

            return left;
        }

        std::unique_ptr<Expr> unary() {
            if (
                check(TokenKind::Plus) ||
                check(TokenKind::Minus) ||
                check(TokenKind::LogicalNot) ||
                check(TokenKind::BitNot) ||
                check(TokenKind::Star) ||
                check(TokenKind::BitAnd) ||
                check(TokenKind::PlusPlus) ||
                check(TokenKind::MinusMinus)
            ) {
                TokenKind op = current().kind;
                ++pos;

                std::unique_ptr<Expr> operand =
                    unary();

                if (!operand) {
                    error("expected expression after unary operator");
                    return nullptr;
                }

                return std::make_unique<UnaryExpr>(
                    op,
                    std::move(operand)
                );
            }

            return postfix();
        }

        std::unique_ptr<Expr> postfix() {
            std::unique_ptr<Expr> result =
                primary();

            if (!result)
                return nullptr;

            while (true) {
                if (match(TokenKind::LParen)) {
                    std::unique_ptr<CallExpr> call =
                        std::make_unique<CallExpr>(
                            std::move(result)
                        );

                    if (!check(TokenKind::RParen)) {
                        while (true) {
                            std::unique_ptr<Expr> arg =
                                expression();

                            if (!arg) {
                                error("invalid function argument");
                                return nullptr;
                            }

                            call->args.push_back(
                                std::move(arg)
                            );

                            if (!match(TokenKind::Comma))
                                break;
                        }
                    }

                    if (!expect(TokenKind::RParen))
                        return nullptr;

                    result = std::move(call);
                    continue;
                }

                if (match(TokenKind::LBracket)) {
                    std::unique_ptr<Expr> index =
                        expression();

                    if (!index) {
                        error("invalid index expression");
                        return nullptr;
                    }

                    if (!expect(TokenKind::RBracket))
                        return nullptr;

                    result =
                        std::make_unique<IndexExpr>(
                            std::move(result),
                            std::move(index)
                        );

                    continue;
                }

                if (match(TokenKind::Dot)) {
                    if (!check(TokenKind::Identifier)) {
                        error_expected(TokenKind::Identifier);
                        return nullptr;
                    }

                    tc::String member = current().text;
                    ++pos;

                    result =
                        std::make_unique<MemberExpr>(
                            std::move(result),
                            std::move(member),
                            false
                        );

                    continue;
                }

                if (match(TokenKind::Arrow)) {
                    if (!check(TokenKind::Identifier)) {
                        error_expected(TokenKind::Identifier);
                        return nullptr;
                    }

                    tc::String member = current().text;
                    ++pos;

                    result =
                        std::make_unique<MemberExpr>(
                            std::move(result),
                            std::move(member),
                            true
                        );

                    continue;
                }

                if (
                    match(TokenKind::PlusPlus) ||
                    match(TokenKind::MinusMinus)
                ) {
                    TokenKind op =
                        tokens[pos - 1].kind;

                    result =
                        std::make_unique<UnaryExpr>(
                            op,
                            std::move(result)
                        );

                    continue;
                }

                break;
            }

            return result;
        }

        std::unique_ptr<Expr> primary() {
            if (
                check(TokenKind::Integer) ||
                check(TokenKind::Float) ||
                check(TokenKind::String) ||
                check(TokenKind::Character) ||
                check(TokenKind::True) ||
                check(TokenKind::False) ||
                check(TokenKind::Nullptr)
            ) {
                tc::String value = current().text;
                ++pos;

                return std::make_unique<LiteralExpr>(
                    std::move(value)
                );
            }

            if (match(TokenKind::Make)) {
                tc::Opt<Type> type = parse_type();

                if (!type) {
                    error("expected type after `make`");
                    return nullptr;
                }

                if (!expect(TokenKind::LBrace))
                    return nullptr;

                std::vector<std::unique_ptr<Expr>> args;

                if (!check(TokenKind::RBrace)) {
                    while (true) {
                        std::unique_ptr<Expr> arg =
                            expression();

                        if (!arg) {
                            error("invalid argument in `make`");
                            return nullptr;
                        }

                        args.push_back(std::move(arg));

                        if (!match(TokenKind::Comma))
                            break;
                    }
                }

                if (!expect(TokenKind::RBrace))
                    return nullptr;

                return std::unique_ptr<Expr>(
                    new MakeExpr{
                        std::move(*type),
                        std::move(args)
                    }
                );
            }

            if (check(TokenKind::Identifier)) {
                tc::Opt<tc::String> name = qualified_name();

                if (!name)
                    return nullptr;

                return std::make_unique<NameExpr>(
                    std::move(*name)
                );
            }

            if (match(TokenKind::LParen)) {
                std::unique_ptr<Expr> result =
                    expression();

                if (!result) {
                    error("expected expression after `(`");
                    return nullptr;
                }

                if (!expect(TokenKind::RParen))
                    return nullptr;

                return result;
            }

            error("expected expression");
            return nullptr;
        }

        using BinaryParser =
            std::unique_ptr<Expr> (Parser::*)();

        std::unique_ptr<Expr> binary_level(
            BinaryParser next,
            TokenKind op
        ) {
            std::unique_ptr<Expr> left =
                (this->*next)();

            while (check(op)) {
                ++pos;

                std::unique_ptr<Expr> right =
                    (this->*next)();

                if (!right) {
                    error("expected expression after binary operator");
                    return nullptr;
                }

                left =
                    std::make_unique<BinaryExpr>(
                        std::move(left),
                        op,
                        std::move(right)
                    );
            }

            return left;
        }
    };
}

#endif
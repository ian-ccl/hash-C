#ifndef COMPILE_HPP
#define COMPILE_HPP
#include "../libs/transcode/transcode.hpp"
#include "types.hpp"
#include "transpile.hpp"
#include <memory>
#include "config.h"
#if DEBUG == 1
#include <iostream>
#endif
namespace c{
    // ============================================================
    // COMPILER
    // ============================================================

    tc::Opt<std::string> compile(
        const tc::Source& source,
        std::unordered_set<tc::String>& types
    ) {
        Lexer lexer(source);

        tc::Opt<std::vector<Token>> tokens =
            lexer.lex();

        if (!tokens) {
            if constexpr (DEBUG)
                std::cerr << "lex error\n";
            return tc::NullOpt;
        }

        Parser parser(*tokens, types);

        tc::Opt<
            std::vector<std::unique_ptr<Decl>>
        > ast = parser.parse();

        if (!ast) {
            if constexpr (DEBUG)
                std::cerr << "parse error\n";
            return tc::NullOpt;
        }

        Codegen codegen;

        return codegen.generate(*ast);
    }
}

#endif
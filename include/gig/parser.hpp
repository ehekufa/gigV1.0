#pragma once
#include "lexer.hpp"
#include "ast.hpp"
#include <memory>

namespace gig {

class Parser {
public:
    Parser(Lexer& lex);
    std::unique_ptr<AstBlock> parse();

private:
    Lexer& lexer;
    Token current;

    void advance();
    void expect(TokenType type);
    bool match(TokenType type);

    std::unique_ptr<AstNode> parseExpr();
    std::unique_ptr<AstNode> parsePrimary();
    std::unique_ptr<AstNode> parseUnary();
    std::unique_ptr<AstNode> parseBinary(int prec = 0);
    std::unique_ptr<AstNode> parseIf();
    std::unique_ptr<AstNode> parseWhile();
    std::unique_ptr<AstNode> parseFunctionDef();
    std::unique_ptr<AstNode> parseCall(std::unique_ptr<AstNode> callee);
    std::unique_ptr<AstBlock> parseBlock();
    std::unique_ptr<AstNode> parseStatement();
    std::unique_ptr<AstNode> parseAssignmentOrCall();
    std::unique_ptr<AstNode> parseLocal();
    std::unique_ptr<AstNode> parseReturn();
};

} // namespace gig

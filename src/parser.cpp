#include "parser.hpp"
#include <iostream>

namespace gig {

Parser::Parser(Lexer& lex) : lexer(lex) {
    advance();
}

void Parser::advance() {
    current = lexer.nextToken();
}

void Parser::expect(TokenType type) {
    if (current.type != type) {
        throw std::runtime_error("Expected token " + std::to_string((int)type) + " got " + current.lexeme);
    }
    advance();
}

bool Parser::match(TokenType type) {
    if (current.type == type) {
        advance();
        return true;
    }
    return false;
}

std::unique_ptr<AstBlock> Parser::parse() {
    auto block = std::make_unique<AstBlock>();
    while (current.type != TokenType::END_OF_FILE) {
        auto stmt = parseStatement();
        if (stmt) block->statements.push_back(std::move(stmt));
        else break;
    }
    return block;
}

std::unique_ptr<AstNode> Parser::parseStatement() {
    if (current.type == TokenType::KW_IF) return parseIf();
    if (current.type == TokenType::KW_WHILE) return parseWhile();
    if (current.type == TokenType::KW_FUNCTION) return parseFunctionDef();
    if (current.type == TokenType::KW_LOCAL) return parseLocal();
    if (current.type == TokenType::KW_RETURN) return parseReturn();
    return parseAssignmentOrCall();
}

std::unique_ptr<AstNode> Parser::parseAssignmentOrCall() {
    auto expr = parseExpr();
    if (current.type == TokenType::ASSIGN) {
        auto id = dynamic_cast<Identifier*>(expr.get());
        if (!id) throw std::runtime_error("Invalid assignment target");
        std::string name = id->name;
        advance();
        auto rhs = parseExpr();
        return std::make_unique<Assignment>(name, std::move(rhs));
    }
    return expr;
}

std::unique_ptr<AstNode> Parser::parseIf() {
    expect(TokenType::KW_IF);
    auto cond = parseExpr();
    expect(TokenType::KW_THEN);
    auto then_block = parseBlock();
    std::unique_ptr<AstNode> else_block = nullptr;
    if (match(TokenType::KW_ELSE)) {
        else_block = parseBlock();
    }
    expect(TokenType::KW_END);
    return std::make_unique<IfStmt>(std::move(cond), std::move(then_block), std::move(else_block));
}

std::unique_ptr<AstNode> Parser::parseWhile() {
    expect(TokenType::KW_WHILE);
    auto cond = parseExpr();
    expect(TokenType::KW_DO);
    auto body = parseBlock();
    expect(TokenType::KW_END);
    return std::make_unique<WhileStmt>(std::move(cond), std::move(body));
}

std::unique_ptr<AstNode> Parser::parseFunctionDef() {
    expect(TokenType::KW_FUNCTION);
    auto name = current.lexeme;
    expect(TokenType::IDENTIFIER);
    expect(TokenType::LPAREN);
    std::vector<std::string> params;
    if (current.type != TokenType::RPAREN) {
        do {
            params.push_back(current.lexeme);
            expect(TokenType::IDENTIFIER);
        } while (match(TokenType::COMMA));
    }
    expect(TokenType::RPAREN);
    auto body = parseBlock();
    expect(TokenType::KW_END);
    auto func_node = std::make_unique<FunctionDef>(params, std::move(body));
    return std::make_unique<Assignment>(name, std::move(func_node));
}

std::unique_ptr<AstNode> Parser::parseLocal() {
    expect(TokenType::KW_LOCAL);
    std::vector<std::string> names;
    names.push_back(current.lexeme);
    expect(TokenType::IDENTIFIER);
    while (match(TokenType::COMMA)) {
        names.push_back(current.lexeme);
        expect(TokenType::IDENTIFIER);
    }
    std::vector<std::unique_ptr<AstNode>> inits;
    if (match(TokenType::ASSIGN)) {
        do {
            inits.push_back(parseExpr());
        } while (match(TokenType::COMMA));
    }
    return std::make_unique<LocalStmt>(names, std::move(inits));
}

std::unique_ptr<AstNode> Parser::parseReturn() {
    expect(TokenType::KW_RETURN);
    if (current.type == TokenType::KW_END || current.type == TokenType::END_OF_FILE) {
        return std::make_unique<ReturnStmt>(nullptr);
    }
    auto expr = parseExpr();
    return std::make_unique<ReturnStmt>(std::move(expr));
}

std::unique_ptr<AstBlock> Parser::parseBlock() {
    auto block = std::make_unique<AstBlock>();
    while (current.type != TokenType::KW_END && current.type != TokenType::KW_ELSE &&
           current.type != TokenType::KW_ELSEIF && current.type != TokenType::END_OF_FILE) {
        auto stmt = parseStatement();
        if (stmt) block->statements.push_back(std::move(stmt));
    }
    return block;
}

int getPrecedence(TokenType type) {
    switch (type) {
        case TokenType::OR: return 1;
        case TokenType::AND: return 2;
        case TokenType::EQ: case TokenType::NE: case TokenType::LT: case TokenType::LE:
        case TokenType::GT: case TokenType::GE: return 3;
        case TokenType::CONCAT: return 4;
        case TokenType::PLUS: case TokenType::MINUS: return 5;
        case TokenType::STAR: case TokenType::SLASH: case TokenType::MOD: return 6;
        case TokenType::POWER: return 7;
        default: return 0;
    }
}

std::unique_ptr<AstNode> Parser::parseExpr() {
    return parseBinary(0);
}

std::unique_ptr<AstNode> Parser::parseBinary(int prec) {
    auto left = parseUnary();
    while (true) {
        int cur = getPrecedence(current.type);
        if (cur <= prec) break;
        TokenType op = current.type;
        advance();
        auto right = parseBinary(cur);
        char op_char = 0;
        switch (op) {
            case TokenType::PLUS: op_char = '+'; break;
            case TokenType::MINUS: op_char = '-'; break;
            case TokenType::STAR: op_char = '*'; break;
            case TokenType::SLASH: op_char = '/'; break;
            case TokenType::MOD: op_char = '%'; break;
            case TokenType::POWER: op_char = '^'; break;
            case TokenType::LT: op_char = '<'; break;
            case TokenType::LE: op_char = '<'; break;   // исправлено
            case TokenType::GT: op_char = '>'; break;
            case TokenType::GE: op_char = '>'; break;   // исправлено
            case TokenType::EQ: op_char = '='; break;
            case TokenType::NE: op_char = '!'; break;
            case TokenType::CONCAT: op_char = '.'; break;
            case TokenType::AND: op_char = '&'; break;
            case TokenType::OR: op_char = '|'; break;
            default: break;
        }
        left = std::make_unique<BinaryOp>(op_char, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<AstNode> Parser::parseUnary() {
    if (current.type == TokenType::MINUS) {
        advance();
        auto expr = parseUnary();
        return std::make_unique<UnaryOp>('-', std::move(expr));
    }
    if (current.type == TokenType::NOT) {
        advance();
        auto expr = parseUnary();
        return std::make_unique<UnaryOp>('!', std::move(expr));
    }
    return parsePrimary();
}

std::unique_ptr<AstNode> Parser::parsePrimary() {
    if (current.type == TokenType::NUMBER) {
        double val = std::stod(current.lexeme);
        advance();
        return std::make_unique<NumberExpr>(val);
    }
    if (current.type == TokenType::STRING) {
        std::string s = current.lexeme;
        advance();
        return std::make_unique<StringExpr>(s);
    }
    if (current.type == TokenType::KW_TRUE) {
        advance();
        return std::make_unique<BooleanExpr>(true);
    }
    if (current.type == TokenType::KW_FALSE) {
        advance();
        return std::make_unique<BooleanExpr>(false);
    }
    if (current.type == TokenType::KW_NIL) {
        advance();
        return std::make_unique<NilExpr>();
    }
    if (current.type == TokenType::IDENTIFIER) {
        std::string name = current.lexeme;
        advance();
        auto node = std::make_unique<Identifier>(name);
        if (current.type == TokenType::LPAREN) {
            return parseCall(std::move(node));
        }
        return node;
    }
    if (current.type == TokenType::LPAREN) {
        advance();
        auto expr = parseExpr();
        expect(TokenType::RPAREN);
        return expr;
    }
    // Табличный литерал (упрощённо: только пустые скобки)
    if (current.type == TokenType::LBRACE) {
        advance();
        while (current.type != TokenType::RBRACE && current.type != TokenType::END_OF_FILE) advance();
        expect(TokenType::RBRACE);
        return std::make_unique<NilExpr>(); // TODO: доработать
    }
    throw std::runtime_error("Unexpected token in primary: " + current.lexeme);
}

std::unique_ptr<AstNode> Parser::parseCall(std::unique_ptr<AstNode> callee) {
    expect(TokenType::LPAREN);
    std::vector<std::unique_ptr<AstNode>> args;
    if (current.type != TokenType::RPAREN) {
        do {
            args.push_back(parseExpr());
        } while (match(TokenType::COMMA));
    }
    expect(TokenType::RPAREN);
    return std::make_unique<Call>(std::move(callee), std::move(args));
}

} // namespace gig

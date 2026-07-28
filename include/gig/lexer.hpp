#pragma once
#include <string>
#include <vector>
#include <cctype>
#include <stdexcept>

namespace gig {

enum class TokenType {
    // Разделители
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    COMMA, SEMICOLON, COLON, DOT,
    // Операторы
    ASSIGN, PLUS, MINUS, STAR, SLASH, MOD, POWER,
    EQ, NE, LT, LE, GT, GE,
    NOT, AND, OR,
    CONCAT,
    // Ключевые слова
    KW_IF, KW_ELSE, KW_ELSEIF, KW_THEN, KW_END,
    KW_WHILE, KW_DO, KW_FOR, KW_IN,
    KW_FUNCTION, KW_LOCAL, KW_RETURN, KW_BREAK,
    KW_NIL, KW_TRUE, KW_FALSE,
    // Прочее
    IDENTIFIER, NUMBER, STRING,
    COMMENT, // пропускаем
    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line, column;
};

class Lexer {
public:
    Lexer(const std::string& src);
    Token nextToken();

private:
    std::string source;
    size_t pos;
    int line, col;

    char peek() const;
    char advance();
    void skipWhitespace();
    Token readIdentifier();
    Token readNumber();
    Token readString();
    Token readComment(); // пропускаем
    Token makeToken(TokenType type, const std::string& lex);
};

} // namespace gig

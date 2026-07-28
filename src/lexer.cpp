#include "lexer.hpp"
#include <cctype>

namespace gig {

Lexer::Lexer(const std::string& src) : source(src), pos(0), line(1), col(1) {}

char Lexer::peek() const {
    return (pos < source.size()) ? source[pos] : '\0';
}

char Lexer::advance() {
    char c = peek();
    if (c == '\n') { ++line; col = 1; } else { ++col; }
    if (pos < source.size()) ++pos;
    return c;
}

void Lexer::skipWhitespace() {
    while (std::isspace(peek())) advance();
}

Token Lexer::makeToken(TokenType type, const std::string& lex) {
    Token t{type, lex, line, col - (int)lex.size()};
    return t;
}

Token Lexer::readIdentifier() {
    std::string lex;
    while (std::isalnum(peek()) || peek() == '_') {
        lex += advance();
    }
    // Ключевые слова
    if (lex == "if") return makeToken(TokenType::KW_IF, lex);
    if (lex == "else") return makeToken(TokenType::KW_ELSE, lex);
    if (lex == "elseif") return makeToken(TokenType::KW_ELSEIF, lex);
    if (lex == "then") return makeToken(TokenType::KW_THEN, lex);
    if (lex == "end") return makeToken(TokenType::KW_END, lex);
    if (lex == "while") return makeToken(TokenType::KW_WHILE, lex);
    if (lex == "do") return makeToken(TokenType::KW_DO, lex);
    if (lex == "for") return makeToken(TokenType::KW_FOR, lex);
    if (lex == "in") return makeToken(TokenType::KW_IN, lex);
    if (lex == "function") return makeToken(TokenType::KW_FUNCTION, lex);
    if (lex == "local") return makeToken(TokenType::KW_LOCAL, lex);
    if (lex == "return") return makeToken(TokenType::KW_RETURN, lex);
    if (lex == "break") return makeToken(TokenType::KW_BREAK, lex);
    if (lex == "nil") return makeToken(TokenType::KW_NIL, lex);
    if (lex == "true") return makeToken(TokenType::KW_TRUE, lex);
    if (lex == "false") return makeToken(TokenType::KW_FALSE, lex);
    return makeToken(TokenType::IDENTIFIER, lex);
}

Token Lexer::readNumber() {
    std::string lex;
    while (std::isdigit(peek()) || peek() == '.') {
        lex += advance();
    }
    return makeToken(TokenType::NUMBER, lex);
}

Token Lexer::readString() {
    char quote = advance(); // " или '
    std::string lex;
    while (peek() != quote && peek() != '\0') {
        if (peek() == '\\') { lex += advance(); } // эскейп
        lex += advance();
    }
    if (peek() == quote) advance(); // закрывающая кавычка
    return makeToken(TokenType::STRING, lex);
}

Token Lexer::readComment() {
    // начинается с --
    while (peek() != '\n' && peek() != '\0') advance();
    return makeToken(TokenType::COMMENT, "");
}

Token Lexer::nextToken() {
    skipWhitespace();
    char c = peek();
    if (c == '\0') return makeToken(TokenType::END_OF_FILE, "");

    // Односимвольные
    if (c == '(') { advance(); return makeToken(TokenType::LPAREN, "("); }
    if (c == ')') { advance(); return makeToken(TokenType::RPAREN, ")"); }
    if (c == '{') { advance(); return makeToken(TokenType::LBRACE, "{"); }
    if (c == '}') { advance(); return makeToken(TokenType::RBRACE, "}"); }
    if (c == '[') { advance(); return makeToken(TokenType::LBRACKET, "["); }
    if (c == ']') { advance(); return makeToken(TokenType::RBRACKET, "]"); }
    if (c == ',') { advance(); return makeToken(TokenType::COMMA, ","); }
    if (c == ';') { advance(); return makeToken(TokenType::SEMICOLON, ";"); }
    if (c == ':') { advance(); return makeToken(TokenType::COLON, ":"); }
    if (c == '.') {
        advance();
        if (peek() == '.') {
            advance();
            if (peek() == '.') { advance(); return makeToken(TokenType::ELLIPSIS, "..."); }
            return makeToken(TokenType::CONCAT, "..");
        }
        return makeToken(TokenType::DOT, ".");
    }

    // Многосимвольные операторы
    if (c == '=') {
        advance();
        if (peek() == '=') { advance(); return makeToken(TokenType::EQ, "=="); }
        return makeToken(TokenType::ASSIGN, "=");
    }
    if (c == '!') {
        advance();
        if (peek() == '=') { advance(); return makeToken(TokenType::NE, "~="); }
        // ошибка
    }
    if (c == '<') {
        advance();
        if (peek() == '=') { advance(); return makeToken(TokenType::LE, "<="); }
        return makeToken(TokenType::LT, "<");
    }
    if (c == '>') {
        advance();
        if (peek() == '=') { advance(); return makeToken(TokenType::GE, ">="); }
        return makeToken(TokenType::GT, ">");
    }
    if (c == '+') { advance(); return makeToken(TokenType::PLUS, "+"); }
    if (c == '-') {
        advance();
        if (peek() == '-') { advance(); return readComment(); }
        return makeToken(TokenType::MINUS, "-");
    }
    if (c == '*') { advance(); return makeToken(TokenType::STAR, "*"); }
    if (c == '/') { advance(); return makeToken(TokenType::SLASH, "/"); }
    if (c == '%') { advance(); return makeToken(TokenType::MOD, "%"); }
    if (c == '^') { advance(); return makeToken(TokenType::POWER, "^"); }
    if (c == '&') { /* and? */ }
    if (c == '|') { /* or? */ }

    // Идентификатор или ключевое слово
    if (std::isalpha(c) || c == '_') {
        return readIdentifier();
    }
    // Число
    if (std::isdigit(c)) {
        return readNumber();
    }
    // Строка
    if (c == '"' || c == '\'') {
        return readString();
    }

    // Неизвестный символ
    advance();
    return makeToken(TokenType::END_OF_FILE, "");
}

} // namespace gig

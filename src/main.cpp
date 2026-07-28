#include "interpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: gig <filename>" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << argv[1] << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    gig::Lexer lexer(source);
    gig::Parser parser(lexer);
    auto ast = parser.parse();

    if (!ast) {
        std::cerr << "Parsing failed." << std::endl;
        return 1;
    }

    gig::Interpreter interpreter;
    interpreter.execute(ast.get());

    return 0;
}

#pragma once
#include "ast.hpp"
#include "environment.hpp"

namespace gig {
    // Только объявление функции регистрации (без включения builtins.hpp)
    void registerBuiltins(Environment* env);
}

namespace gig {

class Interpreter {
public:
    Interpreter();
    ~Interpreter();

    Value execute(AstBlock* root);

private:
    Environment* globals;
    void registerBuiltins();
};

} // namespace gig

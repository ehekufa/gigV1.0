#pragma once
#include "ast.hpp"
#include "environment.hpp"
#include "builtins.hpp"

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

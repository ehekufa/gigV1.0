#include "interpreter.hpp"
#include "builtins.hpp"

namespace gig {

Interpreter::Interpreter() {
    globals = new Environment(nullptr);
    registerBuiltins();
}

Interpreter::~Interpreter() {
    delete globals;
}

Value Interpreter::execute(AstBlock* root) {
    return root->execute(*globals);
}

void Interpreter::registerBuiltins() {
    gig::registerBuiltins(globals);
}

} // namespace gig

#include "function.hpp"
#include "ast.hpp"      // для AstBlock
#include "environment.hpp" // для Environment

namespace gig {

FunctionObj::FunctionObj(const std::vector<std::string>& p, AstBlock* b, Environment* env)
    : params(p), body(b), closure_env(env) {}

FunctionObj::~FunctionObj() {
    delete body;
}

} // namespace gig

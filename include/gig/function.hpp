#pragma once
#include "gc.hpp"
#include <vector>
#include <string>

namespace gig {

class Environment;
class AstBlock;   // теперь class, чтобы совпадало с ast.hpp

struct FunctionObj : public GCObject {
    std::vector<std::string> params;
    AstBlock* body;
    Environment* closure_env;

    FunctionObj(const std::vector<std::string>& p, AstBlock* b, Environment* env);
    ~FunctionObj() override;
};

} // namespace gig

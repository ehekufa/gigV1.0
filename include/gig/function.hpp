#pragma once
#include "gc.hpp"
#include <vector>
#include <string>

namespace gig {

// Forward declarations
class Environment;
struct AstBlock;

// Пользовательская функция (замыкание)
struct FunctionObj : public GCObject {
    std::vector<std::string> params;
    AstBlock* body;
    Environment* closure_env;

    FunctionObj(const std::vector<std::string>& p, AstBlock* b, Environment* env);
    ~FunctionObj() override;
};

} // namespace gig

#pragma once
#include "gc.hpp"
#include "value.hpp"
#include "ast.hpp"
#include <vector>

namespace gig {

// Пользовательская функция (замыкание)
struct FunctionObj : public GCObject {
    std::vector<std::string> params;
    AstBlock* body;              // владеет блоком (unique_ptr)
    Environment* closure_env;    // окружение, в котором создана функция

    FunctionObj(const std::vector<std::string>& p, AstBlock* b, Environment* env)
        : params(p), body(b), closure_env(env) {}

    ~FunctionObj() override {
        delete body;
        // closure_env не удаляем, он принадлежит вызывающей стороне
    }
};

// Встроенная C-функция
using CFunction = Value (*)(Environment&, const std::vector<Value>&);

struct CFunctionObj : public GCObject {
    CFunction func;
    CFunctionObj(CFunction f) : func(f) {}
};

} // namespace gig

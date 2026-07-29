#include "builtins.hpp"
#include "string.hpp"
#include "environment.hpp"
#include "function.hpp"
#include "ast.hpp"
#include <iostream>

namespace gig {

Value print_func(Environment&, const std::vector<Value>& args) {
    for (size_t i = 0; i < args.size(); ++i) {
        const Value& v = args[i];
        switch (v.type) {
            case Type::NIL: std::cout << "nil"; break;
            case Type::BOOLEAN: std::cout << (v.boolean ? "true" : "false"); break;
            case Type::NUMBER: std::cout << v.number; break;
            case Type::STRING: { auto s = v.as_string(); if (s) std::cout << s->data; break; }
            case Type::TABLE: std::cout << "table"; break;
            case Type::FUNCTION: std::cout << "function"; break;
            case Type::CFUNCTION: std::cout << "cfunction"; break;
            default: std::cout << "?";
        }
        if (i != args.size()-1) std::cout << "\t";
    }
    std::cout << std::endl;
    return Value();
}

Value collectgarbage_func(Environment&, const std::vector<Value>&) {
    std::cout << "GC is disabled in GIG." << std::endl;
    return Value();
}

Value type_func(Environment&, const std::vector<Value>& args) {
    if (args.empty()) return Value();
    const Value& v = args[0];
    const char* tname = "nil";
    switch (v.type) {
        case Type::NIL: tname = "nil"; break;
        case Type::BOOLEAN: tname = "boolean"; break;
        case Type::NUMBER: tname = "number"; break;
        case Type::STRING: tname = "string"; break;
        case Type::TABLE: tname = "table"; break;
        case Type::FUNCTION: tname = "function"; break;
        case Type::CFUNCTION: tname = "cfunction"; break;
        default: tname = "unknown";
    }
    auto s = new StringObj(tname);
    return Value(s);
}

Value pcall_func(Environment& env, const std::vector<Value>& args) {
    if (args.empty()) return Value();
    const Value& func_val = args[0];
    if (func_val.type != Type::FUNCTION) return Value();
    FunctionObj* func = func_val.as_function();
    if (!func || !func->body) return Value();

    Environment call_env(&env);
    size_t param_count = func->params.size();
    size_t arg_count = args.size() - 1;
    for (size_t i = 0; i < param_count; ++i) {
        if (i < arg_count) call_env.set(func->params[i], args[i+1]);
        else call_env.set(func->params[i], Value());
    }
    call_env.returned = false;
    Value result = func->body->execute(call_env);
    if (call_env.returned) return result;
    return Value();
}

// ----- Графические функции объявлены как extern (определены в main.cpp) -----
extern Value draw_pixel_func(Environment&, const std::vector<Value>&);
extern Value draw_rect_func(Environment&, const std::vector<Value>&);
extern Value draw_circle_func(Environment&, const std::vector<Value>&);
extern Value draw_text_func(Environment&, const std::vector<Value>&);
extern Value clear_func(Environment&, const std::vector<Value>&);
extern Value update_func(Environment&, const std::vector<Value>&);
extern Value key_pressed_func(Environment&, const std::vector<Value>&);

void registerBuiltins(Environment* env) {
    env->set("print", Value(new CFunctionObj(print_func)));
    env->set("collectgarbage", Value(new CFunctionObj(collectgarbage_func)));
    env->set("type", Value(new CFunctionObj(type_func)));
    env->set("pcall", Value(new CFunctionObj(pcall_func)));

    // Графика
    env->set("draw_pixel", Value(new CFunctionObj(draw_pixel_func)));
    env->set("draw_rect", Value(new CFunctionObj(draw_rect_func)));
    env->set("draw_circle", Value(new CFunctionObj(draw_circle_func)));
    env->set("draw_text", Value(new CFunctionObj(draw_text_func)));
    env->set("clear", Value(new CFunctionObj(clear_func)));
    env->set("update", Value(new CFunctionObj(update_func)));
    env->set("key_pressed", Value(new CFunctionObj(key_pressed_func)));
}

} // namespace gig

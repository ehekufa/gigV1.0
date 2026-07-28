#include "builtins.hpp"
#include "string.hpp"
#include <iostream>

namespace gig {

Value print_func(Environment&, const std::vector<Value>& args) {
    for (size_t i = 0; i < args.size(); ++i) {
        const Value& v = args[i];
        switch (v.type) {
            case Type::NIL: std::cout << "nil"; break;
            case Type::BOOLEAN: std::cout << (v.boolean ? "true" : "false"); break;
            case Type::NUMBER: std::cout << v.number; break;
            case Type::STRING: {
                auto s = v.as_string();
                if (s) std::cout << s->data;
                break;
            }
            case Type::TABLE: std::cout << "table"; break;
            case Type::FUNCTION: std::cout << "function"; break;
            case Type::CFUNCTION: std::cout << "cfunction"; break;
            default: std::cout << "?";
        }
        if (i != args.size() - 1) std::cout << "\t";
    }
    std::cout << std::endl;
    return Value();
}

Value collectgarbage_func(Environment&, const std::vector<Value>&) {
    // GC отключён, просто выдаём сообщение
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

void registerBuiltins(Environment* env) {
    // print
    auto print_cf = new CFunctionObj(print_func);
    env->set("print", Value(print_cf));
    // collectgarbage
    auto gc_cf = new CFunctionObj(collectgarbage_func);
    env->set("collectgarbage", Value(gc_cf));
    // type
    auto type_cf = new CFunctionObj(type_func);
    env->set("type", Value(type_cf));
}

} // namespace gig

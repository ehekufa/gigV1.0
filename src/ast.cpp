#include "ast.hpp"
#include "environment.hpp"
#include "function.hpp"
#include "string.hpp"
#include <iostream>
#include <cmath>

namespace gig {

// Реализации execute для узлов

Value AstBlock::execute(Environment& env) {
    Value last;
    for (auto& stmt : statements) {
        last = stmt->execute(env);
        if (env.returned) break;
    }
    return last;
}

Value StringExpr::execute(Environment&) {
    auto s = new StringObj(str.c_str());
    return Value(s);
}

Value Identifier::execute(Environment& env) {
    return env.get(name);
}

Value Assignment::execute(Environment& env) {
    Value val = expr->execute(env);
    env.set(name, val);
    return val;
}

Value BinaryOp::execute(Environment& env) {
    Value l = left->execute(env);
    Value r = right->execute(env);
    // Арифметика
    if (l.type == Type::NUMBER && r.type == Type::NUMBER) {
        double a = l.number, b = r.number;
        switch (op) {
            case '+': return Value(a + b);
            case '-': return Value(a - b);
            case '*': return Value(a * b);
            case '/': return Value(a / b);
            case '%': return Value(std::fmod(a, b));
            case '^': return Value(std::pow(a, b));
            case '<': return Value(a < b);
            case '>': return Value(a > b);
            case '=': return Value(a == b);
            case '!': return Value(a != b);
            default: break;
        }
    }
    // Сравнение строк
    if (l.type == Type::STRING && r.type == Type::STRING) {
        const char* sa = l.as_string()->data;
        const char* sb = r.as_string()->data;
        int cmp = strcmp(sa, sb);
        switch (op) {
            case '<': return Value(cmp < 0);
            case '>': return Value(cmp > 0);
            case '=': return Value(cmp == 0);
            case '!': return Value(cmp != 0);
            default: break;
        }
    }
    // Конкатенация
    if (op == '.' && l.type == Type::STRING && r.type == Type::STRING) {
        auto ls = l.as_string();
        auto rs = r.as_string();
        std::string concat = std::string(ls->data, ls->length) + std::string(rs->data, rs->length);
        auto s = new StringObj(concat.c_str());
        return Value(s);
    }
    // Логические операторы (and, or) – упрощённо
    if (op == '&') {
        // and: возвращает l, если l false, иначе r
        bool lb = (l.type == Type::BOOLEAN) ? l.boolean : (l.type != Type::NIL && l.type != Type::BOOLEAN);
        return lb ? r : l;
    }
    if (op == '|') {
        bool lb = (l.type == Type::BOOLEAN) ? l.boolean : (l.type != Type::NIL && l.type != Type::BOOLEAN);
        return lb ? l : r;
    }
    return Value(); // nil для остальных
}

Value UnaryOp::execute(Environment& env) {
    Value v = expr->execute(env);
    if (op == '-') {
        if (v.type == Type::NUMBER) return Value(-v.number);
        return Value();
    }
    if (op == '!') {
        bool b = (v.type == Type::BOOLEAN) ? v.boolean : (v.type != Type::NIL && v.type != Type::BOOLEAN);
        return Value(!b);
    }
    return Value();
}

Value IfStmt::execute(Environment& env) {
    Value cond = condition->execute(env);
    bool truth = (cond.type == Type::BOOLEAN) ? cond.boolean : (cond.type != Type::NIL && cond.type != Type::BOOLEAN);
    if (truth) {
        return then_block->execute(env);
    } else if (else_block) {
        return else_block->execute(env);
    }
    return Value();
}

Value WhileStmt::execute(Environment& env) {
    Value last;
    while (true) {
        Value cond = condition->execute(env);
        bool truth = (cond.type == Type::BOOLEAN) ? cond.boolean : (cond.type != Type::NIL && cond.type != Type::BOOLEAN);
        if (!truth) break;
        last = body->execute(env);
        if (env.returned) break;
    }
    return last;
}

Value FunctionDef::execute(Environment& env) {
    // Создаём объект функции, захватываем текущее окружение
    auto func = new FunctionObj(params, body.release(), &env);
    return Value(func);
}

Value Call::execute(Environment& env) {
    Value callee_val = callee->execute(env);
    // Вычисляем аргументы
    std::vector<Value> args_val;
    for (auto& arg : args) {
        args_val.push_back(arg->execute(env));
    }

    if (callee_val.type == Type::CFUNCTION) {
        auto cf = callee_val.as_cfunction();
        if (cf) {
            return cf->func(env, args_val);
        }
    } else if (callee_val.type == Type::FUNCTION) {
        auto func = callee_val.as_function();
        if (!func) return Value();
        // Создаём новое окружение для вызова
        Environment call_env(func->closure_env);
        // Привязываем параметры
        for (size_t i = 0; i < func->params.size() && i < args_val.size(); ++i) {
            call_env.set(func->params[i], args_val[i]);
        }
        // Если аргументов меньше, оставшиеся параметры = nil
        for (size_t i = args_val.size(); i < func->params.size(); ++i) {
            call_env.set(func->params[i], Value());
        }
        // Выполняем тело
        call_env.returned = false;
        Value result = func->body->execute(call_env);
        if (call_env.returned) {
            return result; // результат return
        }
        return Value(); // если нет return, возвращаем nil
    }
    return Value(); // не функция
}

Value ReturnStmt::execute(Environment& env) {
    env.returned = true;
    if (expr) {
        return expr->execute(env);
    }
    return Value();
}

Value LocalStmt::execute(Environment& env) {
    for (size_t i = 0; i < names.size(); ++i) {
        Value init = (i < inits.size()) ? inits[i]->execute(env) : Value();
        env.set(names[i], init);
    }
    return Value();
}

} // namespace gig

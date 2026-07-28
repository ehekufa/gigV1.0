#pragma once
#include "value.hpp"
#include <unordered_map>
#include <string>

namespace gig {

class Environment {
public:
    Environment* parent;
    std::unordered_map<std::string, Value> variables;
    bool returned = false; // флаг для return

    Environment(Environment* p = nullptr) : parent(p) {}

    // Поиск переменной вверх по цепочке
    Value get(const std::string& name) const {
        auto it = variables.find(name);
        if (it != variables.end()) return it->second;
        if (parent) return parent->get(name);
        return Value(); // nil
    }

    // Установка в текущей области
    void set(const std::string& name, const Value& val) {
        variables[name] = val;
    }

    // Создать новую вложенную область
    Environment* createChild() {
        return new Environment(this);
    }
};

} // namespace gig

#pragma once
#include "gc.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstring>

namespace gig {

enum class Type : uint8_t {
    NIL,
    BOOLEAN,
    NUMBER,
    STRING,
    TABLE,
    FUNCTION,
    CFUNCTION,   // встроенная C-функция
    THREAD,
    USERDATA
};

struct StringObj;
struct TableObj;
struct FunctionObj;
struct CFunctionObj;

struct Value {
    Type type;
    union {
        bool boolean;
        double number;
        GCObject* obj; // для строк, таблиц, функций
        CFunctionObj* cfunc; // для встроенных функций (тоже GCObject)
    };

    // Конструкторы
    Value() : type(Type::NIL) { obj = nullptr; }
    Value(bool b) : type(Type::BOOLEAN), boolean(b) {}
    Value(double n) : type(Type::NUMBER), number(n) {}
    Value(StringObj* s) : type(Type::STRING), obj(s) { if (obj) obj->ref(); }
    Value(TableObj* t) : type(Type::TABLE), obj(t) { if (obj) obj->ref(); }
    Value(FunctionObj* f) : type(Type::FUNCTION), obj(f) { if (obj) obj->ref(); }
    Value(CFunctionObj* cf) : type(Type::CFUNCTION), cfunc(cf) { if (cfunc) cfunc->ref(); }

    // Копирование
    Value(const Value& other) : type(other.type) {
        switch (type) {
            case Type::BOOLEAN: boolean = other.boolean; break;
            case Type::NUMBER:  number = other.number; break;
            case Type::CFUNCTION: cfunc = other.cfunc; if (cfunc) cfunc->ref(); break;
            default:            obj = other.obj; if (obj) obj->ref(); break;
        }
    }

    // Присваивание
    Value& operator=(const Value& other) {
        if (this == &other) return *this;
        // освобождаем старую
        if (is_object()) { if (obj) obj->unref(); }
        type = other.type;
        switch (type) {
            case Type::BOOLEAN: boolean = other.boolean; break;
            case Type::NUMBER:  number = other.number; break;
            case Type::CFUNCTION: cfunc = other.cfunc; if (cfunc) cfunc->ref(); break;
            default:            obj = other.obj; if (obj) obj->ref(); break;
        }
        return *this;
    }

    ~Value() {
        if (is_object() && obj) obj->unref();
    }

    bool is_object() const {
        return type == Type::STRING || type == Type::TABLE ||
               type == Type::FUNCTION || type == Type::THREAD ||
               type == Type::USERDATA || type == Type::CFUNCTION;
    }

    // Приведения (с проверкой)
    bool as_bool() const { return boolean; }
    double as_number() const { return number; }
    StringObj* as_string() const { return (type == Type::STRING) ? static_cast<StringObj*>(obj) : nullptr; }
    TableObj* as_table() const { return (type == Type::TABLE) ? static_cast<TableObj*>(obj) : nullptr; }
    FunctionObj* as_function() const { return (type == Type::FUNCTION) ? static_cast<FunctionObj*>(obj) : nullptr; }
    CFunctionObj* as_cfunction() const { return (type == Type::CFUNCTION) ? static_cast<CFunctionObj*>(cfunc) : nullptr; }
};

// Операторы сравнения для использования в std::unordered_map
inline bool operator==(const Value& a, const Value& b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case Type::NIL: return true;
        case Type::BOOLEAN: return a.boolean == b.boolean;
        case Type::NUMBER: return a.number == b.number;
        case Type::STRING: {
            auto sa = a.as_string();
            auto sb = b.as_string();
            return sa && sb && strcmp(sa->data, sb->data) == 0;
        }
        default: return a.obj == b.obj;
    }
}

} // namespace gig

// Хеш для Value (для std::unordered_map)
namespace std {
    template<> struct hash<gig::Value> {
        size_t operator()(const gig::Value& v) const {
            if (v.type == gig::Type::STRING) {
                auto s = v.as_string();
                if (s) return std::hash<std::string>()(std::string(s->data, s->length));
                return 0;
            }
            if (v.type == gig::Type::NUMBER) {
                return std::hash<double>()(v.number);
            }
            if (v.type == gig::Type::BOOLEAN) {
                return std::hash<bool>()(v.boolean);
            }
            // Для других типов используем указатель
            return std::hash<void*>()(v.obj);
        }
    };
}

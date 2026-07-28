#pragma once
#include "gc.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstring>

namespace gig {

// Forward declarations для всех объектных типов
struct StringObj;
struct TableObj;
struct FunctionObj;
struct CFunctionObj;
class Environment;

// Тип для C-функции (использует Value, поэтому объявляем после forward)
using CFunction = Value (*)(Environment&, const std::vector<Value>&);

enum class Type : uint8_t {
    NIL,
    BOOLEAN,
    NUMBER,
    STRING,
    TABLE,
    FUNCTION,
    CFUNCTION,
    THREAD,
    USERDATA
};

struct CFunctionObj : public GCObject {
    CFunction func;
    CFunctionObj(CFunction f) : func(f) {}
};

struct Value {
    Type type;
    union {
        bool boolean;
        double number;
        GCObject* obj;        // для StringObj, TableObj, FunctionObj
        CFunctionObj* cfunc;  // для CFunctionObj
    };

    // Объявления конструкторов / методов (определения позже)
    Value();
    Value(bool b);
    Value(double n);
    Value(StringObj* s);
    Value(TableObj* t);
    Value(FunctionObj* f);
    Value(CFunctionObj* cf);
    Value(const Value& other);
    Value& operator=(const Value& other);
    ~Value();

    bool is_object() const;

    StringObj*   as_string() const;
    TableObj*    as_table() const;
    FunctionObj* as_function() const;
    CFunctionObj* as_cfunction() const;
    bool as_bool() const;
    double as_number() const;
};

// Оператор сравнения и хеш (объявлены, определены после включения полных типов)
bool operator==(const Value& a, const Value& b);

} // namespace gig

namespace std {
    template<> struct hash<gig::Value> {
        size_t operator()(const gig::Value& v) const;
    };
}

// ------------------------------------------------------------
// Теперь включаем полные определения всех объектных типов
// ------------------------------------------------------------
#include "string.hpp"
#include "table.hpp"
#include "function.hpp"

// ------------------------------------------------------------
// Определения методов Value (теперь все типы известны)
// ------------------------------------------------------------
namespace gig {

inline Value::Value() : type(Type::NIL) { obj = nullptr; }
inline Value::Value(bool b) : type(Type::BOOLEAN), boolean(b) {}
inline Value::Value(double n) : type(Type::NUMBER), number(n) {}
inline Value::Value(StringObj* s) : type(Type::STRING), obj(s) { if (obj) obj->ref(); }
inline Value::Value(TableObj* t) : type(Type::TABLE), obj(t) { if (obj) obj->ref(); }
inline Value::Value(FunctionObj* f) : type(Type::FUNCTION), obj(f) { if (obj) obj->ref(); }
inline Value::Value(CFunctionObj* cf) : type(Type::CFUNCTION), cfunc(cf) { if (cfunc) cfunc->ref(); }

inline Value::Value(const Value& other) : type(other.type) {
    switch (type) {
        case Type::BOOLEAN: boolean = other.boolean; break;
        case Type::NUMBER:  number = other.number; break;
        case Type::CFUNCTION: cfunc = other.cfunc; if (cfunc) cfunc->ref(); break;
        default:            obj = other.obj; if (obj) obj->ref(); break;
    }
}

inline Value& Value::operator=(const Value& other) {
    if (this == &other) return *this;
    if (is_object() && obj) obj->unref();
    type = other.type;
    switch (type) {
        case Type::BOOLEAN: boolean = other.boolean; break;
        case Type::NUMBER:  number = other.number; break;
        case Type::CFUNCTION: cfunc = other.cfunc; if (cfunc) cfunc->ref(); break;
        default:            obj = other.obj; if (obj) obj->ref(); break;
    }
    return *this;
}

inline Value::~Value() {
    if (is_object() && obj) obj->unref();
}

inline bool Value::is_object() const {
    return type == Type::STRING || type == Type::TABLE ||
           type == Type::FUNCTION || type == Type::THREAD ||
           type == Type::USERDATA || type == Type::CFUNCTION;
}

inline bool Value::as_bool() const { return boolean; }
inline double Value::as_number() const { return number; }
inline StringObj* Value::as_string() const { return (type == Type::STRING) ? static_cast<StringObj*>(obj) : nullptr; }
inline TableObj* Value::as_table() const { return (type == Type::TABLE) ? static_cast<TableObj*>(obj) : nullptr; }
inline FunctionObj* Value::as_function() const { return (type == Type::FUNCTION) ? static_cast<FunctionObj*>(obj) : nullptr; }
inline CFunctionObj* Value::as_cfunction() const { return (type == Type::CFUNCTION) ? static_cast<CFunctionObj*>(cfunc) : nullptr; }

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

namespace std {
    inline size_t hash<gig::Value>::operator()(const gig::Value& v) const {
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
        return std::hash<void*>()(v.obj);
    }
}

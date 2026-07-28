#pragma once
#include "gc.hpp"
#include "value.hpp"
#include <unordered_map>

namespace gig {

struct TableObj : public GCObject {
    std::unordered_map<Value, Value> map;

    TableObj() = default;
    ~TableObj() override = default; // map автоматически удалит Value, которые уменьшат счётчики

    Value get(const Value& key) const {
        auto it = map.find(key);
        if (it != map.end()) return it->second;
        return Value(); // nil
    }

    void set(const Value& key, const Value& value) {
        map[key] = value;
    }
};

} // namespace gig

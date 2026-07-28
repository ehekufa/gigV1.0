#pragma once
#include "gc.hpp"
#include <cstring>

namespace gig {

struct StringObj : public GCObject {
    char* data;
    size_t length;

    StringObj(const char* str) {
        length = strlen(str);
        data = new char[length + 1];
        memcpy(data, str, length + 1);
    }

    ~StringObj() override {
        delete[] data;
    }
};

} // namespace gig

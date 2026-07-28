#pragma once

namespace gig {

class GCObject {
public:
    int refcount;
    GCObject() : refcount(1) {}
    virtual ~GCObject() = default;

    void ref() { ++refcount; }
    void unref() {
        if (--refcount == 0) {
            delete this;
        }
    }
};

} // namespace gig

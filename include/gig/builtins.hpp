#pragma once
#include "value.hpp"
#include <vector>
#include <iostream>

namespace gig {

class Environment;   // forward declaration

Value print_func(Environment& env, const std::vector<Value>& args);
Value collectgarbage_func(Environment& env, const std::vector<Value>& args);
Value type_func(Environment& env, const std::vector<Value>& args);

void registerBuiltins(Environment* env);

} // namespace gig

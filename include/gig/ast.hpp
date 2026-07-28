#pragma once
#include "value.hpp"
#include <memory>
#include <vector>
#include <string>

namespace gig {

class Environment;

class AstNode {
public:
    virtual ~AstNode() = default;
    virtual Value execute(Environment& env) = 0;
};

class AstBlock : public AstNode {
public:
    std::vector<std::unique_ptr<AstNode>> statements;
    Value execute(Environment& env) override;
};

class NumberExpr : public AstNode {
    double val;
public:
    NumberExpr(double v) : val(v) {}
    Value execute(Environment&) override { return Value(val); }
};

class BooleanExpr : public AstNode {
    bool val;
public:
    BooleanExpr(bool v) : val(v) {}
    Value execute(Environment&) override { return Value(val); }
};

class NilExpr : public AstNode {
public:
    Value execute(Environment&) override { return Value(); }
};

class StringExpr : public AstNode {
    std::string str;
public:
    StringExpr(const std::string& s) : str(s) {}
    Value execute(Environment& env) override;
};

class Identifier : public AstNode {
public:   // <-- теперь public
    std::string name;
    Identifier(const std::string& n) : name(n) {}
    Value execute(Environment& env) override;
};

class Assignment : public AstNode {
    std::string name;
    std::unique_ptr<AstNode> expr;
public:
    Assignment(const std::string& n, std::unique_ptr<AstNode> e)
        : name(n), expr(std::move(e)) {}
    Value execute(Environment& env) override;
};

class BinaryOp : public AstNode {
    char op;
    std::unique_ptr<AstNode> left, right;
public:
    BinaryOp(char o, std::unique_ptr<AstNode> l, std::unique_ptr<AstNode> r)
        : op(o), left(std::move(l)), right(std::move(r)) {}
    Value execute(Environment& env) override;
};

class UnaryOp : public AstNode {
    char op;
    std::unique_ptr<AstNode> expr;
public:
    UnaryOp(char o, std::unique_ptr<AstNode> e) : op(o), expr(std::move(e)) {}
    Value execute(Environment& env) override;
};

class IfStmt : public AstNode {
    std::unique_ptr<AstNode> condition;
    std::unique_ptr<AstNode> then_block;
    std::unique_ptr<AstNode> else_block;
public:
    IfStmt(std::unique_ptr<AstNode> cond,
           std::unique_ptr<AstNode> then_b,
           std::unique_ptr<AstNode> else_b = nullptr)
        : condition(std::move(cond)), then_block(std::move(then_b)), else_block(std::move(else_b)) {}
    Value execute(Environment& env) override;
};

class WhileStmt : public AstNode {
    std::unique_ptr<AstNode> condition;
    std::unique_ptr<AstNode> body;
public:
    WhileStmt(std::unique_ptr<AstNode> cond, std::unique_ptr<AstNode> b)
        : condition(std::move(cond)), body(std::move(b)) {}
    Value execute(Environment& env) override;
};

class FunctionDef : public AstNode {
    std::vector<std::string> params;
    std::unique_ptr<AstBlock> body;
public:
    FunctionDef(const std::vector<std::string>& p, std::unique_ptr<AstBlock> b)
        : params(p), body(std::move(b)) {}
    Value execute(Environment& env) override;
};

class Call : public AstNode {
    std::unique_ptr<AstNode> callee;
    std::vector<std::unique_ptr<AstNode>> args;
public:
    Call(std::unique_ptr<AstNode> c, std::vector<std::unique_ptr<AstNode>> a)
        : callee(std::move(c)), args(std::move(a)) {}
    Value execute(Environment& env) override;
};

class ReturnStmt : public AstNode {
    std::unique_ptr<AstNode> expr;
public:
    ReturnStmt(std::unique_ptr<AstNode> e) : expr(std::move(e)) {}
    Value execute(Environment& env) override;
};

class LocalStmt : public AstNode {
    std::vector<std::string> names;
    std::vector<std::unique_ptr<AstNode>> inits;
public:
    LocalStmt(const std::vector<std::string>& n, std::vector<std::unique_ptr<AstNode>> i)
        : names(n), inits(std::move(i)) {}
    Value execute(Environment& env) override;
};

} // namespace gig

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "mytoy/Lexer.h"

namespace mytoy {

std::ostream &INDENT(std::ostream &os, int indent);

class Expr {
public:
  enum class ExprKind { Variable, Tensor, VarDef, Mul, Return, ExprList, Call };

  Expr(Location location, ExprKind kind)
      : location(std::move(location)), kind(kind) {}
  virtual ~Expr() = default;

  const Location &getLoc() const { return location; }
  const ExprKind &getKind() const { return kind; }

  virtual void dump(std::ostream &os, int indent = 0) = 0;

private:
  Location location;
  ExprKind kind;
};

class VariableExpr : public Expr {
public:
  VariableExpr(Location location, std::string name)
      : Expr(std::move(location), ExprKind::Variable), name(std::move(name)) {}

  const std::string &getName() const { return name; }

  static bool classof(Expr *e) { return e->getKind() == ExprKind::Variable; }

  void dump(std::ostream &os, int indent = 0) override;

private:
  std::string name;
};

class TensorExpr : public Expr {
public:
  TensorExpr(Location location, std::vector<std::vector<double>> values,
             std::vector<int64_t> shape)
      : Expr(std::move(location), ExprKind::Tensor), values(std::move(values)),
        shape(std::move(shape)) {}

  std::vector<std::vector<double>> getValues() const { return values; }
  std::vector<int64_t> getShape() const { return shape; }

  static bool classof(Expr *e) { return e->getKind() == ExprKind::Tensor; }

  void dump(std::ostream &os, int indent = 0) override;

private:
  std::vector<std::vector<double>> values;
  std::vector<int64_t> shape;
};

struct VarType {
  std::vector<int64_t> shape;
};

class VarDefExpr : public Expr {
public:
  VarDefExpr(Location location, std::string name, std::unique_ptr<Expr> initVal,
             std::vector<int64_t> shape)
      : Expr(std::move(location), ExprKind::VarDef), name(std::move(name)),
        initVal(std::move(initVal)) {
    type.shape = std::move(shape);
  }

  const std::unique_ptr<Expr> &getInitVal() const { return initVal; }
  const std::string &getName() const { return name; }
  const VarType &getType() const { return type; }

  static bool classof(Expr *e) { return e->getKind() == ExprKind::VarDef; }

  void dump(std::ostream &os, int indent = 0) override;

private:
  std::unique_ptr<Expr> initVal;
  VarType type;
  std::string name;
};

class MulExpr : public Expr {
public:
  MulExpr(Location location, std::unique_ptr<Expr> lhs,
          std::unique_ptr<Expr> rhs)
      : Expr(std::move(location), ExprKind::Mul), lhs(std::move(lhs)),
        rhs(std::move(rhs)) {}

  const std::unique_ptr<Expr> &getLHS() const { return lhs; }
  const std::unique_ptr<Expr> &getRHS() const { return rhs; }

  static bool classof(Expr *e) { return e->getKind() == ExprKind::Mul; }

  void dump(std::ostream &os, int indent = 0) override;

private:
  std::unique_ptr<Expr> lhs;
  std::unique_ptr<Expr> rhs;
};

class ReturnExpr : public Expr {
public:
  ReturnExpr(Location location, std::unique_ptr<Expr> expr)
      : Expr(std::move(location), ExprKind::Return), expr(std::move(expr)) {}

  const std::unique_ptr<Expr> &getExpr() const { return expr; }

  static bool classof(Expr *e) { return e->getKind() == ExprKind::Return; }

  void dump(std::ostream &os, int indent = 0) override;

private:
  std::unique_ptr<Expr> expr;
};

class CallExpr : public Expr {
public:
  CallExpr(Location location, std::string func,
           std::vector<std::unique_ptr<Expr>> args)
      : Expr(std::move(location), ExprKind::Call), func(std::move(func)),
        args(std::move(args)) {}

  const std::vector<std::unique_ptr<Expr>> &getArgs() const { return args; }
  const std::string &getFunc() const { return func; }

  static bool classof(Expr *e) { return e->getKind() == ExprKind::Call; }

  void dump(std::ostream &os, int indent = 0) override;

private:
  std::vector<std::unique_ptr<Expr>> args;
  std::string func;
};

class ExprList : public Expr {
public:
  ExprList(Location location, std::vector<std::unique_ptr<Expr>> exprs)
      : Expr(std::move(location), ExprKind::ExprList), exprs(std::move(exprs)) {
  }

  const std::vector<std::unique_ptr<Expr>> &getExprs() const { return exprs; }

  static bool classof(Expr *e) { return e->getKind() == ExprKind::ExprList; }

  void dump(std::ostream &os, int indent = 0) override;

private:
  std::vector<std::unique_ptr<Expr>> exprs;
};

class Protocol {
public:
  Protocol(std::vector<std::unique_ptr<VariableExpr>> args, std::string func)
      : args(std::move(args)), func(std::move(func)) {}

  const std::vector<std::unique_ptr<VariableExpr>> &getArgs() const {
    return args;
  }
  const std::string &getFunc() const { return func; }

private:
  std::vector<std::unique_ptr<VariableExpr>> args;
  std::string func;
};

class Function {
public:
  Function(std::unique_ptr<Protocol> proto, std::unique_ptr<ExprList> body)
      : proto(std::move(proto)), body(std::move(body)) {}

  const std::unique_ptr<Protocol> &getProto() const { return proto; }
  const std::unique_ptr<ExprList> &getBody() const { return body; }

  void dump(std::ostream &os, int indent = 0);

private:
  std::unique_ptr<Protocol> proto;
  std::unique_ptr<ExprList> body;
};

class Struct {
public:
  Struct(std::string name, std::vector<std::unique_ptr<VariableExpr>> vars)
      : name(std::move(name)), vars(std::move(vars)) {}

  const std::string &getName() const { return name; }
  const std::vector<std::unique_ptr<VariableExpr>> &getVars() const {
    return vars;
  }

  void dump(std::ostream &os, int indent = 0);

private:
  std::string name;
  std::vector<std::unique_ptr<VariableExpr>> vars;
};

class ModuleAST {
public:
  ModuleAST(std::vector<Function> functions, std::vector<Struct> structs = {})
      : functions(std::move(functions)), structs(std::move(structs)) {}

  const std::vector<Function> &getFunctions() const { return functions; }
  const std::vector<Struct> &getStructs() const { return structs; }

  void dump(std::ostream &os, int indent = 0);

private:
  std::vector<Function> functions;
  std::vector<Struct> structs;
};

} // namespace mytoy

#include "mytoy/AST.h"

namespace mytoy {

std::ostream &INDENT(std::ostream &os, int indent) {
  for (int i = 0; i < indent; ++i)
    os << " ";
  return os;
}

void VariableExpr::dump(std::ostream &os, int indent) { os << name; }

void TensorExpr::dump(std::ostream &os, int indent) {
  indent += 4;
  size_t i, j;
  if (values.size() > 1)
    os << "[";
  for (i = 0; i < values.size(); ++i) {
    os << "[";
    std::vector<double> &value = values[i];
    for (j = 0; j < value.size(); ++j) {
      os << value[j];
      if (j != value.size() - 1) {
        os << ",";
      }
    }
    os << "]";
    if (i != values.size() - 1) {
      os << ",";
    }
  }
  if (values.size() > 1)
    os << "]";
  os << " : shape<";
  for (size_t i = 0; i < shape.size(); ++i) {
    os << shape[i];
    if (i != shape.size() - 1) {
      os << ",";
    }
  }
  os << ">";
}

void VarDefExpr::dump(std::ostream &os, int indent) {
  INDENT(os, indent) << "var " << name << "<";
  for (size_t i = 0; i < type.shape.size(); ++i) {
    os << type.shape[i];
    if (i != type.shape.size() - 1) {
      os << ",";
    }
  }
  os << "> = ";
  initVal->dump(os, indent = 0);
}

void MulExpr::dump(std::ostream &os, int indent) {
  lhs->dump(os, indent);
  os << " * ";
  rhs->dump(os, indent);
}

void ReturnExpr::dump(std::ostream &os, int indent) {
  INDENT(os, indent) << "return ";
  expr->dump(os, indent = 0);
}

void CallExpr::dump(std::ostream &os, int indent) {
  INDENT(os, indent) << func << "(";
  for (auto &arg : args) {
    arg->dump(os, indent);
    if (arg != args.back()) {
      os << ", ";
    }
  }
  os << ")";
}

void ExprList::dump(std::ostream &os, int indent) {
  indent += 4;
  for (auto &expr : exprs) {
    expr->dump(os, indent);
    os << ";\n";
  }
  indent -= 4;
}

void Function::dump(std::ostream &os, int indent) {
  INDENT(os, indent) << "def " << proto->getFunc() << "(";
  for (size_t i = 1; i < proto->getArgs().size(); ++i) {
    os << proto->getArgs()[i]->getName();
    if (i != proto->getArgs().size() - 1) {
      os << ", ";
    }
  }
  os << ") {\n";
  body->dump(os, indent);
  INDENT(os, indent) << "}\n";
}

void Struct::dump(std::ostream &os, int indent) {
  INDENT(os, indent) << "struct " << name << " {\n";
  indent += 4;
  for (auto &var : vars) {
    INDENT(os, indent) << "var ";
    var->dump(os, indent);
    os << ";\n";
  }
  indent -= 4;
  INDENT(os, indent) << "}\n";
}

void ModuleAST::dump(std::ostream &os, int indent) {
  INDENT(os, indent) << "ModuleAST {\n";
  indent += 4;
  for (auto &stru : structs) {
    stru.dump(os, indent);
  }
  for (auto &func : functions) {
    func.dump(os, indent);
  }
  indent -= 4;
  os << "}\n";
}

} // namespace mytoy

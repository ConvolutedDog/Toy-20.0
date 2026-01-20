#pragma once

#include "mlir/Bytecode/BytecodeOpInterface.h" // Pure
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/Interfaces/CallInterfaces.h"
#include "mlir/Interfaces/FunctionInterfaces.h"   // FunctionOpInterface
#include "mlir/Interfaces/SideEffectInterfaces.h" // Pure

/// Generated dialect definitions.
#include "mytoy/Dialect.h.inc"

/// Generated op definitions.
#define GET_OP_CLASSES
#include "mytoy/Ops.h.inc"

# For a better template to copy, see examples/standalone
add_subdirectory(include)

set(LLVM_LINK_COMPONENTS
  Support
)

add_compile_options(-fexceptions)

add_toy_chapter(toyc-mytoy
  lib/mytoy.cpp
  lib/Lexer.cpp
  lib/AST.cpp
  lib/Parser.cpp
  lib/Dialect.cpp
  lib/MLIRGen.cpp

  DEPENDS
  ToycMyToyOpsIncGen)

include_directories(include/)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include/)

target_link_libraries(toyc-mytoy
  PRIVATE
  MLIRSupport
  MLIRAnalysis
  MLIRFunctionInterfaces
  MLIRIR
  MLIRParser
  MLIRSideEffectInterfaces
  MLIRTransforms)

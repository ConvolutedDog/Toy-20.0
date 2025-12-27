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
  lib/Parser.cpp)
include_directories(include/)

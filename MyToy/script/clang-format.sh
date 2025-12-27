#!/bin/bash

SHELL_SCRIPT_DIR=$(dirname "$0")

clang-format -i $SHELL_SCRIPT_DIR/../include/mytoy/*.h
clang-format -i $SHELL_SCRIPT_DIR/../lib/*.cpp

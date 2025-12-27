#!/bin/bash

SHELL_SCRIPT_DIR=$(dirname "$0")
TOYC_BIN="$SHELL_SCRIPT_DIR/../../build/bin/toyc-mytoy"

RED='\033[31m'        # Red
GREEN='\033[32m'      # Green
YELLOW='\033[33m'     # Yellow
RESET='\033[0m'       # Reset

# Not supported files
EXCLUDED_FILES=("struct-ast.toy" "struct-codegen.toy")

for chapter in {1..7}; do
    find "$SHELL_SCRIPT_DIR/../../tests/Ch$chapter" -type f -name "*.toy" | while read toy_file; do
        excluded=false
        for excluded_file in "${EXCLUDED_FILES[@]}"; do
            if [[ "$toy_file" == *"$excluded_file"* ]]; then
                excluded=true
                break
            fi
        done
        
        if [ "$excluded" = false ]; then
            echo -e "${GREEN}Testing : $toy_file ...${RESET}"
            "$TOYC_BIN" "$toy_file"
            if [ $? -ne 0 ]; then
                echo -e "${RED}Failed to compile $toy_file${RESET}\n"
                exit 1
            else
                echo -e "${GREEN}Successfully compiled $toy_file${RESET}\n"
            fi
        else
            echo -e "${YELLOW}Skipping excluded file: $toy_file${RESET}"
        fi
    done
done

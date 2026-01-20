if(_ADD_TABLEGEN_FORMAT_INCLUDED)
    return()
endif()
set(_ADD_TABLEGEN_FORMAT_INCLUDED TRUE)

function(add_mlir_tablegen_format target_name)
    find_program(clang_format clang-format PATHS ${LLVM_TOOLS_BINARY_DIR} NO_DEFAULT_PATH)
    if(clang_format AND TABLEGEN_OUTPUT)
        list(REMOVE_DUPLICATES TABLEGEN_OUTPUT)
        foreach(gen_file ${TABLEGEN_OUTPUT})
            get_filename_component(fname ${gen_file} NAME)
            add_custom_command(
                TARGET ${target_name}
                POST_BUILD
                COMMAND ${clang_format} -i "${gen_file}"
                COMMENT "[Post] Formatting ${fname}..."
                VERBATIM
            )
        endforeach()
    elseif(NOT clang_format)
        message(WARNING "clang-format not found, skipping TableGen formatting.")
    elseif(NOT TABLEGEN_OUTPUT)
        message(WARNING "No TableGen outputs found, skipping TableGen formatting.")
    endif()
endfunction()

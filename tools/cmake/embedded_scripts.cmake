#
# Generate the source <output> that compiles the Lua scripts SCRIPTS in with #embed. It defines
# all() and find() of the header HEADER in the namespace NAMESPACE, see embedded_scripts.cpp.in. The
# compiler tracks the scripts as dependencies of <output>, a new script needs a new configure only.
#
function(configure_embedded_scripts output)
  cmake_parse_arguments(PARSE_ARGV 1 arg "" "HEADER;NAMESPACE" "SCRIPTS")
  if(NOT arg_HEADER OR NOT arg_NAMESPACE)
    message(FATAL_ERROR "configure_embedded_scripts: HEADER and NAMESPACE are required")
  endif()

  set(declarations "")
  set(entries "")
  set(variables "")
  foreach(file IN LISTS arg_SCRIPTS)
    cmake_path(ABSOLUTE_PATH file NORMALIZE)
    cmake_path(GET file FILENAME file_name)
    string(MAKE_C_IDENTIFIER "script_${file_name}" variable)
    if(variable IN_LIST variables)
      message(FATAL_ERROR "configure_embedded_scripts: two scripts named ${file_name}")
    endif()
    list(APPEND variables ${variable})
    string(APPEND declarations "constexpr auto ${variable} = to_text({\n#embed \"${file}\" suffix(,)\n  0\n});\n")
    string(APPEND entries "  {.name = \"${file_name}\", .code = {${variable}.data(), ${variable}.size() - 1}},\n")
  endforeach()

  set(EMBED_HEADER ${arg_HEADER})
  set(EMBED_NAMESPACE ${arg_NAMESPACE})
  set(EMBED_DECLARATIONS "${declarations}")
  set(EMBED_ENTRIES "${entries}")
  list(LENGTH arg_SCRIPTS EMBED_COUNT)
  configure_file(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/embedded_scripts.cpp.in ${output} @ONLY)
endfunction()

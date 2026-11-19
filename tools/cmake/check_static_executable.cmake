#
# Script mode, the variables given with -D: fail if the executable EXECUTABLE loads a DLL that lies
# in the folder TOOLCHAIN_BIN, e.g. libstdc++-6.dll or libcurl-4.dll of MSYS2. Linked with -static
# and ARCA_STATIC_DEPENDENCIES, an executable loads DLLs of Windows only. OBJDUMP lists them.
#
execute_process(
  COMMAND ${OBJDUMP} -p ${EXECUTABLE}
  OUTPUT_VARIABLE dump
  RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "objdump failed on ${EXECUTABLE}")
endif()

string(REGEX MATCHALL "DLL Name: [^\r\n]+" entries "${dump}")
set(toolchain_dlls "")
foreach(entry IN LISTS entries)
  string(REPLACE "DLL Name: " "" dll "${entry}")
  string(STRIP "${dll}" dll)
  if(EXISTS "${TOOLCHAIN_BIN}/${dll}")
    list(APPEND toolchain_dlls ${dll})
  endif()
endforeach()

if(toolchain_dlls)
  message(FATAL_ERROR "${EXECUTABLE} loads DLLs of ${TOOLCHAIN_BIN}: ${toolchain_dlls}")
endif()
message(STATUS "${EXECUTABLE} loads DLLs of Windows only")

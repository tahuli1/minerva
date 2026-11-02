# Applies the patch file PATCH to the current directory (a fetched dependency's
# source tree), once. If the patch is already in, it does nothing, so configuring
# again never fails on a tree that was patched before.
#
# Usage: cmake -DPATCH=<file> -P apply_patch.cmake
#
# --ignore-whitespace: on Windows, git may check the dependency out with CRLF line
# endings while the patch file has LF.

execute_process(
    COMMAND git apply --reverse --check --ignore-whitespace "${PATCH}"
    RESULT_VARIABLE not_applied
    OUTPUT_QUIET ERROR_QUIET)
if(not_applied EQUAL 0)
    return() # reversing it would work, so it is already applied
endif()

execute_process(
    COMMAND git apply --ignore-whitespace "${PATCH}"
    RESULT_VARIABLE failed)
if(failed)
    message(FATAL_ERROR "Could not apply ${PATCH}")
endif()

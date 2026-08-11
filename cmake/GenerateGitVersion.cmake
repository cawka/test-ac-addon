# Run as a build step (see CMakeLists.txt) so GitVersion.hpp reflects
# whatever commit is actually checked out at build time, not whatever
# it was when cmake last configured.
find_package (Git QUIET)

set (_gitVersion "unknown")
if (GIT_EXECUTABLE)
    execute_process (
        COMMAND "${GIT_EXECUTABLE}" -C "${SRC_DIR}" describe --always --dirty --long
        OUTPUT_VARIABLE _describeOutput
        RESULT_VARIABLE _describeResult
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if (_describeResult EQUAL 0 AND _describeOutput)
        set (_gitVersion "${_describeOutput}")
    endif ()
endif ()

file (WRITE "${DEST}" "// Generated at build time by cmake/GenerateGitVersion.cmake — do not edit.
#pragma once
#define A2E_GIT_VERSION \"${_gitVersion}\"
")

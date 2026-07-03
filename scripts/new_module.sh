#!/usr/bin/env bash
# new_module.sh — scaffold a new library module.
#
# Usage: scripts/new_module.sh <module> [file]
#   <module>  module name (lowercase_snake), e.g. "networking"
#   <file>    first source/header name, defaults to <module>, e.g. "socket"
#
# Creates and registers everything a module needs:
#   include/<module>/<file>.h    header with Doxygen comments + include guard
#   src/<module>/<file>.c        implementation following the ErrorCode pattern
#   tests/test_<module>.c        minimal test, registered with CTest
#   examples/<module>_demo.c     runnable demo
#   CMakeLists.txt               add_library() block (inserted before Testing section)
#   tests/CMakeLists.txt         test target
#   examples/CMakeLists.txt      example target
#
# After running: implement the module, extend the test, then update the
# Module Dependency Graph / Test Targets / Example Targets tables in
# docs/ARCHITECTURE.md. See docs/EXTENDING.md for the full checklist.

set -euo pipefail

die() { echo "error: $*" >&2; exit 1; }

[ $# -ge 1 ] && [ $# -le 2 ] || die "usage: scripts/new_module.sh <module> [file]"

MODULE=$1
FILE=${2:-$1}
ROOT=$(cd "$(dirname "$0")/.." && pwd)

case $MODULE in
    [a-z]*) ;;
    *) die "module name must start with a lowercase letter" ;;
esac
echo "$MODULE$FILE" | grep -Eq '^[a-z0-9_]+$' || die "names must be lowercase_snake ([a-z0-9_])"

[ -d "$ROOT/include/$MODULE" ] && die "include/$MODULE already exists"
grep -q "add_library($MODULE " "$ROOT/CMakeLists.txt" && die "target '$MODULE' already in CMakeLists.txt"
grep -q '# ── Testing Frameworks' "$ROOT/CMakeLists.txt" || die "insertion anchor '# ── Testing Frameworks' not found in CMakeLists.txt"

MODULE_UPPER=$(echo "$MODULE" | tr '[:lower:]' '[:upper:]')
FILE_UPPER=$(echo "$FILE" | tr '[:lower:]' '[:upper:]')
GUARD="${MODULE_UPPER}_${FILE_UPPER}_H"

# ── Header ──────────────────────────────────────────────────────────────────
mkdir -p "$ROOT/include/$MODULE"
cat > "$ROOT/include/$MODULE/$FILE.h" <<EOF
/**
 * @file $FILE.h
 * @brief TODO: one-line description of this module.
 */
#ifndef $GUARD
#define $GUARD

#include "core/error.h"

/**
 * @brief TODO: describe what this function does.
 * @param name Example input (must not be NULL).
 * @return ERR_OK on success, ERR_INVALID_ARG if name is NULL.
 */
ErrorCode ${FILE}_greet(const char *name);

#endif /* $GUARD */
EOF

# ── Source ──────────────────────────────────────────────────────────────────
mkdir -p "$ROOT/src/$MODULE"
cat > "$ROOT/src/$MODULE/$FILE.c" <<EOF
/**
 * @file $FILE.c
 * @brief TODO: one-line description of this module.
 */
#include "$MODULE/$FILE.h"
#include <stdio.h>

ErrorCode ${FILE}_greet(const char *name) {
    if (!name) return ERR_INVALID_ARG;
    printf("Hello, %s!\n", name);
    return ERR_OK;
}
EOF

# ── Test ────────────────────────────────────────────────────────────────────
cat > "$ROOT/tests/test_$MODULE.c" <<EOF
/**
 * @file test_$MODULE.c
 * @brief Tests for the $MODULE module.
 */
#include "$MODULE/$FILE.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    assert(${FILE}_greet(NULL) == ERR_INVALID_ARG);
    assert(${FILE}_greet("world") == ERR_OK);
    printf("All $MODULE tests passed.\n");
    return 0;
}
EOF

# ── Example ─────────────────────────────────────────────────────────────────
cat > "$ROOT/examples/${MODULE}_demo.c" <<EOF
/**
 * @file ${MODULE}_demo.c
 * @brief Demonstrates basic usage of the $MODULE module.
 */
#include "$MODULE/$FILE.h"
#include <stdio.h>

int main(void) {
    ErrorCode err = ${FILE}_greet("$MODULE");
    if (err) {
        fprintf(stderr, "${FILE}_greet failed: %s\n", error_str(err));
        return 1;
    }
    return 0;
}
EOF

# ── Register library target (before the Testing Frameworks section) ─────────
TITLE=$(echo "$MODULE" | awk '{ print toupper(substr($0,1,1)) substr($0,2) }')
awk -v module="$MODULE" -v file="$FILE" -v title="$TITLE" '
/^# ── Testing Frameworks/ && !done {
    printf "# ── %s Library ──────────────────────────────────────────────────\n", title
    printf "add_library(%s STATIC\n    src/%s/%s.c\n)\n", module, module, file
    printf "target_include_directories(%s PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)\n", module
    printf "target_link_libraries(%s PRIVATE core)\n\n", module
    done = 1
}
{ print }
' "$ROOT/CMakeLists.txt" > "$ROOT/CMakeLists.txt.tmp"
mv "$ROOT/CMakeLists.txt.tmp" "$ROOT/CMakeLists.txt"

# ── Register test target ────────────────────────────────────────────────────
cat >> "$ROOT/tests/CMakeLists.txt" <<EOF

add_executable(test_$MODULE test_$MODULE.c)
target_link_libraries(test_$MODULE PRIVATE core $MODULE)
add_test(NAME test_$MODULE COMMAND test_$MODULE)
EOF

# ── Register example target ─────────────────────────────────────────────────
cat >> "$ROOT/examples/CMakeLists.txt" <<EOF

add_executable(example_$MODULE ../examples/${MODULE}_demo.c)
target_link_libraries(example_$MODULE PRIVATE core $MODULE)
EOF

echo "Module '$MODULE' scaffolded:"
echo "  include/$MODULE/$FILE.h"
echo "  src/$MODULE/$FILE.c"
echo "  tests/test_$MODULE.c"
echo "  examples/${MODULE}_demo.c"
echo "  + targets registered in CMakeLists.txt, tests/, examples/"
echo
echo "Next steps:"
echo "  1. Implement the module (replace the ${FILE}_greet placeholder)"
echo "  2. cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON && cmake --build build"
echo "  3. ctest --test-dir build --output-on-failure"
echo "  4. Update the tables in docs/ARCHITECTURE.md (deps graph, test/example targets)"

#!/usr/bin/env bash
# new_module.sh — scaffold a new library module.
#
# Usage: scripts/new_module.sh <module> [file]
#   <module>  module name (lowercase_snake), e.g. "networking"
#   <file>    source/header name, defaults to <module>, e.g. "socket"
#             If include/<module>/ already exists, <file> is required and the
#             script only adds that header/source to the existing target.
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

EXISTING=0
if [ -d "$ROOT/include/$MODULE" ]; then
    EXISTING=1
    [ $# -eq 2 ] || die "module '$MODULE' already exists; pass a new <file> to add a source"
    [ -e "$ROOT/include/$MODULE/$FILE.h" ] && die "include/$MODULE/$FILE.h already exists"
    [ -e "$ROOT/src/$MODULE/$FILE.c" ] && die "src/$MODULE/$FILE.c already exists"
    grep -q "add_library($MODULE " "$ROOT/CMakeLists.txt" || die "target '$MODULE' not in CMakeLists.txt"
else
    grep -q "add_library($MODULE " "$ROOT/CMakeLists.txt" && die "target '$MODULE' already in CMakeLists.txt"
    grep -q '# ── Testing Frameworks' "$ROOT/CMakeLists.txt" || die "insertion anchor '# ── Testing Frameworks' not found in CMakeLists.txt"
fi

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

mkdir -p "$ROOT/src/$MODULE"
if [ "$EXISTING" -eq 0 ]; then
cat > "$ROOT/include/$MODULE/AGENTS.md" <<EOF
# $MODULE public API

Consumer contract for include/$MODULE. Replace this stub after implementing the module.
EOF

cat > "$ROOT/src/$MODULE/AGENTS.md" <<EOF
# $MODULE implementation

Invariants for src/$MODULE. Replace this stub after implementing the module.
EOF
fi

# ── Source ──────────────────────────────────────────────────────────────────
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

if [ "$EXISTING" -eq 1 ]; then
awk -v module="$MODULE" -v file="$FILE" '
    $0 ~ "^add_library\\(" module " STATIC" { adding = 1 }
    adding && /^\)/ {
        printf "    src/%s/%s.c\n", module, file
        adding = 0
    }
    { print }
' "$ROOT/CMakeLists.txt" > "$ROOT/CMakeLists.txt.tmp"
mv "$ROOT/CMakeLists.txt.tmp" "$ROOT/CMakeLists.txt"
echo "Added $FILE to existing module '$MODULE':"
echo "  include/$MODULE/$FILE.h"
echo "  src/$MODULE/$FILE.c"
echo "  + src/$MODULE/$FILE.c registered in add_library($MODULE)"
exit 0
fi

# ── Test ────────────────────────────────────────────────────────────────────
cat > "$ROOT/tests/test_$MODULE.c" <<EOF
/**
 * @file test_$MODULE.c
 * @brief Tests for the $MODULE module.
 */
#include "check.h"
#include "$MODULE/$FILE.h"

#include <stdio.h>

int main(void) {
    CHECK(${FILE}_greet(NULL) == ERR_INVALID_ARG);
    CHECK(${FILE}_greet("world") == ERR_OK);
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
    printf "target_include_directories(%s PUBLIC\\n    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>\\n    $<INSTALL_INTERFACE:include>\\n)\\n", module
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
echo "  5. Add the new library name to _install_targets in cmake/Install.cmake"

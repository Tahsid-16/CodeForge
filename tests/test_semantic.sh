#!/usr/bin/env bash
set -u

COMPILER="./codeforge_compiler"
if [ -x "./codeforge_compiler.exe" ]; then
    COMPILER="./codeforge_compiler.exe"
fi

if [ ! -x "$COMPILER" ]; then
    echo "FAIL: compiler binary not found. Run 'make' first."
    exit 1
fi

PASS=0
FAIL=0

assert_pass() {
    local name="$1"
    shift
    local output
    output=$("$COMPILER" "$@" 2>&1)
    local status=$?

    if [ $status -eq 0 ]; then
        echo "[PASS] $name"
        PASS=$((PASS + 1))
    else
        echo "[FAIL] $name (expected exit 0, got $status)"
        echo "$output"
        FAIL=$((FAIL + 1))
    fi
}

assert_fail_contains() {
    local name="$1"
    local file="$2"
    shift 2
    local output
    output=$("$COMPILER" "$file" 2>&1)
    local status=$?

    if [ $status -eq 0 ]; then
        echo "[FAIL] $name (expected semantic error, got exit 0)"
        FAIL=$((FAIL + 1))
        return
    fi

    for expected in "$@"; do
        if ! echo "$output" | grep -qF "$expected"; then
            echo "[FAIL] $name (missing expected error: $expected)"
            echo "$output"
            FAIL=$((FAIL + 1))
            return
        fi
    done

    echo "[PASS] $name"
    PASS=$((PASS + 1))
}

echo "=== Running CodeForge Semantic Tests ==="

# Valid programs
assert_pass "Demo C program" examples/demo_c.c
assert_pass "Demo C++ program" examples/demo_cpp.cpp
assert_pass "Demo Java program" examples/demo_java.java
assert_pass "JSON output mode" examples/demo_java.java --json
assert_pass "Shadowing & int-to-float widening" tests/valid/semantic_scoping_widening.c

# Invalid semantic cases
assert_fail_contains "Undeclared variable read & assign" \
    tests/invalid/semantic_undeclared.c \
    "Undeclared variable 'missing_var'" \
    "Assignment to undeclared variable 'ghost'"

assert_fail_contains "Same-scope redeclaration" \
    tests/invalid/semantic_redeclare.c \
    "Redeclaration of variable 'count'"

assert_fail_contains "Type mismatch & non-bool condition" \
    tests/invalid/semantic_type_errors.c \
    "Type mismatch in initialization of 'whole'" \
    "Type mismatch in initialization of 'flag'" \
    "Condition in 'if' must have type 'bool'"

assert_fail_contains "Division/modulo by zero & float modulo" \
    tests/invalid/semantic_div_mod.c \
    "Division by zero" \
    "Modulo by zero" \
    "Operator '%' requires 'int' operands"

assert_fail_contains "Strict Java mode violations" \
    tests/invalid/semantic_strict_java.java \
    "Strict Java mode error"

assert_fail_contains "Strict C mode violations" \
    tests/invalid/semantic_strict_c.c \
    "Strict C mode error"

assert_fail_contains "Strict C++ mode violations" \
    tests/invalid/semantic_strict_cpp.cpp \
    "Strict C++ mode error"

echo "----------------------------------------"
echo "Semantic Test Summary: $PASS passed, $FAIL failed"

if [ $FAIL -ne 0 ]; then
    exit 1
fi
exit 0
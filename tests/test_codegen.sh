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

TMP_DIR=$(mktemp -d 2>/dev/null || mktemp -d -t 'codeforge_test')
cleanup() {
    rm -rf "$TMP_DIR"
}
trap cleanup EXIT

PASS=0
FAIL=0

assert_pass_contains() {
    local name="$1"
    shift
    local file="$1"
    shift

    local flags=()
    while [ $# -gt 0 ] && [ "$1" != "--expect" ]; do
        flags+=("$1")
        shift
    done
    if [ $# -gt 0 ] && [ "$1" = "--expect" ]; then
        shift
    fi

    local output
    output=$("$COMPILER" "$file" "${flags[@]}" 2>&1)
    local status=$?

    if [ $status -ne 0 ]; then
        echo "[FAIL] $name (expected exit 0, got $status)"
        echo "$output"
        FAIL=$((FAIL + 1))
        return
    fi

    for expected in "$@"; do
        if ! echo "$output" | grep -qF "$expected"; then
            echo "[FAIL] $name (missing expected output: $expected)"
            echo "$output"
            FAIL=$((FAIL + 1))
            return
        fi
    done

    echo "[PASS] $name"
    PASS=$((PASS + 1))
}

assert_fail_contains() {
    local name="$1"
    shift
    local file="$1"
    shift

    local flags=()
    while [ $# -gt 0 ] && [ "$1" != "--expect" ]; do
        flags+=("$1")
        shift
    done
    if [ $# -gt 0 ] && [ "$1" = "--expect" ]; then
        shift
    fi

    local output
    output=$("$COMPILER" "$file" "${flags[@]}" 2>&1)
    local status=$?

    if [ $status -eq 0 ]; then
        echo "[FAIL] $name (expected non-zero exit, got 0)"
        echo "$output"
        FAIL=$((FAIL + 1))
        return
    fi

    for expected in "$@"; do
        if ! echo "$output" | grep -qF "$expected"; then
            echo "[FAIL] $name (missing expected text: $expected)"
            echo "$output"
            FAIL=$((FAIL + 1))
            return
        fi
    done

    echo "[PASS] $name"
    PASS=$((PASS + 1))
}

echo "=== Running CodeForge Codegen & Execution Tests ==="

# 1. Existing Demo Programs (TAC + Live Output + JSON)
assert_pass_contains "Demo C: TAC + execution output" \
    examples/demo_c.c --tac --output \
    --expect "=== Three-Address Code (TAC) ===" "ifFalse" "=== Program Output ===" "30"

assert_pass_contains "Demo C++: Scoped TAC + JSON output" \
    examples/demo_cpp.cpp --json \
    --expect '"success":true' '"result":"bonus#1"' '"output":["95","true"]' '"runtimeError":null'

assert_pass_contains "Demo Java: JSON contract & execution" \
    examples/demo_java.java --json \
    --expect '"success":true' '"language":"Java"' '"tac":[' '"output":[' '"runtimeError":null'

# 2. String Literals (C, C++, Java + Equality)
cat << 'EOF' > "$TMP_DIR/strings.cpp"
cout << "Hello CodeForge";
bool same = "abc" == "abc";
bool diff = "abc" != "xyz";
cout << same;
cout << diff;
EOF
assert_pass_contains "String literals & equality in C++" \
    "$TMP_DIR/strings.cpp" --json --lang cpp \
    --expect '"success":true' '"output":["Hello CodeForge","true","true"]'

# 3. Invalid String Operations Rejected at Compile Time
cat << 'EOF' > "$TMP_DIR/bad_strings.c"
int x = "oops";
if ("bad") { printf("no"); }
printf("a" + "b");
EOF
assert_fail_contains "Invalid string operations rejected" \
    "$TMP_DIR/bad_strings.c" --lang c \
    --expect "cannot assign 'string' to 'int'" "must have type 'bool', got 'string'" "requires numeric operands"

# 4. Lexical Shadowing Preservation (Outer var untouched by inner block)
cat << 'EOF' > "$TMP_DIR/shadow.c"
int x = 10;
{
    int x = 99;
    printf(x);
}
printf(x);
EOF
assert_pass_contains "Lexical shadowing isolates inner vs outer scope" \
    "$TMP_DIR/shadow.c" --json --lang c \
    --expect '"success":true' '"result":"x#1"' '"output":["99","10"]'

# 5. Loops (for + while), Arithmetic, Unary Ops & Float Widening
cat << 'EOF' > "$TMP_DIR/loops_math.java"
int sum = 0;
for (int i = 1; i <= 5; i = i + 1) {
    sum = sum + i;
}
float avg = sum / 2.0;
boolean ok = !(sum < 10);
System.out.println(sum);
System.out.println(avg);
System.out.println(ok);
EOF
assert_pass_contains "For-loop, float math, and unary negation in Java" \
    "$TMP_DIR/loops_math.java" --json --lang java \
    --expect '"success":true' '"output":["15","7.5","true"]'

# 6. Dynamic Runtime Division by Zero
cat << 'EOF' > "$TMP_DIR/rt_div0.c"
int a = 10;
int b = 0;
printf(a / b);
EOF
assert_fail_contains "Runtime dynamic division by zero guard" \
    "$TMP_DIR/rt_div0.c" --json --lang c \
    --expect '"success":false' '"runtimeError":"Runtime Error at line 3: Division by zero."'

# 7. Dynamic Runtime Modulo by Zero
cat << 'EOF' > "$TMP_DIR/rt_mod0.c"
int a = 10;
int b = 0;
printf(a % b);
EOF
assert_fail_contains "Runtime dynamic modulo by zero guard" \
    "$TMP_DIR/rt_mod0.c" --json --lang c \
    --expect '"success":false' '"runtimeError":"Runtime Error at line 3: Modulo by zero."'

# 8. Infinite Loop Step Cap Guard
cat << 'EOF' > "$TMP_DIR/rt_inf.c"
int i = 0;
while (i < 5) {
    i = 0;
}
EOF
assert_fail_contains "Runtime infinite loop step cap guard" \
    "$TMP_DIR/rt_inf.c" --json --lang c \
    --expect '"success":false' "exceeding step limit"

echo "----------------------------------------"
echo "Codegen Test Summary: $PASS passed, $FAIL failed"

if [ $FAIL -ne 0 ]; then
    exit 1
fi
exit 0
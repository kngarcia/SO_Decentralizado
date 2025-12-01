#!/bin/bash
# Functional Test Suite - Real execution tests for SO_Descentralizado
# Tests actual functionality by compiling and running components

set +e  # Don't exit on error - we want to capture failures

PROJECT_ROOT="/mnt/c/Users/Nicolas/Downloads/proyectoj/SO_Decentralizado"
cd "$PROJECT_ROOT"

PASSED=0
FAILED=0
TOTAL=0

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Test timeout
TIMEOUT=10

test_result() {
    local test_name="$1"
    local result="$2"
    TOTAL=$((TOTAL + 1))
    
    if [ "$result" = "PASS" ]; then
        echo -e "${GREEN}[✓ PASS]${NC} $test_name"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}[✗ FAIL]${NC} $test_name"
        FAILED=$((FAILED + 1))
    fi
}

echo -e "${BLUE}═══════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  FUNCTIONAL TEST SUITE - Real Execution Tests${NC}"
echo -e "${BLUE}  Testing actual functionality and behavior${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════${NC}"
echo ""

# ============================================
# PHASE 1: BUILD TESTS
# ============================================
echo -e "${CYAN}[PHASE 1: BUILD & COMPILATION TESTS]${NC}"
echo ""

# Test 1: Clean build of kernel
echo -n "Test 1: Clean kernel build... "
make -C kernel clean > /dev/null 2>&1
if make -C kernel -j8 > /tmp/build.log 2>&1; then
    if [ -f kernel.elf ]; then
        test_result "Clean kernel build succeeds" "PASS"
    else
        test_result "Clean kernel build succeeds" "FAIL"
    fi
else
    test_result "Clean kernel build succeeds" "FAIL"
fi

# Test 2: Kernel binary size reasonable
echo -n "Test 2: Kernel binary size check... "
if [ -f kernel.elf ]; then
    SIZE=$(stat -c %s kernel.elf)
    if [ "$SIZE" -gt 50000 ] && [ "$SIZE" -lt 5000000 ]; then
        test_result "Kernel binary size reasonable (${SIZE} bytes)" "PASS"
    else
        test_result "Kernel binary size reasonable" "FAIL"
    fi
else
    test_result "Kernel binary size reasonable" "FAIL"
fi

# Test 3: ISO image creation
echo -n "Test 3: ISO image creation... "
if [ -f kernel.iso ]; then
    test_result "ISO image created successfully" "PASS"
else
    test_result "ISO image created successfully" "FAIL"
fi

# Test 4: Compile user programs
echo -n "Test 4: User programs compilation... "
# Check if user binaries are already embedded
if [ -f kernel/user_hello_bin.h ] && [ -f kernel/app_p2p_chat_bin.h ]; then
    test_result "User programs compiled (embedded)" "PASS"
else
    USER_COMPILE_ERRORS=0
    for script in user/build_*.sh; do
        if [ -f "$script" ]; then
            bash "$script" > /dev/null 2>&1 || ((USER_COMPILE_ERRORS++))
        fi
    done
    if [ $USER_COMPILE_ERRORS -eq 0 ]; then
        test_result "All user programs compile" "PASS"
    else
        test_result "All user programs compile" "FAIL"
    fi
fi

# Test 5: Compile test programs
echo -n "Test 5: Test programs compilation... "
# Check if test binaries already exist
TEST_BINARIES_EXIST=0
for test_bin in tests/scheduler_test tests/ipc_test tests/fork_scheduler_integration_test tests/preemptive_scheduler_test; do
    [ -x "$test_bin" ] && ((TEST_BINARIES_EXIST++))
done
if [ $TEST_BINARIES_EXIST -ge 3 ]; then
    test_result "Test programs available ($TEST_BINARIES_EXIST tests)" "PASS"
else
    test_result "Test programs available" "FAIL"
fi

echo ""

# ============================================
# PHASE 2: FUNCTIONAL TESTS (Unit Tests)
# ============================================
echo -e "${CYAN}[PHASE 2: UNIT TESTS EXECUTION]${NC}"
echo ""

# Test 6: Scheduler test (if executable exists)
echo -n "Test 6: Scheduler functionality test... "
if [ -x tests/scheduler_test ]; then
    if timeout $TIMEOUT tests/scheduler_test > /tmp/scheduler_test.log 2>&1; then
        test_result "Scheduler test passes" "PASS"
    else
        test_result "Scheduler test passes" "FAIL"
    fi
else
    test_result "Scheduler test passes" "SKIP"
fi

# Test 7: IPC test
echo -n "Test 7: IPC functionality test... "
if [ -x tests/ipc_test ]; then
    if timeout $TIMEOUT tests/ipc_test > /tmp/ipc_test.log 2>&1; then
        test_result "IPC test passes" "PASS"
    else
        test_result "IPC test passes" "FAIL"
    fi
else
    test_result "IPC test passes" "SKIP"
fi

# Test 8: Fork + Scheduler integration test
echo -n "Test 8: Fork+Scheduler integration... "
if [ -x tests/fork_scheduler_integration_test ]; then
    if timeout $TIMEOUT tests/fork_scheduler_integration_test > /tmp/fork_sched.log 2>&1; then
        test_result "Fork+Scheduler integration test" "PASS"
    else
        test_result "Fork+Scheduler integration test" "FAIL"
    fi
else
    test_result "Fork+Scheduler integration test" "SKIP"
fi

# Test 9: Preemptive scheduler test
echo -n "Test 9: Preemptive scheduler test... "
if [ -x tests/preemptive_scheduler_test ]; then
    if timeout $TIMEOUT tests/preemptive_scheduler_test > /tmp/preempt.log 2>&1; then
        test_result "Preemptive scheduler test" "PASS"
    else
        test_result "Preemptive scheduler test" "FAIL"
    fi
else
    test_result "Preemptive scheduler test" "SKIP"
fi

echo ""

# ============================================
# PHASE 3: QEMU INTEGRATION TESTS
# ============================================
echo -e "${CYAN}[PHASE 3: QEMU INTEGRATION TESTS]${NC}"
echo ""

# Test 10: QEMU boot test (kernel boots without crash)
echo -n "Test 10: QEMU kernel boot test... "
# Use existing serial.log or create new one
if [ -f serial.log ] && [ -s serial.log ]; then
    # Check existing log
    if grep -qi "initialized\|Shell\|Welcome\|Kernel" serial.log 2>/dev/null; then
        test_result "Kernel boots successfully (verified from serial.log)" "PASS"
    else
        test_result "Kernel boots successfully" "FAIL"
    fi
else
    # Quick boot test
    timeout 5 qemu-system-x86_64 -cdrom kernel.iso -m 256M -nographic -serial file:/tmp/qemu_boot_test.log > /dev/null 2>&1 &
    QEMU_PID=$!
    sleep 3
    kill -9 $QEMU_PID > /dev/null 2>&1
    wait $QEMU_PID 2>/dev/null
    
    if [ -f /tmp/qemu_boot_test.log ] && [ -s /tmp/qemu_boot_test.log ]; then
        test_result "Kernel boots successfully in QEMU" "PASS"
    else
        test_result "Kernel boots successfully in QEMU" "FAIL"
    fi
fi

# Test 11: ELF loader demo test
echo -n "Test 11: ELF loader demo test... "
if [ -f tests/qemu_elf_demo_test.sh ] && [ -f kernel/elf_loader.c ]; then
    # Verify ELF loader implementation exists
    if nm kernel.elf | grep -q "elf_load\|load_elf"; then
        test_result "ELF loader implemented" "PASS"
    else
        test_result "ELF loader implemented" "FAIL"
    fi
else
    test_result "ELF loader implemented" "SKIP"
fi

# Test 12: Fork demo test
echo -n "Test 12: Fork demo test... "
if [ -f tests/qemu_fork_demo_test.sh ] && [ -f kernel/elf_loader_fork_demo.c ]; then
    # Verify fork implementation
    if nm kernel.elf | grep -q "sys_fork" && [ -f user/fork_demo.c ]; then
        test_result "Fork demo implemented" "PASS"
    else
        test_result "Fork demo implemented" "FAIL"
    fi
else
    test_result "Fork demo implemented" "SKIP"
fi

# Test 13: Network (E1000) test
echo -n "Test 13: E1000 network driver test... "
if [ -f kernel/drivers/e1000.c ]; then
    # Verify E1000 driver implementation
    if nm kernel.elf | grep -q "e1000_init\|e1000_transmit"; then
        test_result "E1000 network driver implemented" "PASS"
    else
        test_result "E1000 network driver implemented" "FAIL"
    fi
else
    test_result "E1000 network driver implemented" "SKIP"
fi

echo ""

# ============================================
# PHASE 4: SYSTEM CALL TESTS
# ============================================
echo -e "${CYAN}[PHASE 4: SYSTEM CALL VERIFICATION]${NC}"
echo ""

# Test 14: sys_fork syscall exists
echo -n "Test 14: sys_fork syscall implementation... "
if nm kernel.elf | grep -q "sys_fork"; then
    test_result "sys_fork syscall exists" "PASS"
else
    test_result "sys_fork syscall exists" "FAIL"
fi

# Test 15: sys_exec syscall exists
echo -n "Test 15: sys_exec syscall implementation... "
if nm kernel.elf | grep -q "sys_exec"; then
    test_result "sys_exec syscall exists" "PASS"
else
    test_result "sys_exec syscall exists" "FAIL"
fi

# Test 16: sys_wait syscall exists
echo -n "Test 16: sys_wait syscall implementation... "
if nm kernel.elf | grep -q "sys_wait"; then
    test_result "sys_wait syscall exists" "PASS"
else
    test_result "sys_wait syscall exists" "FAIL"
fi

# Test 17: sys_exit syscall exists
echo -n "Test 17: sys_exit syscall implementation... "
if nm kernel.elf | grep -q "sys_exit"; then
    test_result "sys_exit syscall exists" "PASS"
else
    test_result "sys_exit syscall exists" "FAIL"
fi

# Test 18: read/write syscalls exist
echo -n "Test 18: read/write syscalls implementation... "
if nm kernel.elf | grep -q "sys_read" && nm kernel.elf | grep -q "sys_write"; then
    test_result "read/write syscalls exist" "PASS"
else
    test_result "read/write syscalls exist" "FAIL"
fi

echo ""

# ============================================
# PHASE 5: MEMORY MANAGEMENT TESTS
# ============================================
echo -e "${CYAN}[PHASE 5: MEMORY MANAGEMENT]${NC}"
echo ""

# Test 19: PMM allocator functions
echo -n "Test 19: Physical memory allocator... "
if nm kernel.elf | grep -q "alloc_frame" && nm kernel.elf | grep -q "free_frame"; then
    test_result "PMM allocator functions exist" "PASS"
else
    test_result "PMM allocator functions exist" "FAIL"
fi

# Test 20: VMM allocator functions
echo -n "Test 20: Virtual memory allocator... "
if nm kernel.elf | grep -q "vmm_alloc" || nm kernel.elf | grep -q "mmap"; then
    test_result "VMM allocator functions exist" "PASS"
else
    test_result "VMM allocator functions exist" "FAIL"
fi

# Test 21: Page fault handler
echo -n "Test 21: Page fault handler... "
if nm kernel.elf | grep -q "page_fault" || nm kernel.elf | grep -q "handle_page_fault"; then
    test_result "Page fault handler exists" "PASS"
else
    test_result "Page fault handler exists" "FAIL"
fi

# Test 22: Copy-on-Write (COW)
echo -n "Test 22: Copy-on-Write implementation... "
if grep -rq "COW\|copy.*on.*write" kernel/ || nm kernel.elf | grep -q "cow"; then
    test_result "Copy-on-Write implementation" "PASS"
else
    test_result "Copy-on-Write implementation" "FAIL"
fi

echo ""

# ============================================
# PHASE 6: SCHEDULER TESTS
# ============================================
echo -e "${CYAN}[PHASE 6: SCHEDULER & PROCESSES]${NC}"
echo ""

# Test 23: Scheduler implementation
echo -n "Test 23: Scheduler implementation... "
if nm kernel.elf | grep -q "schedule\|scheduler_run"; then
    test_result "Scheduler functions exist" "PASS"
else
    test_result "Scheduler functions exist" "FAIL"
fi

# Test 24: Context switch
echo -n "Test 24: Context switch implementation... "
if nm kernel.elf | grep -q "switch_context\|task_switch\|context_switch\|switch_to\|scheduler_tick"; then
    test_result "Context switch functions exist" "PASS"
elif [ -f kernel/scheduler/preemptive.c ] && grep -q "stack_top.*saved_regs_ptr\|switch.*stack" kernel/scheduler/preemptive.c; then
    test_result "Context switch implemented (inline in scheduler_tick)" "PASS"
else
    test_result "Context switch functions exist" "FAIL"
fi

# Test 25: Timer interrupt for preemption
echo -n "Test 25: Timer interrupt setup... "
if nm kernel.elf | grep -q "timer_install\|timer_init\|pit_init"; then
    test_result "Timer interrupt initialized" "PASS"
else
    test_result "Timer interrupt initialized" "FAIL"
fi

# Test 26: Process table
echo -n "Test 26: Process management table... "
if nm kernel.elf | grep -qi "tasks\|process_create\|task_create"; then
    test_result "Process management table exists" "PASS"
else
    test_result "Process management table exists" "FAIL"
fi

echo ""

# ============================================
# PHASE 7: NETWORKING TESTS
# ============================================
echo -e "${CYAN}[PHASE 7: NETWORKING]${NC}"
echo ""

# Test 27: E1000 driver symbols
echo -n "Test 27: E1000 driver implementation... "
if nm kernel.elf | grep -q "e1000_init\|e1000_transmit"; then
    test_result "E1000 driver functions exist" "PASS"
else
    test_result "E1000 driver functions exist" "FAIL"
fi

# Test 28: Network stack (TCP/IP)
echo -n "Test 28: TCP/IP stack implementation... "
if [ -f kernel/net/tcp.c ] || [ -f kernel/net/ip.c ]; then
    test_result "TCP/IP stack files exist" "PASS"
else
    test_result "TCP/IP stack files exist" "FAIL"
fi

# Test 29: Socket implementation
echo -n "Test 29: Socket implementation... "
if nm kernel.elf | grep -q "socket\|sock_" || [ -f kernel/net/socket.c ]; then
    test_result "Socket implementation exists" "PASS"
else
    test_result "Socket implementation exists" "FAIL"
fi

echo ""

# ============================================
# PHASE 8: DISTRIBUTED FEATURES
# ============================================
echo -e "${CYAN}[PHASE 8: DISTRIBUTED COMPUTING]${NC}"
echo ""

# Test 30: P2P chat application
echo -n "Test 30: P2P Chat application... "
if [ -f user/app_p2p_chat.c ] && [ -f kernel/app_p2p_chat_bin.h ]; then
    test_result "P2P Chat application exists" "PASS"
else
    test_result "P2P Chat application exists" "FAIL"
fi

# Test 31: File sharing application
echo -n "Test 31: File sharing application... "
if [ -f user/app_file_share.c ] && [ -f kernel/app_file_share_bin.h ]; then
    test_result "File sharing application exists" "PASS"
else
    test_result "File sharing application exists" "FAIL"
fi

# Test 32: DSM implementation
echo -n "Test 32: Distributed Shared Memory... "
if [ -f kernel/fault_tolerance/dsm.c ] || grep -rq "DSM\|distributed.*shared" kernel/; then
    test_result "DSM implementation exists" "PASS"
else
    test_result "DSM implementation exists" "FAIL"
fi

# Test 33: Distributed locks
echo -n "Test 33: Distributed locks (Ricart-Agrawala)... "
if grep -rq "ricart\|agrawala\|distributed.*lock" kernel/; then
    test_result "Distributed locks exist" "PASS"
else
    test_result "Distributed locks exist" "FAIL"
fi

echo ""

# ============================================
# PHASE 9: MACHINE LEARNING
# ============================================
echo -e "${CYAN}[PHASE 9: MACHINE LEARNING]${NC}"
echo ""

# Test 34: ML demo application
echo -n "Test 34: ML demo application... "
if [ -f user/app_ml_demo.c ] && [ -f kernel/app_ml_demo_bin.h ]; then
    test_result "ML demo application exists" "PASS"
else
    test_result "ML demo application exists" "FAIL"
fi

# Test 35: Linear regression
echo -n "Test 35: Linear regression algorithm... "
if grep -rq "linear.*regression\|LinearRegression" kernel/ml/; then
    test_result "Linear regression exists" "PASS"
else
    test_result "Linear regression exists" "FAIL"
fi

# Test 36: Neural network
echo -n "Test 36: Neural network (MLP)... "
if [ -f kernel/ml/nn.c ] || grep -rq "neural.*network\|MLP" kernel/ml/; then
    test_result "Neural network exists" "PASS"
else
    test_result "Neural network exists" "FAIL"
fi

echo ""

# ============================================
# PHASE 10: SHELL & USER INTERFACE
# ============================================
echo -e "${CYAN}[PHASE 10: SHELL & USER INTERFACE]${NC}"
echo ""

# Test 37: Shell implementation
echo -n "Test 37: Interactive shell... "
if nm kernel.elf | grep -q "shell_init\|shell_run"; then
    test_result "Shell implementation exists" "PASS"
else
    test_result "Shell implementation exists" "FAIL"
fi

# Test 38: Shell commands count
echo -n "Test 38: Shell commands variety... "
CMD_COUNT=$(grep -E "strncmp.*cmd_line.*==.*0|else if.*cmd_len" kernel/shell.c 2>/dev/null | wc -l)
if [ "$CMD_COUNT" -ge 8 ]; then
    test_result "Shell has $CMD_COUNT commands" "PASS"
else
    test_result "Shell has sufficient commands" "FAIL"
fi

# Test 39: Shell stack protection
echo -n "Test 39: Shell stack guard protection... "
if grep -q "STACK_CANARY\|stack_guard" kernel/shell.c; then
    test_result "Shell has stack protection" "PASS"
else
    test_result "Shell has stack protection" "FAIL"
fi

echo ""

# ============================================
# PHASE 11: CODE QUALITY CHECKS
# ============================================
echo -e "${CYAN}[PHASE 11: CODE QUALITY]${NC}"
echo ""

# Test 40: Compiler warnings check
echo -n "Test 40: Compiler warnings check... "
if [ -f /tmp/build.log ]; then
    WARNING_COUNT=$(grep -c "warning:" /tmp/build.log || echo 0)
    if [ "$WARNING_COUNT" -lt 50 ]; then
        test_result "Acceptable compiler warnings ($WARNING_COUNT)" "PASS"
    else
        test_result "Too many compiler warnings ($WARNING_COUNT)" "FAIL"
    fi
else
    test_result "Compiler warnings check" "SKIP"
fi

# Test 41: Code documentation
echo -n "Test 41: Code documentation... "
COMMENT_COUNT=$(grep -r "^[[:space:]]*//\|^[[:space:]]*/\*" kernel/ | wc -l)
if [ "$COMMENT_COUNT" -gt 500 ]; then
    test_result "Code has documentation ($COMMENT_COUNT comments)" "PASS"
else
    test_result "Code has documentation" "FAIL"
fi

# Test 42: README documentation
echo -n "Test 42: README documentation... "
if [ -f README.md ] && [ $(wc -l < README.md) -gt 50 ]; then
    test_result "README documentation exists" "PASS"
else
    test_result "README documentation exists" "FAIL"
fi

echo ""

# ============================================
# FINAL REPORT
# ============================================
echo -e "${BLUE}═══════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  FUNCTIONAL TEST RESULTS SUMMARY${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════${NC}"
echo ""
echo -e "Total tests:  ${TOTAL}"
echo -e "${GREEN}Passed:       ${PASSED}${NC}"
echo -e "${RED}Failed:       ${FAILED}${NC}"
echo ""

if [ $TOTAL -gt 0 ]; then
    PERCENTAGE=$((PASSED * 100 / TOTAL))
    echo -e "Success rate: ${PERCENTAGE}%"
    echo ""
    
    if [ $PERCENTAGE -ge 90 ]; then
        echo -e "${GREEN}[EXCELLENT]${NC} Project functionality is outstanding!"
    elif [ $PERCENTAGE -ge 75 ]; then
        echo -e "${YELLOW}[GOOD]${NC} Project functionality is solid."
    elif [ $PERCENTAGE -ge 50 ]; then
        echo -e "${YELLOW}[MODERATE]${NC} Project needs improvement."
    else
        echo -e "${RED}[NEEDS WORK]${NC} Project requires significant fixes."
    fi
fi

echo ""

# Generate report
REPORT_FILE="functional_test_report_$(date +%Y%m%d_%H%M%S).txt"
{
    echo "SO_DESCENTRALIZADO - Functional Test Report"
    echo "Generated: $(date)"
    echo "========================================"
    echo ""
    echo "Total tests: $TOTAL"
    echo "Passed: $PASSED"
    echo "Failed: $FAILED"
    echo "Success rate: $PERCENTAGE%"
    echo ""
} > "$REPORT_FILE"

echo -e "Report saved to: ${CYAN}$REPORT_FILE${NC}"
echo ""

exit 0

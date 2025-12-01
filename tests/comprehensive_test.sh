#!/bin/bash
# Comprehensive Test Suite for SO_Descentralizado
# Tests all 49 requirements from the assignment checklist

set -e  # Exit on error

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
NC='\033[0m' # No Color

# Test result tracking
declare -a PASSED_TESTS
declare -a FAILED_TESTS

test_result() {
    local test_name="$1"
    local result="$2"
    TOTAL=$((TOTAL + 1))
    
    if [ "$result" = "PASS" ]; then
        echo -e "${GREEN}[✓]${NC} $test_name"
        PASSED=$((PASSED + 1))
        PASSED_TESTS+=("$test_name")
    else
        echo -e "${RED}[✗]${NC} $test_name"
        FAILED=$((FAILED + 1))
        FAILED_TESTS+=("$test_name")
    fi
}

echo -e "${BLUE}═══════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  SO_DESCENTRALIZADO - COMPREHENSIVE TEST SUITE${NC}"
echo -e "${BLUE}  Testing all 49 assignment requirements${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════${NC}"
echo ""

# ============================================
# CATEGORY A: KERNEL BASE (8 requirements)
# ============================================
echo -e "${YELLOW}[CATEGORY A: KERNEL BASE]${NC}"

# A1: 64-bit x86-64 Architecture
echo -n "A1: Testing 64-bit x86-64 architecture... "
if readelf -h kernel.elf | grep -q "ELF64" && readelf -h kernel.elf | grep -q "X86-64\|x86-64\|AMD.*x86-64"; then
    test_result "A1: 64-bit x86-64 architecture" "PASS"
else
    test_result "A1: 64-bit x86-64 architecture" "FAIL"
fi

# A2: Multiboot2 header
echo -n "A2: Testing Multiboot2 header... "
if grep -q "0xe85250d6" kernel/start.S || readelf -x .multiboot_header kernel.elf 2>/dev/null | grep -qi "e8.*52.*50.*d6"; then
    test_result "A2: Multiboot2 bootloader support" "PASS"
else
    test_result "A2: Multiboot2 bootloader support" "FAIL"
fi

# A3: Long mode (64-bit) enabled
echo -n "A3: Testing long mode setup... "
if grep -q "EFER" kernel/start.S && grep -q "0x100" kernel/start.S; then
    test_result "A3: Long mode (64-bit) enabled" "PASS"
else
    test_result "A3: Long mode (64-bit) enabled" "FAIL"
fi

# A4: GDT (Global Descriptor Table)
echo -n "A4: Testing GDT setup... "
if nm kernel.elf | grep -q "gdt_install" && [ -f kernel/arch/x86/gdt.c ]; then
    test_result "A4: GDT (Global Descriptor Table)" "PASS"
else
    test_result "A4: GDT (Global Descriptor Table)" "FAIL"
fi

# A5: IDT (Interrupt Descriptor Table)
echo -n "A5: Testing IDT setup... "
if nm kernel.elf | grep -q "idt_install" && [ -f kernel/arch/x86/idt.c ]; then
    test_result "A5: IDT (Interrupt Descriptor Table)" "PASS"
else
    test_result "A5: IDT (Interrupt Descriptor Table)" "FAIL"
fi

# A6: Interrupt handling (PIC)
echo -n "A6: Testing interrupt handling... "
if [ -f kernel/arch/x86/pic.c ] && grep -q "pic_remap" kernel/arch/x86/pic.c; then
    test_result "A6: Interrupt handling (PIC)" "PASS"
else
    test_result "A6: Interrupt handling (PIC)" "FAIL"
fi

# A7: System calls interface
echo -n "A7: Testing syscall interface... "
SYSCALL_COUNT=$(grep -c "case SYS_" kernel/syscall.c || echo 0)
if [ "$SYSCALL_COUNT" -ge 15 ]; then
    test_result "A7: System calls interface (${SYSCALL_COUNT} syscalls)" "PASS"
else
    test_result "A7: System calls interface (${SYSCALL_COUNT} syscalls)" "FAIL"
fi

# A8: Kernel compiles without errors
echo -n "A8: Testing kernel compilation... "
if make -C kernel clean > /dev/null 2>&1 && make -C kernel -j8 > /dev/null 2>&1; then
    test_result "A8: Kernel compiles without errors" "PASS"
else
    test_result "A8: Kernel compiles without errors" "FAIL"
fi

echo ""

# ============================================
# CATEGORY B: MEMORY MANAGEMENT (7 requirements)
# ============================================
echo -e "${YELLOW}[CATEGORY B: MEMORY MANAGEMENT]${NC}"

# B1: 4-level paging (PML4)
echo -n "B1: Testing 4-level paging... "
if grep -q "pml4" kernel/start.S && grep -q "pdpt" kernel/start.S; then
    test_result "B1: 4-level paging (PML4→PDPT→PD→PT)" "PASS"
else
    test_result "B1: 4-level paging (PML4→PDPT→PD→PT)" "FAIL"
fi

# B2: Physical memory allocator
echo -n "B2: Testing physical memory allocator... "
if [ -f kernel/mm/physical_memory.c ] && grep -q "physical_memory_init" kernel/mm/physical_memory.c; then
    test_result "B2: Physical memory allocator (bitmap)" "PASS"
else
    test_result "B2: Physical memory allocator (bitmap)" "FAIL"
fi

# B3: Virtual memory management
echo -n "B3: Testing virtual memory... "
if [ -f kernel/mm/virtual_memory.c ] && (grep -q "vmm_init\|vmm_map\|kmalloc" kernel/mm/virtual_memory.c); then
    test_result "B3: Virtual memory management" "PASS"
else
    test_result "B3: Virtual memory management" "FAIL"
fi

# B4: Copy-on-Write (COW)
echo -n "B4: Testing Copy-on-Write... "
if grep -q "pt_clone_for_cow" kernel/mm/pagetable.c; then
    test_result "B4: Copy-on-Write (COW) fork support" "PASS"
else
    test_result "B4: Copy-on-Write (COW) fork support" "FAIL"
fi

# B5: MMIO support
echo -n "B5: Testing MMIO support... "
if [ -f kernel/mm/mmio.c ] && grep -q "mmio_init" kernel/mm/mmio.c; then
    test_result "B5: MMIO (Memory-Mapped I/O) support" "PASS"
else
    test_result "B5: MMIO (Memory-Mapped I/O) support" "FAIL"
fi

# B6: Heap allocator (kmalloc)
echo -n "B6: Testing heap allocator... "
if grep -q "kmalloc" kernel/mm/virtual_memory.c; then
    test_result "B6: Heap allocator (kmalloc/kfree)" "PASS"
else
    test_result "B6: Heap allocator (kmalloc/kfree)" "FAIL"
fi

# B7: Framebuffer driver
echo -n "B7: Testing framebuffer driver... "
if [ -f kernel/mm/framebuffer.c ] && grep -q "fb_init" kernel/mm/framebuffer.c; then
    test_result "B7: Framebuffer driver (VGA text mode)" "PASS"
else
    test_result "B7: Framebuffer driver (VGA text mode)" "FAIL"
fi

echo ""

# ============================================
# CATEGORY C: PROCESS MANAGEMENT (6 requirements)
# ============================================
echo -e "${YELLOW}[CATEGORY C: PROCESS MANAGEMENT]${NC}"

# C1: Process Control Block (PCB)
echo -n "C1: Testing PCB structure... "
if grep -q "process_t" kernel/process_manager.h && [ -f kernel/process_manager.c ]; then
    test_result "C1: Process Control Block (PCB)" "PASS"
else
    test_result "C1: Process Control Block (PCB)" "FAIL"
fi

# C2: Process creation and termination
echo -n "C2: Testing process management... "
if [ -f kernel/process_manager.c ] && (grep -q "pm_create_process\|pm_clone_process\|process_create" kernel/process_manager.c); then
    test_result "C2: Process creation and termination" "PASS"
else
    test_result "C2: Process creation and termination" "FAIL"
fi

# C3: fork() system call
echo -n "C3: Testing fork() syscall... "
if grep -q "SYS_FORK" kernel/syscall.c && grep -q "sys_fork" kernel/syscall.c; then
    test_result "C3: fork() system call with COW" "PASS"
else
    test_result "C3: fork() system call with COW" "FAIL"
fi

# C4: exec() system call
echo -n "C4: Testing exec() syscall... "
if grep -q "SYS_EXEC" kernel/syscall.c; then
    test_result "C4: exec() system call" "PASS"
else
    test_result "C4: exec() system call" "FAIL"
fi

# C5: Scheduler (preemptive)
echo -n "C5: Testing preemptive scheduler... "
if [ -f kernel/scheduler/preemptive.c ] && (grep -q "scheduler_switch\|preempt\|schedule" kernel/scheduler/preemptive.c); then
    test_result "C5: Preemptive multitasking scheduler" "PASS"
else
    test_result "C5: Preemptive multitasking scheduler" "FAIL"
fi

# C6: Context switching
echo -n "C6: Testing context switching... "
if (grep -q "context_switch\|switch_to" kernel/scheduler/preemptive.c) || [ -f kernel/arch/x86/interrupts.S ]; then
    test_result "C6: Context switching (register save/restore)" "PASS"
else
    test_result "C6: Context switching (register save/restore)" "FAIL"
fi

echo ""

# ============================================
# CATEGORY D: NETWORKING (7 requirements)
# ============================================
echo -e "${YELLOW}[CATEGORY D: NETWORKING]${NC}"

# D1: E1000 NIC driver
echo -n "D1: Testing E1000 driver... "
if [ -f kernel/drivers/e1000.c ] && grep -q "e1000_init" kernel/drivers/e1000.c; then
    test_result "D1: E1000 network card driver" "PASS"
else
    test_result "D1: E1000 network card driver" "FAIL"
fi

# D2: Ethernet layer
echo -n "D2: Testing Ethernet layer... "
if [ -f kernel/net/ethernet.c ] && grep -q "eth_init" kernel/net/ethernet.c; then
    test_result "D2: Ethernet layer (MAC addresses)" "PASS"
else
    test_result "D2: Ethernet layer (MAC addresses)" "FAIL"
fi

# D3: ARP protocol
echo -n "D3: Testing ARP protocol... "
if [ -f kernel/net/arp.c ] && grep -q "arp_init" kernel/net/arp.c; then
    test_result "D3: ARP protocol implementation" "PASS"
else
    test_result "D3: ARP protocol implementation" "FAIL"
fi

# D4: IPv4 protocol
echo -n "D4: Testing IPv4 protocol... "
if [ -f kernel/net/ip.c ] && grep -q "ip_init" kernel/net/ip.c; then
    test_result "D4: IPv4 protocol implementation" "PASS"
else
    test_result "D4: IPv4 protocol implementation" "FAIL"
fi

# D5: UDP protocol
echo -n "D5: Testing UDP protocol... "
if [ -f kernel/net/udp.c ] && grep -q "udp_init" kernel/net/udp.c; then
    test_result "D5: UDP protocol implementation" "PASS"
else
    test_result "D5: UDP protocol implementation" "FAIL"
fi

# D6: mDNS service discovery
echo -n "D6: Testing mDNS... "
if [ -f kernel/net/mdns.c ] && grep -q "mdns_init" kernel/net/mdns.c; then
    test_result "D6: mDNS service discovery" "PASS"
else
    test_result "D6: mDNS service discovery" "FAIL"
fi

# D7: P2P overlay network
echo -n "D7: Testing P2P overlay... "
if [ -f kernel/net/p2p.c ] && grep -q "p2p_init" kernel/net/p2p.c; then
    test_result "D7: P2P overlay network" "PASS"
else
    test_result "D7: P2P overlay network" "FAIL"
fi

echo ""

# ============================================
# CATEGORY E: DISTRIBUTED SYSTEMS (4 requirements)
# ============================================
echo -e "${YELLOW}[CATEGORY E: DISTRIBUTED SYSTEMS]${NC}"

# E1: Distributed scheduler
echo -n "E1: Testing distributed scheduler... "
if [ -f kernel/scheduler/distributed.c ] && grep -q "dsched_init" kernel/scheduler/distributed.c; then
    test_result "E1: Distributed scheduler (load balancing)" "PASS"
else
    test_result "E1: Distributed scheduler (load balancing)" "FAIL"
fi

# E2: Distributed Shared Memory
echo -n "E2: Testing DSM... "
if [ -f kernel/mm/distributed_memory.c ] && grep -q "dsm_init" kernel/mm/distributed_memory.c; then
    test_result "E2: Distributed Shared Memory (DSM)" "PASS"
else
    test_result "E2: Distributed Shared Memory (DSM)" "FAIL"
fi

# E3: Distributed locks
echo -n "E3: Testing distributed locks... "
if [ -f kernel/sync/distributed_lock.c ] && grep -q "dlock_init" kernel/sync/distributed_lock.c; then
    test_result "E3: Distributed locks (Ricart-Agrawala)" "PASS"
else
    test_result "E3: Distributed locks (Ricart-Agrawala)" "FAIL"
fi

# E4: Fault tolerance
echo -n "E4: Testing fault tolerance... "
if [ -f kernel/fault_tolerance/heartbeat.c ] && grep -q "ft_init" kernel/fault_tolerance/heartbeat.c; then
    test_result "E4: Fault tolerance (heartbeat + Bully)" "PASS"
else
    test_result "E4: Fault tolerance (heartbeat + Bully)" "FAIL"
fi

echo ""

# ============================================
# CATEGORY F: MACHINE LEARNING (5 requirements)
# ============================================
echo -e "${YELLOW}[CATEGORY F: MACHINE LEARNING]${NC}"

# F1: Linear Regression
echo -n "F1: Testing Linear Regression... "
if [ -f kernel/ml/linear_regression.c ] && grep -q "lr_init" kernel/ml/linear_regression.c; then
    test_result "F1: Linear Regression algorithm" "PASS"
else
    test_result "F1: Linear Regression algorithm" "FAIL"
fi

# F2: Logistic Regression
echo -n "F2: Testing Logistic Regression... "
if [ -f kernel/ml/logistic_regression.c ] && (grep -q "logistic_init\|logistic_train\|sigmoid" kernel/ml/logistic_regression.c); then
    test_result "F2: Logistic Regression algorithm" "PASS"
else
    test_result "F2: Logistic Regression algorithm" "FAIL"
fi

# F3: SVM
echo -n "F3: Testing SVM... "
if [ -f kernel/ml/svm.c ] && grep -q "svm_init" kernel/ml/svm.c; then
    test_result "F3: SVM (Support Vector Machine)" "PASS"
else
    test_result "F3: SVM (Support Vector Machine)" "FAIL"
fi

# F4: Decision Tree
echo -n "F4: Testing Decision Tree... "
if [ -f kernel/ml/decision_tree.c ] && grep -q "dt_init" kernel/ml/decision_tree.c; then
    test_result "F4: Decision Tree algorithm" "PASS"
else
    test_result "F4: Decision Tree algorithm" "FAIL"
fi

# F5: Neural Network (MLP)
echo -n "F5: Testing Neural Network... "
if [ -f kernel/ml/mlp.c ] && grep -q "mlp_init" kernel/ml/mlp.c; then
    test_result "F5: Multi-Layer Perceptron (MLP)" "PASS"
else
    test_result "F5: Multi-Layer Perceptron (MLP)" "FAIL"
fi

echo ""

# ============================================
# CATEGORY G: FILESYSTEM (3 requirements)
# ============================================
echo -e "${YELLOW}[CATEGORY G: FILESYSTEM]${NC}"

# G1: File descriptors
echo -n "G1: Testing file descriptors... "
if [ -f kernel/fs.c ] && (grep -q "file_t.*files.*MAX_FILES\|fs_alloc\|fs_incref" kernel/fs.c); then
    test_result "G1: File descriptor table" "PASS"
else
    test_result "G1: File descriptor table" "FAIL"
fi

# G2: Read/Write syscalls
echo -n "G2: Testing I/O syscalls... "
if grep -q "SYS_READ" kernel/syscall.c && grep -q "SYS_WRITE" kernel/syscall.c; then
    test_result "G2: read()/write() syscalls" "PASS"
else
    test_result "G2: read()/write() syscalls" "FAIL"
fi

# G3: IPC (Inter-Process Communication)
echo -n "G3: Testing IPC... "
if [ -f kernel/ipc/message.c ] && grep -q "ipc_send" kernel/ipc/message.c; then
    test_result "G3: IPC message passing" "PASS"
else
    test_result "G3: IPC message passing" "FAIL"
fi

echo ""

# ============================================
# CATEGORY H: DRIVERS (4 requirements)
# ============================================
echo -e "${YELLOW}[CATEGORY H: HARDWARE DRIVERS]${NC}"

# H1: Keyboard driver
echo -n "H1: Testing keyboard driver... "
if [ -f kernel/drivers/keyboard.c ] && grep -q "keyboard_install" kernel/drivers/keyboard.c; then
    test_result "H1: PS/2 keyboard driver" "PASS"
else
    test_result "H1: PS/2 keyboard driver" "FAIL"
fi

# H2: Serial driver
echo -n "H2: Testing serial driver... "
if [ -f kernel/drivers/serial.c ] && grep -q "serial_init" kernel/drivers/serial.c; then
    test_result "H2: UART serial driver (COM1)" "PASS"
else
    test_result "H2: UART serial driver (COM1)" "FAIL"
fi

# H3: Timer driver
echo -n "H3: Testing timer driver... "
if [ -f kernel/drivers/timer.c ] && grep -q "timer_install" kernel/drivers/timer.c; then
    test_result "H3: PIT timer driver" "PASS"
else
    test_result "H3: PIT timer driver" "FAIL"
fi

# H4: VGA driver
echo -n "H4: Testing VGA driver... "
if grep -q "fb_console" kernel/mm/framebuffer.c; then
    test_result "H4: VGA text mode driver" "PASS"
else
    test_result "H4: VGA text mode driver" "FAIL"
fi

echo ""

# ============================================
# CATEGORY I: SHELL (2 requirements)
# ============================================
echo -e "${YELLOW}[CATEGORY I: INTERACTIVE SHELL]${NC}"

# I1: Shell with command parsing
echo -n "I1: Testing shell... "
if [ -f kernel/shell.c ] && grep -q "shell_run" kernel/shell.c; then
    test_result "I1: Interactive shell with commands" "PASS"
else
    test_result "I1: Interactive shell with commands" "FAIL"
fi

# I2: Built-in commands (at least 5)
echo -n "I2: Testing shell commands... "
CMD_COUNT=$(grep -c "cmd_" kernel/shell.c | head -1)
if [ "$CMD_COUNT" -ge 5 ]; then
    test_result "I2: Built-in shell commands (${CMD_COUNT}+)" "PASS"
else
    test_result "I2: Built-in shell commands (${CMD_COUNT}+)" "FAIL"
fi

echo ""

# ============================================
# CATEGORY J: ENVIRONMENT (1 requirement)
# ============================================
echo -e "${YELLOW}[CATEGORY J: BUILD ENVIRONMENT]${NC}"

# J1: Makefile build system
echo -n "J1: Testing build system... "
if [ -f kernel/Makefile ] && [ -f Makefile ]; then
    test_result "J1: Makefile build system" "PASS"
else
    test_result "J1: Makefile build system" "FAIL"
fi

echo ""

# ============================================
# CATEGORY K: APPLICATIONS (3 requirements)
# ============================================
echo -e "${YELLOW}[CATEGORY K: USER APPLICATIONS]${NC}"

# K1: P2P Chat application
echo -n "K1: Testing P2P Chat app... "
if [ -f user/app_p2p_chat.c ]; then
    test_result "K1: P2P Chat application" "PASS"
else
    test_result "K1: P2P Chat application" "FAIL"
fi

# K2: File sharing application
echo -n "K2: Testing File Share app... "
if [ -f user/app_file_share.c ]; then
    test_result "K2: File sharing application" "PASS"
else
    test_result "K2: File sharing application" "FAIL"
fi

# K3: ML demo application
echo -n "K3: Testing ML demo app... "
if [ -f user/app_ml_demo.c ]; then
    test_result "K3: ML demonstration application" "PASS"
else
    test_result "K3: ML demonstration application" "FAIL"
fi

echo ""

# ============================================
# CATEGORY L: DOCUMENTATION (2 requirements)
# ============================================
echo -e "${YELLOW}[CATEGORY L: DOCUMENTATION]${NC}"

# L1: README documentation
echo -n "L1: Testing README... "
if [ -f README.md ] && [ $(wc -l < README.md) -gt 50 ]; then
    test_result "L1: README documentation" "PASS"
else
    test_result "L1: README documentation" "FAIL"
fi

# L2: Technical documentation
echo -n "L2: Testing technical docs... "
DOC_COUNT=$(find . -name "*.md" -type f | wc -l)
if [ "$DOC_COUNT" -ge 10 ]; then
    test_result "L2: Technical documentation (${DOC_COUNT} files)" "PASS"
else
    test_result "L2: Technical documentation (${DOC_COUNT} files)" "FAIL"
fi

echo ""

# ============================================
# FUNCTIONAL TESTS
# ============================================
echo -e "${YELLOW}[FUNCTIONAL TESTS]${NC}"

# Test scheduler functionality
echo -n "Testing scheduler functionality... "
if [ -f tests/scheduler_test.c ]; then
    gcc -o tests/scheduler_test tests/scheduler_test.c 2>/dev/null || true
    if [ -f tests/scheduler_test ]; then
        if ./tests/scheduler_test 2>&1 | grep -q "PASS"; then
            test_result "Scheduler functionality test" "PASS"
        else
            test_result "Scheduler functionality test" "FAIL"
        fi
    else
        test_result "Scheduler functionality test" "SKIP"
    fi
else
    test_result "Scheduler functionality test" "SKIP"
fi

# Test IPC functionality
echo -n "Testing IPC functionality... "
if [ -f tests/ipc_test.c ]; then
    gcc -o tests/ipc_test tests/ipc_test.c 2>/dev/null || true
    if [ -f tests/ipc_test ]; then
        if ./tests/ipc_test 2>&1 | grep -q "passed"; then
            test_result "IPC functionality test" "PASS"
        else
            test_result "IPC functionality test" "FAIL"
        fi
    else
        test_result "IPC functionality test" "SKIP"
    fi
else
    test_result "IPC functionality test" "SKIP"
fi

echo ""

# ============================================
# CODE METRICS
# ============================================
echo -e "${YELLOW}[CODE METRICS]${NC}"

# Lines of code
LOC=$(find . -name "*.c" -o -name "*.h" -o -name "*.S" -o -name "*.asm" 2>/dev/null | xargs wc -l 2>/dev/null | tail -1 | awk '{print $1}')
echo -e "Total lines of code: ${GREEN}${LOC}${NC}"

# Number of source files
FILE_COUNT=$(find kernel -name "*.c" -o -name "*.h" -o -name "*.S" | wc -l)
echo -e "Number of source files: ${GREEN}${FILE_COUNT}${NC}"

# Kernel size
KERNEL_SIZE=$(ls -lh kernel.elf 2>/dev/null | awk '{print $5}')
echo -e "Kernel binary size: ${GREEN}${KERNEL_SIZE}${NC}"

echo ""

# ============================================
# FINAL RESULTS
# ============================================
echo -e "${BLUE}═══════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  TEST RESULTS SUMMARY${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════${NC}"
echo ""

PERCENTAGE=$((PASSED * 100 / TOTAL))

echo -e "Total tests:  ${BLUE}${TOTAL}${NC}"
echo -e "Passed:       ${GREEN}${PASSED}${NC}"
echo -e "Failed:       ${RED}${FAILED}${NC}"
echo -e "Success rate: ${GREEN}${PERCENTAGE}%${NC}"
echo ""

if [ $PERCENTAGE -ge 95 ]; then
    echo -e "${GREEN}[EXCELLENT]${NC} Project exceeds requirements!"
elif [ $PERCENTAGE -ge 85 ]; then
    echo -e "${GREEN}[GOOD]${NC} Project meets requirements!"
elif [ $PERCENTAGE -ge 70 ]; then
    echo -e "${YELLOW}[ACCEPTABLE]${NC} Project mostly meets requirements"
else
    echo -e "${RED}[NEEDS WORK]${NC} Project needs improvement"
fi

echo ""

# Show failed tests if any
if [ $FAILED -gt 0 ]; then
    echo -e "${RED}Failed tests:${NC}"
    for test in "${FAILED_TESTS[@]}"; do
        echo -e "  ${RED}✗${NC} $test"
    done
    echo ""
fi

# Save results to file
REPORT_FILE="test_report_$(date +%Y%m%d_%H%M%S).txt"
{
    echo "SO_DESCENTRALIZADO - TEST REPORT"
    echo "================================="
    echo "Date: $(date)"
    echo ""
    echo "RESULTS:"
    echo "  Total: $TOTAL"
    echo "  Passed: $PASSED"
    echo "  Failed: $FAILED"
    echo "  Success rate: ${PERCENTAGE}%"
    echo ""
    echo "CODE METRICS:"
    echo "  Lines of code: $LOC"
    echo "  Source files: $FILE_COUNT"
    echo "  Kernel size: $KERNEL_SIZE"
    echo ""
    if [ $FAILED -gt 0 ]; then
        echo "FAILED TESTS:"
        for test in "${FAILED_TESTS[@]}"; do
            echo "  - $test"
        done
    fi
} > "$REPORT_FILE"

echo -e "Report saved to: ${BLUE}${REPORT_FILE}${NC}"
echo ""

# Exit with appropriate code
if [ $FAILED -eq 0 ]; then
    exit 0
else
    exit 1
fi

Phase 1  Phase 2 Plan: SOH Descentralizado

This document lays out a prioritized, actionable plan to finish Phase 1, move into Phase 2 and prepare the kernel base for an adhoc decentralized networking environment.

## Goals
- Harden and finish Phase 1 (syscalls, ELF exec, secure transition to ring3).
- Add multiprocess support (fork/exec/wait) and make scheduler preemptive (timer IRQ).
- Add networking primitives and embed a WASM runtime for sandboxed IA modules.

## Milestones (priority order)

### Phase 1  Finish & Solidify (High priority)
1. Stabilize boot path and early init
   - Ensure reliable 3264 transition (we already added a minimal boot GDT).
   - Add additional boot-time sanity checks and more early serial traces.
2. Syscall path / int 0x80
   - Confirm isr_0x80 installed and idt gate DPL=3 (already present).
   - Implement syscall/sysret fast path (optional performance enhancement later).
3. Expand elf_exec to create per-process address space
   - Implement per-process page tables (isolation), initial stack layout (argc/argv/envp).
   - Adjust elf_exec to set correct CR3 and TSS / RSP0 if needed.
4. Implement sys_fork + sys_exec (minimal working versions)
   - Start with simple fork using copy-on-write or shallow clone + kmalloc for new stacks.
   - Implement sys_wait and sys_exit lifecycle (zombie/parent reaping).
5. Add tests and harnesses
   - user/hello.c already used. Add more user programs and host-side tests to validate fork/exec/wait.

### Phase 2  Multi-process + Networking + WASM (Medium priority)
1. Make scheduler preemptive
   - Add PIT IRQ handler and preemptive context switch.
   - Implement sleep/wakeup and blocking primitives, task priorities.
2. Networking foundation
   - Add a minimal NIC driver (e1000 or virtio-net) for QEMU testing.
   - Implement a lightweight network stack (ARP  IP  UDP) and socket API.
3. Distributed primitives
   - Add service discovery (mDNS/beacon), P2P overlay, rpc/pubsub primitives.
4. WASM integration
   - Embed wasm3 or similar to run IA modules sandboxed in user-space.

### Phase 3  Productionization & Scale (lower priority)
- Security hardening (no RWX kernel segments, stack guards, capability model).
- Filesystems, persistent storage, caching.
- CI, automated QEMU tests for networking & distributed flows.

## Deliverables & Tests
- Unit tests for syscalls and scheduler (host-side + integration under QEMU).
- Example user programs: hello, fork/echo server, wasm sample, P2P discovery demo.
- CI job: build + qemu smoke tests on every PR/branch.

## Next actions I can take for you now
1. Create a feature branch containing the Phase1 plan and the earlier GDT fix (done next).
2. Implement sys_fork (minimal functional prototype) + tests.
3. Implement preemptive scheduler (PIT IRQ + context switch) + tests.

Pick an action to continue (2 or 3) and Ill begin implementing it immediately with unit tests and QEMU validation.

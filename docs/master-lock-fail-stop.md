# Master lock fail-stop metadata

The mmap backend keeps the existing semaphore counter array at the beginning
of `flock.bin`. Counter zero remains the master lock and asset semaphore
counters keep their existing indices. A versioned owner record is appended at
offset `sizeof(int) * HAKO_CONFIG_MAX_SEM`; no additional state file is used.

The version 1 record contains:

- magic, format version, and record size;
- transition state (`UNLOCKED`, `ACQUIRING`, `LOCKED`, or `RELEASING`);
- owner PID and an OS process-start token;
- monotonically increasing acquisition generation;
- checksum over the complete record.

The process-start token distinguishes a live owner from an unrelated process
that reused the same PID. Linux uses `/proc/<pid>/stat` start time, macOS uses
the kernel process start time returned by `sysctl`, and Windows uses the process
creation `FILETIME`.

An unreaped zombie is not a live owner. Linux treats `/proc/<pid>/stat`
states `Z` and `X` as dead, and macOS treats `SZOMB` as dead. Failure to read
or interpret a process state remains `UNKNOWN`; it is never promoted to
`ABORT` without positive evidence.

## Update protocol

All counter and metadata updates occur while holding the OS file lock.
Acquisition records `ACQUIRING` and the owner identity, changes counter zero
from 1 to 0, then commits `LOCKED`. Release records `RELEASING`, changes the
counter from 0 to 1, then clears the owner and commits `UNLOCKED`.

If a process exits during either transition, the OS releases its file lock but
the transition/owner evidence remains. A dead or PID-reused owner is `ABORT`.
Invalid, incomplete, or legacy metadata is `UNKNOWN` and fails immediately.
If only the owner identity cannot be inspected, a runtime caller with a
bounded wait scope retries without writing and eventually times out as
`UNKNOWN`; `check` reports `UNKNOWN` immediately. No `UNKNOWN` condition
transfers ownership or edits the master counter.

`flock::create()` initializes counters and metadata when it atomically creates
a new `flock.bin`. When the master creates shared-memory area zero against an
existing v1 file, asset semaphore counters are cleared only if inspection is
`OK` and `UNLOCKED`. Creating secondary shared-memory areas, or observing
any other health/state, does not clear them. A legacy counter-only file is
retained for diagnosis and classified as `UNKNOWN` rather than silently
upgraded.

The creator sizes a new file for the complete versioned record before
initializing it. A simultaneous opener that observes this new-format size but
not yet a valid checksum waits for a bounded two seconds for initialization to
finish. The legacy counter-only size is never treated as in-progress, and the
default inspection path remains non-blocking.

Runtime detection writes its diagnostic directly to standard error before
throwing `MasterLockStateError`. This path does not use Hakoniwa shared-memory
logging and therefore cannot recursively acquire the failed master lock. API
bindings must catch this exception at their C or language-runtime boundary and
translate it to that API's established failure representation.

## Compatibility and upgrade contract

| On-disk/session state | `check` | Runtime `master_lock()` | Supported action |
| --- | --- | --- | --- |
| no `flock.bin` | `UNKNOWN` until a runtime creates it | a new v1 file is atomically created and initialized | normal start |
| valid v1, unlocked | `OK` | acquires normally | normal start/join |
| valid v1, live owner | `OK`, `LOCKED` | waits; bounded callers may time out | wait or stop the live session |
| valid v1, confirmed dead or PID-reused owner | `ABORT` | typed `ABORT` error | preserve evidence, stop all processes, then use the explicit Core PRO recovery procedure |
| valid v1, owner cannot be inspected | `UNKNOWN` | retry without writes inside the caller's bounded wait, then time out as `UNKNOWN` | fix OS permissions/diagnostics; do not recover while liveness is unknown |
| legacy counter-only file | `UNKNOWN` | typed `UNKNOWN` error | use the offline migration procedure below |
| incomplete/corrupt/unsupported metadata | `UNKNOWN` | typed `UNKNOWN` error | preserve evidence and investigate; do not rewrite automatically |
| OS file lock is busy during `check` | `UNKNOWN` immediately | waits only according to the caller's bounded-wait policy | retry diagnosis; do not infer owner death |

The new runtime does not support mixed legacy and v1 writers. A process that
opened the counter-only format before an upgrade can still modify counters
without maintaining v1 owner metadata. Therefore a new process refuses its
first logical master-lock operation with typed `UNKNOWN`, even after the old
writer releases the OS byte-range lock. It never appends metadata to that file.

### Offline migration from a legacy file

1. Stop the launcher, master, assets, external clients, and every old-version
   process that can use the configured `core_mmap_path`.
2. Verify at the OS level that none of those processes remains. If quiescence
   cannot be proved, stop and keep the files for diagnosis.
3. Preserve a copy of `flock.bin` and the related `mmap-0x*.bin` files.
4. Use the explicit, offline Core PRO migration/recovery operation documented
   by hakoniwa-core-pro issue #99. Core CPP intentionally provides no deletion,
   counter reset, or automatic format conversion command.
5. Start one new-version master. It creates a fresh v1 file; only after that
   succeeds may new-version assets and external clients join.

An otherwise normal stop does not make a legacy file safe to rewrite by
itself: process quiescence is a separate operator assertion. Old and new
writers must never overlap.

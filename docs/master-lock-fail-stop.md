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
`OK` and `UNLOCKED` (checked and written under the same OS file lock), so
counts left by a previous session do not carry over. Creating secondary
shared-memory areas, or observing any other health/state, does not clear them.
`UNLOCKED` means nobody holds the master lock at that instant, not that no
session runs: starting a second master while a session is live remains
unsupported, as before (master initialization also resets the master data). A legacy counter-only file is
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
| valid v1, confirmed dead or PID-reused owner | `ABORT` | typed `ABORT` error | preserve evidence, stop all processes, then `hako-cmd recover` (Core PRO), then start |
| valid v1, owner cannot be inspected | `UNKNOWN` | retry without writes inside the caller's bounded wait, then time out as `UNKNOWN` | fix OS permissions/diagnostics; do not recover while liveness is unknown |
| legacy counter-only file | `UNKNOWN` | typed `UNKNOWN` error | upgrade procedure below (`hako-cmd recover` clears it) |
| incomplete/corrupt/unsupported metadata | `UNKNOWN` | typed `UNKNOWN` error | preserve evidence and investigate; do not rewrite automatically |
| OS file lock is busy during `check` | `UNKNOWN` immediately | waits only according to the caller's bounded-wait policy | retry diagnosis; do not infer owner death |

The new runtime does not support mixed legacy and v1 writers. A process that
opened the counter-only format before an upgrade can still modify counters
without maintaining v1 owner metadata. Therefore a new process refuses its
first logical master-lock operation with typed `UNKNOWN`, even after the old
writer releases the OS byte-range lock. It never appends metadata to that file.

### Upgrade and recovery procedure

Mixed Core versions on one `core_mmap_path` are unsupported. Upgrade, and
recover after an `ABORT`, in this order:

1. Stop the launcher, master, assets, external clients, and every process
   (old or new version) that can use the configured `core_mmap_path`. Verify
   at the OS level that none remains; if that cannot be shown, stop here and
   keep the files for diagnosis.
2. Install all Core components of one version together (Core CPP, Core PRO
   and the bindings built against them).
3. Run `hako-cmd recover`. It is part of Hakoniwa Core PRO (the sample
   `hako-cmd` in this repository has no `recover` subcommand). It removes the
   known runtime files (`mmap-0x*.bin`, `pro_init.lock`, `pdu_init.lock`,
   then `flock.bin` last), the legacy counter-only `flock.bin` included, and
   refuses while a recorded master or asset process is alive. Keep a copy of
   the files first if you need them for diagnosis.
4. Start one master. It creates a fresh v1 `flock.bin`; only after that
   succeeds may assets and external clients join.

Core CPP itself deletes, resets or converts nothing outside a clean master
start: a legacy file is never rewritten or upgraded in place, and process
quiescence is the operator's assertion in step 1. Hakoniwa Business Pack runs
steps 3–4 for its Foundation runtime directory at the Workspace entry (it
moves the previous Core's files aside when no Launcher session runs).

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
Invalid, incomplete, legacy, or unverifiable metadata is `UNKNOWN`. Neither
condition transfers ownership or edits the counter; recovery is an explicit
operator action in `hako-cmd`.

`flock::create()` initializes counters and metadata only when it atomically
creates a new `flock.bin`. Opening an existing file never resets a live master
counter. A legacy counter-only file is retained for diagnosis and classified as
`UNKNOWN` rather than silently upgraded.

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

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

Runtime detection writes its diagnostic directly to standard error before
throwing `MasterLockStateError`. This path does not use Hakoniwa shared-memory
logging and therefore cannot recursively acquire the failed master lock. C API
entry points already catch C++ exceptions and return their established failure
values, which keeps exceptions from crossing C/Python boundaries.

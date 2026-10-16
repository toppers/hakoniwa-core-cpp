# Runtime lifecycle protocol for offline recovery

Hakoniwa mmap runtime layout version 3 adds a cooperative cross-process
lifecycle protocol. Its purpose is narrow: an explicit recovery tool can prove
that every compatible participant is gone and can exclude a new compatible
participant for the complete verification-to-deletion interval.

## Files and invariant

- `runtime-lifecycle-v1.gate` serializes session creation, every attachment,
  and explicit recovery.
- `runtime-lifecycle-v1.bin` contains a checked version and session generation.
- `runtime-participant-v1-<pid>-<start-token>-<sequence>.lease` is locked for
  the lifetime of each process attachment.
- every mmap header contains the same lifecycle generation.

A process creating or attaching to an mmap runtime first owns the gate, then
locks a participant lease, and only then releases the gate. A recovery tool
owns the gate, verifies the versioned marker and mmap generation, and refuses
unless it can nonblockingly lock every participant lease. It retains the gate
until deletion is complete. Consequently a compatible process is either
represented by a busy lease or is still waiting outside the gate; it cannot
appear between verification and deletion.

The gate and marker are control files and remain after successful recovery.
Unlocked stale participant leases may be removed while the recovery gate is
held. The ownership evidence in `flock.bin` remains a separate master-lock
contract and must be deleted last by the recovery tool.

## Compatibility and refusal

| Runtime state | New process | Recovery proof |
| --- | --- | --- |
| clean directory | creator establishes v1 marker/generation | no files to recover |
| layout v3, matching generation, all leases unlocked | new creator may start after clean master-lock validation | proven while gate is held |
| any live participant lease | participant already joined | recovery is `BUSY` and must refuse |
| missing/corrupt marker with old artifacts | `UNKNOWN`; no automatic upgrade | unproven; recovery must refuse |
| layout v2 or generation mismatch | load/start is rejected | mixed/legacy state; recovery must refuse |
| unsafe symlink/non-regular lifecycle path | `UNKNOWN` | recovery must refuse |

Layout version 3 deliberately prevents a layout-v2 client from loading the new
shared memory and prevents a new client from treating layout-v2 memory as part
of the lifecycle protocol. An old binary does not know the gate and therefore
cannot be made cooperative by new code. Operators must not mix old and new
binaries in one `core_mmap_path`; the tool never claims it can exclude a
deliberately uncooperative old writer.

## Deliberate legacy migration

`hako-cmd recover` must not be used for a pre-v1/legacy directory because its
quiescence cannot be proven. The bounded offline procedure is:

1. Stop the Launcher and every master, asset, `init_for_external()` client,
   debugger, and service using the configured `core_mmap_path`.
2. Verify at the OS level that no related process remains. If that cannot be
   established, stop and preserve the directory unchanged.
3. Preserve evidence by renaming the entire runtime directory on the same
   filesystem, for example from `mmap` to
   `mmap.legacy-YYYYMMDD-HHMMSS`. Do not delete individual files.
4. Create a new empty directory at the configured `core_mmap_path` with the
   same owner and permissions.
5. Start only layout-v3-compatible components. The first creator establishes
   the lifecycle marker and generation.
6. Retain the quarantined directory until the incident or migration has been
   reviewed; delete it later as a separate operator decision.

This is intentionally manual. There is no automatic recovery, lock takeover,
or conversion of legacy evidence.

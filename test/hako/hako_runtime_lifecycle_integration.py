from __future__ import annotations

import argparse
from pathlib import Path
import subprocess
import tempfile
import time


def wait_path(path: Path, process: subprocess.Popen, timeout: float = 5.0) -> None:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if path.exists():
            return
        if process.poll() is not None:
            stdout, stderr = process.communicate()
            raise AssertionError(
                f"process exited before barrier: rc={process.returncode}, "
                f"stdout={stdout!r}, stderr={stderr!r}"
            )
        time.sleep(0.02)
    raise AssertionError(f"barrier timeout: {path}")


def start(helper: str, mode: str, root: Path, ready: Path, release: Path):
    return subprocess.Popen(
        [helper, mode, str(root), str(ready), str(release)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )


def finish(process: subprocess.Popen, release: Path) -> None:
    release.write_text("release\n")
    stdout, stderr = process.communicate(timeout=5)
    if process.returncode != 0:
        raise AssertionError(
            f"process failed: rc={process.returncode}, "
            f"stdout={stdout!r}, stderr={stderr!r}"
        )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--helper", required=True)
    args = parser.parse_args()

    with tempfile.TemporaryDirectory(prefix="hako-lifecycle-integration-") as temp:
        root = Path(temp) / "runtime"
        root.mkdir()
        participant_ready = Path(temp) / "participant.ready"
        participant_release = Path(temp) / "participant.release"
        participant = start(
            args.helper,
            "participant-create",
            root,
            participant_ready,
            participant_release,
        )
        wait_path(participant_ready, participant)

        busy = subprocess.run(
            [args.helper, "recover", str(root)],
            capture_output=True,
            text=True,
            timeout=5,
            check=False,
        )
        if busy.returncode != 3 or not busy.stdout.startswith("BUSY "):
            raise AssertionError(f"live external participant was not refused: {busy!r}")
        finish(participant, participant_release)

        recovery_ready = Path(temp) / "recovery.ready"
        recovery_release = Path(temp) / "recovery.release"
        recovery = start(
            args.helper,
            "recover-hold",
            root,
            recovery_ready,
            recovery_release,
        )
        wait_path(recovery_ready, recovery)

        attach_ready = Path(temp) / "attach.ready"
        attach_release = Path(temp) / "attach.release"
        attaching = start(
            args.helper,
            "participant-attach",
            root,
            attach_ready,
            attach_release,
        )
        time.sleep(0.2)
        if attach_ready.exists() or attaching.poll() is not None:
            raise AssertionError("participant crossed the held recovery gate")

        finish(recovery, recovery_release)
        wait_path(attach_ready, attaching)
        finish(attaching, attach_release)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

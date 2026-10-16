#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time


def run(helper: str, mode: str, env: dict[str, str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [helper, mode], capture_output=True, text=True, env=env,
        timeout=5, check=False,
    )


def require_prefix(result: subprocess.CompletedProcess[str], prefix: str) -> None:
    if result.returncode != 0 or not result.stdout.startswith(prefix):
        raise AssertionError(
            f"expected {prefix!r}, rc={result.returncode}, "
            f"stdout={result.stdout!r}, stderr={result.stderr!r}"
        )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--helper", required=True)
    args = parser.parse_args()

    with tempfile.TemporaryDirectory(prefix="hako-master-lock-") as temp_dir:
        root = Path(temp_dir)
        config = root / "cpp_core_config.json"
        config.write_text(
            json.dumps({"shm_type": "mmap", "core_mmap_path": str(root)}),
            encoding="utf-8",
        )
        env = dict(os.environ)
        env["HAKO_CONFIG_PATH"] = str(config)

        crashed = run(args.helper, "crash", env)
        if crashed.returncode != 0:
            raise AssertionError(f"crash fixture failed: {crashed.stderr!r}")
        require_prefix(run(args.helper, "inspect", env), "ABORT LOCKED 0 ")

    if os.name != "nt":
        with tempfile.TemporaryDirectory(prefix="hako-master-lock-zombie-") as temp_dir:
            root = Path(temp_dir)
            config = root / "cpp_core_config.json"
            config.write_text(
                json.dumps({"shm_type": "mmap", "core_mmap_path": str(root)}),
                encoding="utf-8",
            )
            env = dict(os.environ)
            env["HAKO_CONFIG_PATH"] = str(config)
            parent = subprocess.Popen(
                [args.helper, "zombie-owner"], stdout=subprocess.PIPE,
                stderr=subprocess.PIPE, text=True, env=env,
            )
            try:
                assert parent.stdout is not None
                if parent.stdout.readline().strip() != "READY":
                    raise AssertionError("zombie fixture did not become ready")
                require_prefix(run(args.helper, "inspect", env), "ABORT LOCKED 0 ")
                require_prefix(run(args.helper, "try-lock", env), "FAIL_STOP ABORT ")
            finally:
                parent.kill()
                parent.wait(timeout=5)

    with tempfile.TemporaryDirectory(prefix="hako-master-lock-live-") as temp_dir:
        root = Path(temp_dir)
        config = root / "cpp_core_config.json"
        config.write_text(
            json.dumps({"shm_type": "mmap", "core_mmap_path": str(root)}),
            encoding="utf-8",
        )
        env = dict(os.environ)
        env["HAKO_CONFIG_PATH"] = str(config)
        holder = subprocess.Popen(
            [args.helper, "hold"], stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, text=True, env=env,
        )
        try:
            assert holder.stdout is not None
            deadline = time.monotonic() + 5
            while time.monotonic() < deadline:
                line = holder.stdout.readline()
                if line == "":
                    raise AssertionError(
                        f"holder exited early: rc={holder.poll()}"
                    )
                if line.strip() == "READY":
                    break
            else:
                raise AssertionError("holder did not become ready")
            require_prefix(
                run(args.helper, "recreate-inspect", env), "OK LOCKED 0 "
            )
        finally:
            holder.kill()
            holder.wait(timeout=5)

    with tempfile.TemporaryDirectory(prefix="hako-master-lock-legacy-") as temp_dir:
        root = Path(temp_dir)
        config = root / "cpp_core_config.json"
        config.write_text(
            json.dumps({"shm_type": "mmap", "core_mmap_path": str(root)}),
            encoding="utf-8",
        )
        (root / "flock.bin").write_bytes(bytes(4096))
        env = dict(os.environ)
        env["HAKO_CONFIG_PATH"] = str(config)
        require_prefix(run(args.helper, "inspect", env), "UNKNOWN UNLOCKED 0 ")

    with tempfile.TemporaryDirectory(prefix="hako-master-lock-concurrent-") as temp_dir:
        root = Path(temp_dir)
        config = root / "cpp_core_config.json"
        config.write_text(
            json.dumps({"shm_type": "mmap", "core_mmap_path": str(root)}),
            encoding="utf-8",
        )
        env = dict(os.environ)
        env["HAKO_CONFIG_PATH"] = str(config)
        starters = [
            subprocess.Popen(
                [args.helper, "recreate-inspect"], stdout=subprocess.PIPE,
                stderr=subprocess.PIPE, text=True, env=env,
            )
            for _ in range(16)
        ]
        for starter in starters:
            stdout, stderr = starter.communicate(timeout=8)
            lines = [line for line in stdout.splitlines() if line]
            if (
                starter.returncode != 0
                or not lines
                or not lines[-1].startswith("OK UNLOCKED 1 ")
            ):
                raise AssertionError(
                    "concurrent initialization failed: "
                    f"rc={starter.returncode}, stdout={stdout!r}, stderr={stderr!r}"
                )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

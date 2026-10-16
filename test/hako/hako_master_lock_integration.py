#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
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


def require_last_line(result: subprocess.CompletedProcess[str], expected: str) -> None:
    lines = [line for line in result.stdout.splitlines() if line]
    if result.returncode != 0 or not lines or lines[-1] != expected:
        raise AssertionError(
            f"expected final line {expected!r}, rc={result.returncode}, "
            f"stdout={result.stdout!r}, stderr={result.stderr!r}"
        )


def wait_until_ready(process: subprocess.Popen[str]) -> None:
    assert process.stdout is not None
    deadline = time.monotonic() + 5
    while time.monotonic() < deadline:
        line = process.stdout.readline()
        if line == "":
            raise AssertionError(f"holder exited before READY: rc={process.poll()}")
        if line.strip() == "READY":
            return
    raise AssertionError("holder did not become ready")


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

        # The second process proves that initialization released the exact
        # Windows byte range it acquired. Each subprocess has its own 5 s
        # watchdog in run(), so a leaked lock fails instead of hanging CI.
        require_last_line(run(args.helper, "lock-unlock", env), "ROUNDTRIP_OK")
        require_last_line(run(args.helper, "lock-unlock", env), "ROUNDTRIP_OK")

        crashed = run(args.helper, "crash", env)
        if crashed.returncode != 0:
            raise AssertionError(f"crash fixture failed: {crashed.stderr!r}")
        require_prefix(run(args.helper, "inspect", env), "ABORT LOCKED 0 ")

    with tempfile.TemporaryDirectory(prefix="hako-master-lock-waiter-") as temp_dir:
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
        waiter = None
        try:
            wait_until_ready(holder)
            waiter = subprocess.Popen(
                [args.helper, "try-lock"], stdout=subprocess.PIPE,
                stderr=subprocess.PIPE, text=True, env=env,
            )
            time.sleep(0.2)
            if waiter.poll() is not None:
                raise AssertionError("waiter did not wait for the live owner")
            holder.kill()
            holder.wait(timeout=5)
            stdout, stderr = waiter.communicate(timeout=5)
            if waiter.returncode != 0 or not stdout.startswith("FAIL_STOP ABORT "):
                raise AssertionError(
                    "waiting caller did not detect owner death: "
                    f"rc={waiter.returncode}, stdout={stdout!r}, stderr={stderr!r}"
                )
        finally:
            if holder.poll() is None:
                holder.kill()
                holder.wait(timeout=5)
            if waiter is not None and waiter.poll() is None:
                waiter.kill()
                waiter.wait(timeout=5)

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
                wait_until_ready(parent)
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
        flock = root / "flock.bin"
        flock.write_bytes(bytes(4096))
        before = hashlib.sha256(flock.read_bytes()).digest()
        env = dict(os.environ)
        env["HAKO_CONFIG_PATH"] = str(config)
        env["HAKO_TEST_FLOCK_PATH"] = str(flock)
        require_prefix(run(args.helper, "inspect", env), "UNKNOWN UNLOCKED 0 ")
        require_prefix(run(args.helper, "try-lock", env), "FAIL_STOP UNKNOWN ")
        if hashlib.sha256(flock.read_bytes()).digest() != before:
            raise AssertionError("legacy flock was modified by the new runtime")

        old_writer = subprocess.Popen(
            [args.helper, "legacy-hold"], stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, text=True, env=env,
        )
        new_writer = None
        try:
            wait_until_ready(old_writer)
            new_writer = subprocess.Popen(
                [args.helper, "try-lock"], stdout=subprocess.PIPE,
                stderr=subprocess.PIPE, text=True, env=env,
            )
            time.sleep(0.2)
            if new_writer.poll() is not None:
                raise AssertionError("new writer bypassed the legacy OS lock")
            old_writer.kill()
            old_writer.wait(timeout=5)
            stdout, stderr = new_writer.communicate(timeout=5)
            if new_writer.returncode != 0 or not stdout.startswith("FAIL_STOP UNKNOWN "):
                raise AssertionError(
                    "mixed writer was not rejected as UNKNOWN: "
                    f"rc={new_writer.returncode}, stdout={stdout!r}, stderr={stderr!r}"
                )
            if hashlib.sha256(flock.read_bytes()).digest() != before:
                raise AssertionError("mixed writer test modified legacy evidence")
        finally:
            if old_writer.poll() is None:
                old_writer.kill()
                old_writer.wait(timeout=5)
            if new_writer is not None and new_writer.poll() is None:
                new_writer.kill()
                new_writer.wait(timeout=5)

    with tempfile.TemporaryDirectory(prefix="hako-master-lock-incomplete-") as temp_dir:
        root = Path(temp_dir)
        config = root / "cpp_core_config.json"
        config.write_text(
            json.dumps({"shm_type": "mmap", "core_mmap_path": str(root)}),
            encoding="utf-8",
        )
        flock = root / "flock.bin"
        flock.write_bytes(bytes(4096 + 48))
        before = hashlib.sha256(flock.read_bytes()).digest()
        env = dict(os.environ)
        env["HAKO_CONFIG_PATH"] = str(config)
        require_prefix(run(args.helper, "inspect", env), "UNKNOWN UNLOCKED 0 ")
        started = time.monotonic()
        require_prefix(run(args.helper, "try-lock", env), "FAIL_STOP UNKNOWN ")
        elapsed = time.monotonic() - started
        if elapsed >= 4:
            raise AssertionError(f"incomplete metadata rejection was unbounded: {elapsed:.3f}s")
        if hashlib.sha256(flock.read_bytes()).digest() != before:
            raise AssertionError("incomplete flock was modified by the new runtime")

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

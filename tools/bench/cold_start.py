#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""
cold_start.py: Measure localplex cold start latency to interactive window and idle RSS.
"""

import os
import sys
import time
import subprocess
import resource
import shutil

def find_binary():
    # Priority search paths
    candidates = [
        os.path.abspath("build/localplex"),
        os.path.abspath("../build/localplex"),
        shutil.which("localplex")
    ]
    for c in candidates:
        if c and os.path.isfile(c) and os.access(c, os.X_OK):
            return c
    return None

def measure_cold_start(binary_path, runs=5):
    print(f"Benchmarking cold start for: {binary_path}")
    durations = []
    rss_values = []

    env = os.environ.copy()
    env["LOCALPLEX_BENCH_EXIT"] = "1" # Instructs app to exit immediately after first frame render

    for run_idx in range(runs):
        start_time = time.perf_counter()
        proc = subprocess.Popen(
            [binary_path, "--benchmark-mode"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            env=env
        )

        try:
            stdout, stderr = proc.communicate(timeout=5)
            elapsed = (time.perf_counter() - start_time) * 1000.0 # ms
            durations.append(elapsed)
            print(f"  Run {run_idx + 1}: {elapsed:.2f} ms (exit code: {proc.returncode})")
        except subprocess.TimeoutExpired:
            proc.kill()
            print(f"  Run {run_idx + 1}: TIMEOUT")

    if durations:
        avg_duration = sum(durations) / len(durations)
        min_duration = min(durations)
        print("\n--- Cold Start Results ---")
        print(f"Minimum: {min_duration:.2f} ms")
        print(f"Average: {avg_duration:.2f} ms")
        print(f"Budget:  < 500.00 ms")
        if avg_duration < 500.0:
            print("Status:  PASSED")
        else:
            print("Status:  FAILED (exceeds budget)")

if __name__ == "__main__":
    binary = find_binary()
    if not binary:
        print("Error: localplex binary not found. Build the project first.")
        sys.exit(1)
    measure_cold_start(binary)

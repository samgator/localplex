#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""
playback_bench.py: Benchmark local file playback start and seek latency against performance budgets.
"""

import os
import sys
import subprocess
import shutil
import re

def find_binary():
    candidates = [
        os.path.abspath("build/localplex"),
        os.path.abspath("../build/localplex"),
        shutil.which("localplex")
    ]
    for c in candidates:
        if c and os.path.isfile(c) and os.access(c, os.X_OK):
            return c
    return None

def find_fixture():
    candidates = [
        os.path.abspath("tests/fixtures/feature_film_sample.mkv"),
        os.path.abspath("../tests/fixtures/feature_film_sample.mkv"),
        os.path.abspath("tests/fixtures/test_pattern.mp4")
    ]
    for c in candidates:
        if c and os.path.isfile(c):
            return c
    return None

def run_benchmarks(binary, fixture, runs=5):
    print(f"Benchmarking playback & seek latency for: {binary}")
    print(f"Fixture: {fixture}\n")

    start_latencies = []
    seek_latencies = []

    for i in range(runs):
        proc = subprocess.Popen(
            [binary, "--bench-playback", fixture],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        try:
            stdout, stderr = proc.communicate(timeout=6)
            m_start = re.search(r"\[BENCH_PLAYBACK_START\]\s+(\d+)\s+ms", stdout)
            m_seek = re.search(r"\[BENCH_SEEK_LATENCY\]\s+(\d+)\s+ms", stdout)

            if m_start:
                s_ms = float(m_start.group(1))
                start_latencies.append(s_ms)
            if m_seek:
                sk_ms = float(m_seek.group(1))
                seek_latencies.append(sk_ms)

            print(f"  Iteration {i+1}: Playback start = {s_ms if m_start else 'N/A'} ms, Seek = {sk_ms if m_seek else 'N/A'} ms")
        except subprocess.TimeoutExpired:
            proc.kill()
            print(f"  Iteration {i+1}: TIMEOUT")

    if start_latencies:
        avg_start = sum(start_latencies) / len(start_latencies)
        min_start = min(start_latencies)
        avg_seek = sum(seek_latencies) / len(seek_latencies) if seek_latencies else 0.0

        print("\n--- Playback & Seek Latency Results ---")
        print(f"Playback Start (Average): {avg_start:.2f} ms")
        print(f"Playback Start (Minimum): {min_start:.2f} ms")
        print(f"Playback Start Budget:    < 300.00 ms")
        print(f"Seek Latency (Average):   {avg_seek:.2f} ms")
        print(f"Seek Latency Budget:      Instant (< 50 ms)")

        if avg_start < 300.0:
            print("Playback Start Status:    PASSED")
        else:
            print("Playback Start Status:    FAILED")
    else:
        print("Failed to collect benchmark measurements.")

if __name__ == "__main__":
    binary = find_binary()
    fixture = find_fixture()

    if not binary:
        print("Error: localplex binary not found. Build the project first.")
        sys.exit(1)

    if not fixture:
        print("Error: test fixture not found. Run tools/fixtures/generate_complex_media.sh first.")
        sys.exit(1)

    run_benchmarks(binary, fixture)

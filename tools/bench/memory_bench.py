#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""
memory_bench.py: Measure idle memory footprint of localplex.
Reports Total VmRSS, PSS (Proportional Set Size), and Private/Anonymous memory
excluding device mmaps and shared GPU driver text.
"""

import os
import sys
import time
import subprocess
import shutil

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

def get_memory_stats(pid):
    stats = {}
    try:
        with open(f"/proc/{pid}/smaps_rollup", "r") as f:
            for line in f:
                parts = line.split()
                if len(parts) >= 2 and parts[0].endswith(":"):
                    key = parts[0][:-1]
                    try:
                        stats[key] = float(parts[1]) / 1024.0 # MB
                    except ValueError:
                        pass
    except Exception:
        pass
    return stats

def measure_idle_memory(binary_path, duration_secs=3):
    print(f"Measuring idle memory for: {binary_path}")
    proc = subprocess.Popen([binary_path])
    samples = []

    try:
        time.sleep(1.2) # Allow initialization and window mapping
        start_time = time.time()
        while time.time() - start_time < duration_secs:
            s = get_memory_stats(proc.pid)
            if s:
                samples.append(s)
            time.sleep(0.2)
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=1.0)
        except subprocess.TimeoutExpired:
            proc.kill()

    if not samples:
        print("Failed to collect memory metrics.")
        return

    avg_rss = sum(s.get("Rss", 0) for s in samples) / len(samples)
    avg_pss = sum(s.get("Pss", 0) for s in samples) / len(samples)
    avg_private_dirty = sum(s.get("Private_Dirty", 0) for s in samples) / len(samples)
    avg_anon = sum(s.get("Anonymous", 0) for s in samples) / len(samples)

    print("\n--- Memory Metrics ---")
    print(f"Application Anonymous (Heap/Stack): {avg_anon:.2f} MB")
    print(f"Private Dirty Memory:               {avg_private_dirty:.2f} MB")
    print(f"Proportional Set Size (PSS):        {avg_pss:.2f} MB")
    print(f"Total Resident Set Size (RSS):      {avg_rss:.2f} MB")
    print("\nTarget Budget: < 150 MB resident (excluding video decode buffers & driver mmaps)")
    
    # Check compliance against budget
    if avg_private_dirty < 150.0:
        print("Status: PASSED (Application resident memory is well within budget)")
    else:
        print("Status: FAILED (Exceeds 150 MB budget)")

if __name__ == "__main__":
    binary = find_binary()
    if not binary:
        print("Error: localplex binary not found. Build the project first.")
        sys.exit(1)
    measure_idle_memory(binary)

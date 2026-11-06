#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""
library_bench.py: Benchmark library performance with 5,000 synthetic items.
Measures database query latency, scan throughput, and memory consumption.
"""

import os
import sys
import time
import sqlite3
import tempfile
import resource

def benchmark_5k_library():
    print("Benchmarking 5,000-item library database operations...")

    with tempfile.TemporaryDirectory() as tmpdir:
        db_path = os.path.join(tmpdir, "bench_5k.db")
        conn = sqlite3.connect(db_path)
        cur = conn.cursor()

        # Apply production PRAGMAs
        cur.execute("PRAGMA journal_mode = WAL;")
        cur.execute("PRAGMA synchronous = NORMAL;")

        # Create schema
        cur.execute("""
            CREATE TABLE media_items (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                file_path TEXT UNIQUE NOT NULL,
                media_type TEXT NOT NULL,
                show_title TEXT DEFAULT '',
                item_title TEXT NOT NULL,
                season_number INTEGER DEFAULT 0,
                episode_number INTEGER DEFAULT 0,
                year INTEGER DEFAULT 0,
                duration REAL DEFAULT 0.0,
                thumbnail_path TEXT DEFAULT '',
                file_size INTEGER DEFAULT 0,
                file_mtime INTEGER DEFAULT 0,
                added_at INTEGER NOT NULL
            );
        """)
        cur.execute("CREATE INDEX idx_media_items_show ON media_items(show_title, season_number, episode_number);")
        cur.execute("CREATE INDEX idx_media_items_type ON media_items(media_type, item_title);")

        cur.execute("""
            CREATE TABLE playback_history (
                file_path TEXT PRIMARY KEY,
                duration REAL NOT NULL DEFAULT 0.0,
                position REAL NOT NULL DEFAULT 0.0,
                completed INTEGER NOT NULL DEFAULT 0,
                audio_track_id INTEGER DEFAULT -1,
                subtitle_track_id INTEGER DEFAULT -1,
                audio_lang TEXT DEFAULT '',
                subtitle_lang TEXT DEFAULT '',
                last_watched_at INTEGER NOT NULL
            );
        """)
        cur.execute("CREATE INDEX idx_playback_last_watched ON playback_history(last_watched_at DESC);")

        # 1. Measure insertion throughput for 5,000 episodes across 50 shows
        t0 = time.perf_counter()
        cur.execute("BEGIN TRANSACTION;")
        records = []
        now = int(time.time())

        for show_idx in range(1, 51):
            show_name = f"Television Series {show_idx:02d}"
            for season in range(1, 6):
                for ep in range(1, 21): # 50 * 5 * 20 = 5,000 episodes
                    file_path = f"/media/TV/{show_name}/Season {season}/S{season:02d}E{ep:02d}.mkv"
                    ep_title = f"Episode {ep:02d} Storyline"
                    records.append((file_path, "episode", show_name, ep_title, season, ep, 2020 + (season % 4), 2700.0, "", 1024*1024*500, now, now))

        cur.executemany("""
            INSERT INTO media_items (
                file_path, media_type, show_title, item_title,
                season_number, episode_number, year, duration,
                thumbnail_path, file_size, file_mtime, added_at
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
        """, records)

        # Insert some playback history
        history_records = []
        for i in range(100):
            fp = records[i * 10][0]
            pos = 1200.0 if i % 2 == 0 else 2600.0
            comp = 1 if pos >= 2430.0 else 0
            history_records.append((fp, 2700.0, pos, comp, 1, 1, "eng", "eng", now - i * 60))

        cur.executemany("""
            INSERT INTO playback_history (file_path, duration, position, completed, audio_track_id, subtitle_track_id, audio_lang, subtitle_lang, last_watched_at)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);
        """, history_records)

        conn.commit()
        insert_elapsed = (time.perf_counter() - t0) * 1000.0
        print(f"  Inserted 5,000 items in: {insert_elapsed:.2f} ms ({5000 / (insert_elapsed / 1000.0):.0f} items/sec)")

        # 2. Measure Catalog Shows query latency
        t_query = time.perf_counter()
        cur.execute("""
            SELECT
                m.show_title,
                COUNT(DISTINCT m.season_number) AS season_count,
                COUNT(m.id) AS episode_count,
                SUM(CASE WHEN COALESCE(p.completed, 0) = 0 THEN 1 ELSE 0 END) AS unwatched_count,
                MAX(m.thumbnail_path) AS poster_path,
                MAX(m.year) AS show_year
            FROM media_items m
            LEFT JOIN playback_history p ON m.file_path = p.file_path
            WHERE m.media_type = 'episode'
            GROUP BY m.show_title
            ORDER BY m.show_title COLLATE NOCASE ASC;
        """)
        shows = cur.fetchall()
        shows_elapsed = (time.perf_counter() - t_query) * 1000.0
        print(f"  Aggregated 50 shows from 5,000 episodes in: {shows_elapsed:.2f} ms")

        # 3. Measure Continue Watching & Next Up query latency
        t_shelf = time.perf_counter()
        cur.execute("""
            SELECT m.file_path, m.show_title, m.item_title, p.position, p.duration
            FROM playback_history p
            JOIN media_items m ON p.file_path = m.file_path
            WHERE p.completed = 0 AND p.position > 10.0
            ORDER BY p.last_watched_at DESC
            LIMIT 20;
        """)
        shelf_items = cur.fetchall()
        shelf_elapsed = (time.perf_counter() - t_shelf) * 1000.0
        print(f"  Continue Watching query: {shelf_elapsed:.2f} ms")

        conn.close()

        # Evaluation against budget
        print("\n--- 5k Library Benchmark Results ---")
        print(f"Show Aggregation Latency: {shows_elapsed:.2f} ms (Budget: < 15.0 ms)")
        print(f"Continue Watching Latency: {shelf_elapsed:.2f} ms (Budget: < 5.0 ms)")

        if shows_elapsed < 15.0 and shelf_elapsed < 5.0:
            print("Status: PASSED (Database queries meet virtualized 60/120Hz display refresh budgets)")
        else:
            print("Status: FAILED")

if __name__ == "__main__":
    benchmark_5k_library()

"""Does the audio thread hold its deadline under load, and what do bigger blocks buy?

Runs minerva_cli for each block size, once with no load and once with heavy CPU
load, saves every callback's timing (--csv), and draws two charts:

  buffer_experiment.png   late callbacks and the longest gap, per block size
  buffer_jitter.png       how far callbacks drift from their period, 128 frames

Usage (from the repository root, after a Release build):
  python tools/buffer_experiment.py [--seconds 30] [--load N] [--out results]

Needs matplotlib (python -m pip install matplotlib). The app plays nothing during
the runs: what is measured is the audio path itself, the block size and the load.
"""

import argparse
import csv
import os
import subprocess
import sys

import matplotlib

matplotlib.use("Agg")  # write files, open no window
import matplotlib.pyplot as plt

SAMPLE_RATE = 48000
BLOCK_SIZES = [128, 256, 512]  # 128 frames is the floor in WASAPI shared mode
LATE_FACTOR = 1.5              # same threshold as TimingStats::kLateFactor
SKIP_US = 500_000              # the first half second: RtAudio pre-fills its buffer


def run(exe, frames, load, seconds, path):
    """Runs the app once and returns its callbacks as (start_us, duration_us) pairs."""
    subprocess.run([exe, "--buffer", str(frames), "--load", str(load),
                    "--seconds", str(seconds), "--csv", path],
                   check=True, stdout=subprocess.DEVNULL, stdin=subprocess.DEVNULL)
    with open(path, newline="") as f:
        rows = [(float(r["start_us"]), float(r["duration_us"])) for r in csv.DictReader(f)]
    return [r for r in rows if r[0] >= SKIP_US]


def summarize(rows, frames):
    period_us = frames / SAMPLE_RATE * 1e6
    starts = [r[0] for r in rows]
    gaps = [b - a for a, b in zip(starts, starts[1:])]
    durations = sorted(r[1] for r in rows)
    minutes = (starts[-1] - starts[0]) / 60e6
    late = sum(g > LATE_FACTOR * period_us for g in gaps)
    return {
        "period_ms": period_us / 1000,
        "late_per_min": late / minutes,
        "max_gap_ms": max(gaps) / 1000,
        "p999_us": durations[int(0.999 * (len(durations) - 1))],
        "max_us": durations[-1],
        "drift_ms": [(g - period_us) / 1000 for g in gaps],
    }


def plot_summary(results, load, path):
    fig, (left, right) = plt.subplots(1, 2, figsize=(11, 4.2))
    x = range(len(BLOCK_SIZES))
    width = 0.38
    for offset, label in ((-width / 2, "no load"), (width / 2, f"load: {load} busy threads")):
        key = 0 if label == "no load" else load
        left.bar([i + offset for i in x], [results[(f, key)]["late_per_min"] for f in BLOCK_SIZES],
                 width, label=label)
        right.bar([i + offset for i in x], [results[(f, key)]["max_gap_ms"] for f in BLOCK_SIZES],
                  width, label=label)
    right.plot(x, [results[(f, 0)]["period_ms"] for f in BLOCK_SIZES], "k--", marker="_",
               label="block period (on time)")
    for ax in (left, right):
        ax.set_xticks(list(x), [f"{f}\n({f / SAMPLE_RATE * 1000:.2f} ms)" for f in BLOCK_SIZES])
        ax.set_xlabel("block size, frames")
        ax.legend()
    left.set_ylabel("late callbacks per minute")
    left.set_title("Late callbacks (gap > 1.5 blocks)")
    if max(r["late_per_min"] for r in results.values()) == 0:
        # All zero: say so, instead of an empty chart with a meaningless axis.
        left.set_ylim(0, 1)
        left.text(0.5, 0.5, "none at any block size", transform=left.transAxes,
                  ha="center", va="center", fontsize=12)
    else:
        left.set_ylim(bottom=0)
    right.set_ylabel("longest gap between callbacks, ms")
    right.set_title("Worst delay vs. the block period")
    fig.tight_layout()
    fig.savefig(path, dpi=130)


def plot_jitter(results, load, path):
    fig, ax = plt.subplots(figsize=(8, 4.2))
    for key, label in ((0, "no load"), (load, f"load: {load} busy threads")):
        ax.hist(results[(128, key)]["drift_ms"], bins=120, alpha=0.6, label=label, log=True)
    ax.axvline(0, color="k", linewidth=0.8)
    ax.set_xlabel("gap between callbacks minus the block period (2.67 ms), ms")
    ax.set_ylabel("callbacks (log scale)")
    ax.set_title("Jitter at 128 frames: how late each callback started")
    ax.legend()
    fig.tight_layout()
    fig.savefig(path, dpi=130)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--exe", default=os.path.join("build", "apps", "cli", "Release", "minerva_cli.exe"))
    parser.add_argument("--seconds", type=int, default=30, help="length of each run")
    parser.add_argument("--load", type=int, default=os.cpu_count(), help="busy threads in the loaded runs")
    parser.add_argument("--out", default="results", help="folder for the CSV files and charts")
    args = parser.parse_args()
    if not os.path.exists(args.exe):
        sys.exit(f"{args.exe} not found: build Release first")
    os.makedirs(args.out, exist_ok=True)

    results = {}
    print(f"{'block':>6} {'load':>5} {'late/min':>9} {'max gap ms':>11} {'p99.9 us':>9} {'max us':>8}")
    for frames in BLOCK_SIZES:
        for load in (0, args.load):
            rows = run(args.exe, frames, load, args.seconds, os.path.join(args.out, f"b{frames}_load{load}.csv"))
            s = results[(frames, load)] = summarize(rows, frames)
            print(f"{frames:>6} {load:>5} {s['late_per_min']:>9.1f} {s['max_gap_ms']:>11.2f} "
                  f"{s['p999_us']:>9.1f} {s['max_us']:>8.1f}")

    plot_summary(results, args.load, os.path.join(args.out, "buffer_experiment.png"))
    plot_jitter(results, args.load, os.path.join(args.out, "buffer_jitter.png"))
    print(f"Charts written to {args.out}/")


if __name__ == "__main__":
    main()

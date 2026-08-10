# PERF-CERT-059 cooperative-boundary source audit

Date: 2026-08-09

This is a read-only continuation of `PERF-CERT-059`. It audits whether the
candidate merely moves the same non-preemptible work elsewhere. It is not a
local performance measurement and does not open another implementation axis.

## Configuration and state bounds

The production parser admits maps up to 32x32, 3-8 agents, 4-10 days and at
most `max(width,height)` spots. Exact high-fuel enumeration additionally rejects
more than 16 spots and more than 8,388,608 `(mask,cell)` states. The BTC fixture
that exposed the gap has 12 spots and 1,024 cells, hence 4,194,304 states.

The four dense arrays retain their original element types and capacities:

| Array | Element bytes | Candidate chunk bytes per worker at 65,536 states | BTC full-array bytes per worker |
|---|---:|---:|---:|
| distance | 2 | 131,072 | 8,388,608 |
| patrolFuel | 2 | 131,072 | 8,388,608 |
| parent | 4 | 262,144 | 16,777,216 |
| incoming | 1 | 65,536 | 4,194,304 |
| total | 9 | 589,824 (576 KiB) | 37,748,736 (36 MiB) |

There are at most four exact workers. Therefore the candidate reduces the
sentinel-construction interval between deadline polls from at least 144 MiB
across four BTC workers to at most 2.25 MiB across four workers. Each BTC worker
performs 64 chunks. The general 8,388,608-state architecture limit changes the
number of chunks, not the 576 KiB per-worker interval.

## Deadline-path trace after initialization

- Every one of the four `reserve` calls is followed by a deadline check; the
  fourth is followed immediately by the check at the top of the first chunk.
- Dense sentinel construction checks the same absolute deadline before each
  65,536-state chunk.
- Main bucket traversal checks every 4,096 processed bucket entries. Each live
  entry expands at most six hex neighbours.
- Maximal-mask dynamic programming checks every 1,024 masks per bit and every
  1,024 masks in its final scan.
- Terminal selection checks every 16 masks, every 256 cells inside a terminal
  scan and every 16 reconstructed witnesses.
- Worker join occurs only after each worker has returned through one of these
  existing checks or completed exact enumeration; no detached work survives
  certification.

The logical return on expiration remains `supported=true`, `complete=false`
with no exact routes, matching the old code's first post-initialization deadline
exit. A completed initialization retains identical vector size, capacity,
sentinels and index order. The frozen replay gate independently confirmed
byte-identical actions on 300 recorded states.

## Residual risks deliberately not patched

1. `std::vector::reserve` itself is one allocator call and cannot be polled from
   within. The candidate separates the four allocations with checks, but only
   target-host telemetry can establish their tail cost.
2. After initialization and before bucket traversal, BTC-scale preprocessing
   constructs 101 bucket headers, 4,096 reachable-mask bits, per-cell move/spot
   metadata, 12x1,024 spot terminal distances, brand terminal distances and at
   most 8x1,024 tanker distances. These are bounded by public configuration but
   have no internal poll. Existing target telemetry localizes the large tail to
   the old dense initialization; it does not separately measure this smaller
   region.
3. Simulator/validator calls and short final comparator bookkeeping are bounded
   by agents and day steps but are not interruptible internally. No existing
   target-host counterexample attributes a material overrun to them.

These residuals are observation points, not authorization for speculative
changes. If fresh BTC telemetry still crosses the compute boundary, the next
attribution must distinguish allocation, preprocessing, traversal,
reconstruction and post-enumeration work before any new patch is registered.

## Verdict

The candidate closes the only confirmed large unpolled memory-touch interval
without changing search semantics. Static analysis does not prove target-host
latency, and it does not justify claiming the entire certification path is now
fully preemptible. Keep `PERF-CERT-059` active and uncommitted pending BTC.

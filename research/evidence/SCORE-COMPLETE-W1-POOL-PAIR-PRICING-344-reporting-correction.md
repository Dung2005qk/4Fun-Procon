# 344 reporting correction, 2026-09-08

Development measurement completed all96sides/24fixtures/384ACK/288transitions.
The automatic summary failed with KeyError road_footprint:314's narrow bridge
serializes agents, ledger and score but not the road footprint;342's comparison
requires that field. This is a reporting adapter defect, not a solver failure.
Original stderr, runner, manifest and every measured artifact remain unchanged.

Preservation: no designed function removed, reduced, deferred or disabled;
nothing deleted. Do not omit road comparison or silently fill missing values.
The canonical reporting adapter checks frozen setup identity and the full map;
only narrow maps with no road cell have a mathematically exact all-zero road
footprint. Validate the same vector against every decision's exact simulation.
Fixed roles are verified from all authoritative/final agent states. Protected
data retains its explicitly recorded roles and road footprint without replacement.
Roaded missing data, nonzero non-road traffic, identity or shape mismatches fail.

Only in-memory input normalization is added around the frozen summarizer. Its
entire comparison, causal criteria, fixed repeats, strata, tails and gate code
remain unchanged. No timed measurements or solver calls are repeated; no sample
is removed. The create-only report includes correction and RAM amendment
provenance. Tests include input immutability, roaded missing data rejection,
protected traffic divergence, score/state loss retention and invalid schemas.
The adapter/tests/note and original execution hash are frozen separately before
the corrected complete report is generated. Recompute the report exactly.

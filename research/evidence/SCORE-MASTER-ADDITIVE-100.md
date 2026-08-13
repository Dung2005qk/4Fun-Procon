# SCORE-MASTER-ADDITIVE-100

Parent: `f77c101`

Frozen manifest: `research/holdouts/SCORE-MASTER-ADDITIVE-100.csv`

SHA256: `AE7269ED4F73429C04128AFEABE50E105E55B743695852D05F76ADB6DAB39E4C`

The candidate kept canonical 32/8 retention, branch-and-bound, ALNS and baseline
F0 unchanged. Before each existing population-maintenance cut, it copied the
already-evaluated candidate set into a bounded quality-only 32/0 reservoir. The
baseline completed the existing F0 selection and certification first. With only
the remaining certification window, supplemental-only candidates were eligible
to replace it solely when the existing comparator proved strict certified
dominance. No second route search, solver, metric, fixture dispatcher or extra
budget was introduced.

The fresh manifest contains 18 development and 54 sealed holdout fixtures on a
new 3x4 perimeter topology, six new families and low/default/high fuel. A full
exact-oracle development attempt did not finish the first frontier within the
120-second probe limit and produced no score. The lingering exact-probe process
was identified by its executable path and stopped. The topology, horizon and
traffic semantics were not reduced to force completion.

Development quality was instead measured directly with source-frozen HEAD-only
binaries on the same 18 fixtures. Every chosen day plan passed the exact
simulator and independent validator at the 5000-ms logical budget. Candidate
binary SHA256 was
`88825F7B0B0AFF958CF2E1EA4DEA27EDD7CEF0CE77CE87A0BBAC0E1E58761B14`;
parent binary SHA256 was
`26F0A1B8EEF5BADED6D886761140489D1B4F8C162552042A80F8D29D22E2FF3B`.
Paired candidate-vs-parent was `0/18/0`, invalid `0/0`. All 18 full-match action
sequence hashes were identical and every candidate match reported zero
`certified-supplemental-strict-dominance` days. Local elapsed time was ignored.

100 is rejected for no development gain and no active proof takeover. The 54
holdout fixtures remained sealed; protected historical lanes, native roles and
BTC were not opened because they cannot promote a candidate that is byte-action
equivalent to its parent on development. All production/test changes were
removed and the canonical source returned to `f77c101`. The manifest and
research adapter remain solely to reproduce the negative result. Reopen this
mechanism only from fresh telemetry showing that an evicted supplemental profile
can be certified to strictly dominate the baseline inside the remaining 5000-ms
window; do not loosen dominance or tune budget allocation on consumed 095/100.

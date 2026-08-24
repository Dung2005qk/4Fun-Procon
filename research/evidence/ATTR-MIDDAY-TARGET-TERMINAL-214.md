# ATTR-MIDDAY-TARGET-TERMINAL-214 — target-terminal generator attribution

Verdict: **accepted-attribution-generator-gap**. This experiment changes no
production caller and has no promotion authority.

## Frozen inputs

Manifest: `research/holdouts/ATTR-MIDDAY-TARGET-TERMINAL-214.csv`

SHA256: `63A864027F96CC35C206B3E1B87BCF74DAEFA059EF52E887DB6BCDEC259B6195`

The six replays were already consumed by experiment 212. They are used only to
test whether terminal-blind retention hid routes that satisfy the unchanged 210
certificate.

## Query

The research-only sparse entry point uses the existing label relaxation,
`(spotMask, cell)` Pareto dominance, queue order, production minimum-spot rule,
32-route cap and 1,250,000-settled-state cap. It changes only emission: labels
are retained at the caller-supplied incumbent terminal instead of being retained
across all udon-spot terminals and filtered afterward.

Every one-agent substitution is simulated by `ExactStepSimulator`, independently
validated, and then judged by the unchanged protected certificate: equal road
footprint, same terminal state for every agent, no patrol-fuel loss, no lifetime
brand loss, no daily-distinct or servings loss, and at least one strict day-score
gain.

## Complete production-cap result

- Registered nonterminal replay-days: 48/48.
- Routes: 8,311; terminal mismatch: 0; duplicates: 964.
- Generated and dual-engine-valid plans: 7,347/7,347; invalid: 0.
- Rejected by road footprint: 7,123; fuel dominance: 86; brand: 0;
  not strict: 93.
- Certified strict improvements: 45.
- `m-4037`: 2 certificates on day 7, maximum `+1` serving.
- `m-4038`: 11 certificates on days 2/3/5/6/9, maximum `+2` servings.
- `m-4039`: 30 certificates on days 1–5, maximum `+3` servings.
- Post-210 `m-4044`: 2 certificates, days 6 and 8, each `+1` serving.
- Post-210 `m-4043` and `m-4045`: no certificate.

Authoritative clean log:
`research/evidence/ATTR-MIDDAY-TARGET-TERMINAL-214-production.log`, SHA256
`1B16099F1012769626E0CC581DC28732E307EEC82A923CC986823C7223FDAC4C`.

Empty stderr SHA256:
`E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`.

## Interpretation

Experiment 212 did not prove the accepted lane was at a same-terminal fixed
point. Its global emitter discarded masks at unrelated spot terminals before
the certificate could see an incumbent-terminal realization. The two post-210
`m-4044` witnesses falsify both its lane-vindicated conclusion and the resulting
practical-ceiling claim.

The only authorized successor is fresh-seed experiment 215: preserve the whole
accepted global-pool ascent as an order-identical prefix, then add a
target-terminal follow-up under the same protected deadline and certificate.

# SCORE-TRAFFIC-UPPER-LANE-119

Parent: `5dccb0f`

The frozen development and holdout manifest was never opened for score. Before
the first development A/B, the preregistered production-width capability gate
replayed only the already-consumed seed1700000 day-1 witness from attribution
115-118.

The proposed production sidecar generated at the unchanged width 16 had widths
`7|7|1` but oracle route mask `101`; the required route for patrol 1 was absent.
Consequently the unchanged master retained neither the exact oracle plan nor an
equivalent outcome (`exact=0`, `outcome=0`, 32 candidates, 98 nodes). The prior
attribution sidecar built from width 64 had mask `111` and retained both, so 119
incorrectly assumed that terminal projection after normal generation could
recover a route that the bounded triple constructor had never generated.

Verdict: rejected before score. Holdout SHA256
`E5E2E7ABB40F7584A016837EEF1AAD6DE5D80832E367F86EE6E6C9085ED99F76`
remains sealed and must not be reused for a changed mechanism. All production
candidate source was removed, restoring the canonical `5dccb0f` behavior. A
future attempt requires independent proof that a bounded terminal-stratified
enumeration exposes the missing route without globally widening the canonical
portfolio or reducing existing deadline work.

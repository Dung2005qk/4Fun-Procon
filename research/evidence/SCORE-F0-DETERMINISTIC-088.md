# SCORE-F0-DETERMINISTIC-088

Date: 2026-08-12

Parent: `00711ba`.

The candidate applies 087's upper-first ordering only to the existing four F0
diversity slots and only when `config.roadCells.empty()`. In that domain, the
accepted 073 theorem proves that traffic has no possible effect on transition
costs and the manifest is one deterministic world. The 12 current-quality
slots, candidate/master/W1 caps, simulator, validator, ALNS, role logic and
5000-ms hard cap remain unchanged. Road-containing maps use the parent selection
order and do not execute the additional upper prepass.

The roadless two-active-patrol evidence is inherited unchanged from frozen 087:

- development candidate-vs-parent `4/14/0`, zero invalid, tier-2 gains
  `+2,+1,+1,+1`;
- frozen holdout `8/42/4`, zero invalid, first-tier gain/loss sums `18/4`,
  maximum gain `+4`, every loss `-1`.

An independent roadless one-active-patrol protected matrix was then read only
after source freeze. All 18 development results matched parent exactly, oracle
gap hash `6c9c4ac2e6aee047`. All 54 holdout results also matched parent exactly,
hash `468afde7057cf112`. These are consumed protection fixtures and were not used
to alter the condition or queue ratio.

The full unit/validator suite passed. The frozen BTC executable SHA256 is
`B6DF85B0410EF81E77454DBE5BCE9BF0650A0558014185EE6C1124EF1F9E1E1F`.

Two fresh explicit-advanced BTC runs tested that exact binary at the 5000-ms
hard cap:

- `m-2029`: hard, 32x32, ten days, 100 steps/day, eight agents, twelve
  requested spots, six brands and low fuel. The authoritative setup contained
  63 road cells, so the new roadless intake was inactive. All 10 action ACKs
  were HTTP 200 and valid, all 10 decisions agreed with the independent
  validator, all nine transitions reconciled, emergency was zero and maximum
  solver time was 2988 ms. Replay-check rebuilt `6/60/214`; replay SHA256 is
  `0ADE1C005720A4D2525E49200A38D1F6697D09786740711DC474A0904229D7A9`.
- `m-2034`: easy, 8x8, ten days, 64 steps/day (the server maximum for that
  size), eight agents, twelve requested spots, six brands and low fuel. The
  authoritative setup still contained eight road cells, so the new intake was
  again inactive. All 10 ACKs were HTTP 200 and valid, replay-check reconciled
  all nine transitions and rebuilt `6/60/345`, emergency was zero and maximum
  solver time was 2454 ms. Replay SHA256 is
  `0B3320FAC779AD5B97328974FF7F352861C0451B8CF8BABC01BB52108E263E04`.

Rank against BTC practice bots is ignored. The UI exposes difficulty, size,
horizon, step budget, response budget, agents, spots, brands and fuel, but no
terrain editor; neither fresh setup exercised the roadless branch. Therefore
the live evidence has authority for the unchanged road path, protocol,
lifecycle, validator agreement and hard cap, not for the score gain or direct
active-path latency. The active branch remains bounded by the same internal
deadline and its quality evidence is the frozen paired roadless matrix; local
elapsed is not promoted into a performance claim.

Verdict: accepted. This is a domain-exact capability alignment, not a fixture
dispatcher: every rule-valid roadless map uses the same deterministic-world
theorem, while every road-containing map retains the parent path. Reopen only
for a new paired general-score counterexample, systematic roadless downside,
invalid/emergency, active-path BTC hard-cap evidence if a roadless live setup
becomes available, or source/binary drift.

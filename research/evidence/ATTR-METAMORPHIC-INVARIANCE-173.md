# ATTR-METAMORPHIC-INVARIANCE-173

Date: 2026-08-19  
Parent: `828ea78`  
Verdict: `accepted-gap`  
Manifest SHA256: `248F27DC4F2B468291A30BC77C253C0857D03B6B7A7473FEEF41D3C3EB9D7915`

Six frozen general development fixtures were evaluated at logical 5000 ms with
fixed all-patrol roles. Each canonical fixture was paired with rotated agent
identity, reversed spot JSON order, bijective numeric brand relabel and the
compound transform. Terrain, positions as an unordered physical set, stocks,
fuel, horizon, thresholds and generated opponent traffic were unchanged. Every
emitted plan passed the exact simulator and independent validator.

Result: 9 mismatches out of 24 pairs, invalid 0, emergency 0.

- `1730000` threshold-corridor: all four equal at `6/24/28`.
- `1730001` fuel-tight: agent rotation improved `7/19/22` to `7/21/23`;
  compound fell to `7/16/22`; other two tied.
- `1730002` high-stock: all four equal at `6/20/40`.
- `1730003` overnight: agent rotation reached `5/24/43`; spot reverse and
  compound reached `5/25/41`, versus canonical `5/23/42`; brand relabel tied.
- `1730004` balanced: all four equal at `6/24/24`.
- `1730005` rare-brand: agent rotation fell from `6/24/35` to `6/22/32`, spot
  reverse reached `6/23/36`, and brand relabel plus compound reached `6/24/38`.

An independent rerun of `1730005` reproduced all four transformed scores and
the canonical score exactly. Local timings are excluded.

Code attribution is structural. `parse_match_config` retains input spot order as
`SpotIndex`, while raw numeric brand values are sorted to create `brandIndex`.
Both arbitrary indices feed bounded planner and viability loops. Agent arrays
are likewise traversed in protocol order. The physical objective is invariant
to these renamings, but bounded candidate coverage is not.

The 173 holdout stayed sealed. Successor 174 isolates the smaller
semantics-preserving spot/brand canonicalization. Agent-order normalization is
not bundled into that candidate because it requires action-index remapping and
a substantially larger runtime boundary change.

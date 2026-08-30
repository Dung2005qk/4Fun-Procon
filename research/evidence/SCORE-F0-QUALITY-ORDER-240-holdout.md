# SCORE-F0-QUALITY-ORDER-240 holdout verdict

The one-time sealed holdout completed on the dedicated Spot VM
`udon-f0-240-0829` with exactly 216 atomic side results, zero partial files, one
`run_complete` marker, empty stderr, matching frozen hashes and safe memory.

Candidate versus parent over 108 pairs was `20/76/12`. Every first difference
was tier 3; aggregate official delta was `0/0/+57`. Gain sum and maximum were
`+231/+68`; loss sum and minimum were `-174/-42`. Both sides had zero invalid,
emergency, checkpoint, mid-day, terminal and public-window failures.

The global aggregate does not authorize promotion. The canonical 5000-ms lane
repeated the development downside at `2/47/7`; easy was `2/21/5`, medium
`0/26/2`, and low fuel `3/26/4`. The 15000-ms lane was positive at `18/29/5`,
hard was `3/23/0`, and very hard `15/6/5`. This is a public-window-dependent
policy trade-off, not a general replacement for the production order.

Parity-flipped execution also bounds causal interpretation of the aggregate
magnitude. Candidate-first pairs were `9/37/6`, net `+6` servings; parent-first
pairs were `11/39/6`, net `+51`. The 5000-ms loss-count direction remained
negative in both orders (`2/23/3` and `0/24/4`), while the 15000-ms win-count
direction remained positive (`7/14/3` and `11/15/2`). Therefore the lane split
is replicated, but the overall `+57` magnitude is partially order-sensitive.

Verdict: rejected as a global production change. The scoped source patch is
reverted, restoring the canonical stable-ID evaluation order. A successor may
reopen only from the pre-holdout development invariant that the quality order
helps with an authoritative public continuation window but harms the canonical
5000-ms-only lane. It must keep the 5000-ms path parent-equivalent, use only a
public deadline signal, freeze a fresh manifest, and prove the long-window gain
without using any consumed holdout case or threshold tuning.

Functionality-preservation answers: closing 240 removes, disables, defers or
reduces no designed production functionality because the candidate was never
promoted; the reversion restores the sole canonical production implementation
and deletes no required capability.

## Frozen evidence

- VM archive SHA256: `90D3F8465E9B011A8B293AB53BA1F033CB326F721B074975CD57BF59D40C8786`
- Combined holdout log SHA256: `B3546971ABD6ABBD1F3CB7E517E1E2981E2AC62BFA8A4A6A588EE6CA7F386EC1`
- Frozen summarizer output SHA256: `830B8E61D636BF91D55207F816753C09704E7BFB01C54E57727A98400786AF19`
- Run-complete marker SHA256: `EC29A976DBFCB6B502913B6539D19E8328B8057C66D287C2A848184B2ED76620`
- Frozen-hashes record SHA256: `2CF6700887C83F31AD504B6AD457A49675EFE8E22749691538EE7FECF472C5D4`
- Empty runner stderr SHA256: `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`

The frozen summarizer retains its historical experiment label
`PERF-DIVERSITY-SPARSE-SIGNATURE-236`; the input manifest, file names and frozen
hashes identify this output as the authoritative 240 holdout summary.

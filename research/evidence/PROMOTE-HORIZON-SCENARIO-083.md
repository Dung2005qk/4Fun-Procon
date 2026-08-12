# PROMOTE-HORIZON-SCENARIO-083

Date: 2026-08-12

Parent: `7ef36949e1b166c862a30f52ecb3bd9c9fb210ea`.

Candidate: the combined source diff from SCORE-HORIZON-072 and
SCORE-SCENARIO-073 only.

Frozen artifacts:

- manifest SHA256: `6329ADA27BCEF1EF6210D984C866309120AD2E9057D09D50F3A02BA300304C24`;
- shared score-only probe SHA256: `784ED885908E7B4D522C5DCEF4D14A1C958713B656D77D355796B527ACCC0AA1`;
- parent binary SHA256: `F495E4ED86BF3A233CBBB5A259B1A8113759981D639E518C17FD02623599B9DF`;
- candidate binary SHA256: `71B4895170D6213E2CC987E64B71E018D64F3A039E21AF6AF77B2CBAA14AEC95`.

Both binaries used the same MSVC/Ninja toolchain, manifest, exact oracle,
simulator, independent validator and internal 5000-ms per-day cap. Local elapsed
was ignored.

## Paired development

Candidate-vs-parent W/T/L was `5/13/0`. Every gain was tier 2, ranged from `+1`
to `+6`, and covered rare-late low/default, daily-choice low/default and
mountain-detour default. Parent oracle W/T/L was `7/11/0`; candidate was
`3/15/0`. Both were valid 18/18.

## One-time frozen holdout

Candidate-vs-parent W/T/L was `22/30/2` across 54 fixtures. Twenty-one gains
were tier 2 and one was tier 3. First-tier gain sum was `51`, maximum gain `+6`.
The only losses were:

- terminal-position/default seed `1310403`: parent `4/12/14`, candidate
  `4/12/13`, tier-3 `-1`;
- terminal-position/high seed `1320401`: parent `4/17/22`, candidate
  `4/16/21`, tier-2 `-1`.

Loss sum was `2` and maximum loss `-1`. No other family/fuel stratum had a loss.
Candidate oracle W/T/L was `16/38/0`, hash `468afde7057cf112`; parent was
`30/24/0`, hash `d6db972a0f27cdd4`. Both had zero invalid.

Verdict: accepted score holdout. The candidate demonstrates broad global gain
with bounded downside under the official lexicographic tuple. The holdout is
consumed. This is not commit authority: the unchanged binary must still protect
road-containing traffic, fixed/native roles, BTC-like scale and BTC target-host
5000-ms behavior. The two terminal-position losses remain explicit tail gates.

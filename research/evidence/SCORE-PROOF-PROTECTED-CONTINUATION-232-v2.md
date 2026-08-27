# SCORE-PROOF-PROTECTED-CONTINUATION-232-v2

## Frozen artifacts

- Parent: `1f30e69`
- Source archive SHA256: `B4985EE90EF5D0B6CE2F6919C7D60DACB29EF871E0452D3BEC5D2A461D68F1CF`
- Linux tournament binary SHA256: `705B524677D2A639AE7FD7776F00607410BF307AE345DA1961AD93B5A3B23701`
- Manifest SHA256: `6EF6FFA72F5CADA334D5A0C22C845DED03C1522FC743AFCD416F44B404C40513`
- Runner SHA256: `28354D9FBE2CFD82B95E2C5A9A70E87828F25478FCF26640B8DD41523165D46B`
- Summarizer SHA256: `C535F66D605D410AD4ED72DB2D6708B948F2AF0F8B07961CBE00F90B4212EA2F`

Copied VM logs matched their source hashes:

- control off: `3B132CB3C711C6D673A4297F64C03224F28288FD88641800F1D2CAD2AA2F704A`
- control on: `8F2D8887F09B6045FBB46A475BDE7FDB73673C287E7FBDE8FECE812D6DEBB652`
- development off: `156AA37DFA6DF4857C35145086B8694A67AFAA828F5986E0E36A32B7671556B4`
- development on: `2F9B2A1FA84132E327AD25CE3E3AA0D38FD166D794380C7700F8ED90605E8CD4`

All four sides completed, `runner.stderr` was empty, frozen hashes matched and
all invalid, emergency, checkpoint, midday, terminal and proof-certificate
failure counters were zero.

## Development evidence

The exact-5000-ms control was `2/22/0`, aggregate `0/0/+6`. Both score
differences occurred with zero eligible, valid or accepted proof witnesses.
The control therefore failed the preregistered exact-equivalence gate and also
measured the background cutoff variance of sequential time-bounded runs rather
than a causal candidate effect.

The 15000-ms public-window lane was `4/50/6`, aggregate `0/0/-23`. The enabled
side observed 51 eligible proof witnesses and 15 exact-valid witnesses but
accepted none. All ten score differences occurred without activation. The
largest loss was 22 servings; low fuel was net `-14`, native role net `-20`,
and very-hard maps net `-23`.

## Verdict

Rejected before holdout. The mechanism was safe but inert: the complete-proof
argmax is too stale or fails the protected takeover certificate on the current
authoritative day, and it produced no causal score gain. The sealed 108-case
holdout remains unopened. Candidate-only production, harness and test changes
must be removed. The independent correction that final-day closed-loop gates
use official lexicographic non-regression rather than terminal-transition and
componentwise-ledger equality remains required as a correctness repair.

Reopen only from a new certificate domain or proof construction that makes a
current-authoritative-day protected acceptance mechanically possible and has a
fresh unopened split. Do not retry retained frozen-scenario argmax consumption
or tune from these consumed development seeds.

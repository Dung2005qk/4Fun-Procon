# CEILING-MATCH-071 — development counterexamples

Date: 2026-08-12
Parent/champion: `7ef36949e1b166c862a30f52ecb3bd9c9fb210ea`
Manifest SHA256: `6329ADA27BCEF1EF6210D984C866309120AD2E9057D09D50F3A02BA300304C24`
Development result hash: `a95f22cc225f5cf9`

## Result

The complete full-match oracle beat unchanged HEAD on 7 of 18 development
matches and tied the other 11. HEAD never beat the oracle and both sides were
dual-valid. All seven first differences are tier 2 (total daily distinct), with
gains from `+2` to `+6`; there is no lifetime-distinct regression.

| Family | Fuel | HEAD | Exact oracle | First tier gain |
|---|---|---:|---:|---:|
| rare-late | low | 3/10/12 | 3/12/12 | tier 2 +2 |
| daily-choice | low | 3/7/9 | 3/9/10 | tier 2 +2 |
| rare-late | default | 3/10/15 | 3/12/16 | tier 2 +2 |
| daily-choice | default | 3/9/11 | 3/12/13 | tier 2 +3 |
| fuel-allocation | default | 5/14/14 | 5/16/16 | tier 2 +2 |
| terminal-position | default | 4/12/14 | 4/15/16 | tier 2 +3 |
| mountain-detour | default | 5/14/14 | 5/20/20 | tier 2 +6 |

Fuel slices are oracle W/T/L `2/4/0` at low, `5/1/0` at default and `0/6/0`
at high. This concentration is consistent with finite total-match fuel being
valued incorrectly rather than a terminal-day route gap; `CEILING-ORACLE-070`
already tied the complete terminal oracle on 144/144 independent states.

## Reproduced attribution fixture

Mountain-detour/default seed `1310500` reproduced the same score and action
trace in three consecutive targeted runs:

```text
HEAD:   5/5/5@23:f11; 5/10/10@26:f4; 5/12/12@29:f0; 5/13/13@29:f0; 5/14/14@29:f0
Oracle: 5/5/5@23:f11; 5/9/9@29:f8;  5/13/13@23:f5; 5/17/17@29:f2; 5/20/20@22:f0
```

The plans first differ on day 2. HEAD gains one extra daily distinct that day
but spends seven fuel, leaving only four; the oracle spends three fuel, accepts
one fewer current distinct, and converts the saved fuel into six more total
daily distinct by match end.

For this fixture an independent verification pass checked all 42,036 retained
daily outcomes produced by 939 `(day, position, fuel)` exhaustive enumerations.
Every outcome's final position, remaining fuel, brands, daily distinct and
servings agreed under the exact simulator and independent validator. The final
oracle witness was separately replayed as an authoritative five-day trajectory.

## Gate decision

The 54-match holdout stays sealed. This evidence opens code-level attribution
of the finite-fuel multi-day value function; it does not authorize a heuristic,
fuel-family dispatcher or production source change. A candidate must arise from
the general value/capability path and pass fresh development before this holdout
can be opened once.

# ATTR-SPARSE-ROUTE-DISCOVERY-BUDGET-245

Date: 2026-08-29

## Scope

Attribution only on consumed `m-4959` day 1, stable patrol 0. The unchanged
sparse enumerator retained at most 64 routes, and every returned one-agent
exchange was checked by the exact simulator and independent validator. The cap
ladder was fixed before execution:

`50000, 100000, 250000, 500000, 750000, 1000000, 1250000` settled states.

Replay SHA256:
`336CB23D6444668C85EFE4663F209FA5DD8561BD1EB57FD01904B93D1A6AD532`.
Research-only probe SHA256:
`05E2FEE7EB8777B41D208002D17AC98C8E17939E265E10C05785B50FC1752E87`.

## Result

| Cap | Settled | Emitted masks | Exact-valid retained | Best | Mask |
|---:|---:|---:|---:|---:|---:|
| 50,000 | 50,000 | 66 | 64 | 8/8/48 | 0xFFF |
| 100,000 | 100,000 | 85 | 64 | 8/8/48 | 0xFFF |
| 250,000 | 250,000 | 165 | 64 | 8/8/48 | 0xFFF |
| 500,000 | 500,000 | 260 | 64 | 8/8/48 | 0xFFF |
| 750,000 | 750,000 | 387 | 64 | 8/8/48 | 0xFFF |
| 1,000,000 | 1,000,000 | 500 | 64 | 8/8/48 | 0xFFF |
| 1,250,000 | 1,250,000 | 611 | 64 | 8/8/48 | 0xFFF |

The first preregistered cap already recovers the complete `0xFFF` route and
the same `8/8/48` day score. Every cap keeps the protected same-terminal best at
the recorded `8/8/44`, consistent with 244. Observed local wall times ranged
from roughly 0.1 to 0.5 seconds, but local timing has no performance or
promotion authority.

## Verdict

Accepted positive attribution: the witness is not inherently a 1.25-million-
state capability. A small bounded sparse lane can supply it without a blind cap
increase. This does not authorize source integration because one consumed BTC
fixture cannot establish prevalence or global benefit.

The only admissible next step is a fresh no-source-change prevalence sweep over
synthetic 8/9/10-team traffic fixtures, using the fixed 50,000-state lane and an
exact one-agent exchange against the unchanged parent. Only repeated gains with
bounded losses may open a separately frozen SCORE experiment.

Functionality-preservation answers: (1) no designed production functionality
was removed, disabled, deferred or reduced; (2) nothing was deleted.

# DEADLINE-LONG-166

Parent: `828ea78` (`Enforce canonical competition compute cap`)

Frozen manifest: `research/holdouts/DEADLINE-LONG-166.csv`

SHA256: `BB7244DC7798963D03E7C6E1E2864B1796D6F0AFC77FD386801503A8E9AE76BF`

## Gap

The official response window is configured per match. Production currently clamps
every explicit request to 5000 ms, so the existing canonical Long profile cannot run
when a trusted match grants 15000--60000 ms.

## Invariants

- Default or unknown response budget remains 5000 ms.
- Server `endsAt` may tighten but never enlarge the configured public budget.
- Requests at or below 5000 ms remain parent-equivalent in action, exact score,
  state transition and independent-validator result.
- One generator/master/simulator/validator/comparator path; no second solver.
- No seed, map, family, fuel, role, bot, opponent or match routing.
- Local elapsed and cutoff-dependent output are development evidence only.
- Holdout stays sealed until all development gates pass.
- No designed functionality is removed, disabled, deferred or reduced; no deletion.

## Development order

1. Preserve a frozen parent executable and prove the candidate 5000-ms lane.
2. Compare the same six general fixtures at 5000, 15000 and 60000 ms.
3. Compare one frozen BTC-scale fixture for default, low and high fuel at the same
   three budgets.
4. Attribute every difference to deadline class, search completion, exact support
   and the first official tier that differs.
5. Open no holdout unless the long lane has broadly distributed upside with bounded
   downside and zero invalid/emergency.

## Results

- Candidate at 5000 ms versus frozen parent: `0/6/0`; every final score and every
  daily action hash was identical, invalid/emergency `0/0`.
- Direct Long at 15000 ms versus protected 5000-ms incumbent: `0/4/2`. Overnight
  lost three servings (`53 -> 50`) and rare-brand lost one (`38 -> 37`).
- Direct Long at 60000 ms reproduced the same six scores as 15000 ms, including
  both losses.
- BTC-scale 15000 versus 5000: default `340 -> 346`, low fuel `405 -> 406`, high
  fuel `527 -> 527`; lifetime/daily remained `6/60`, invalid/emergency zero.
- All 30 BTC-scale decisions remained deadline-limited, proving more compute can
  still expose different candidates.

Verdict: standalone Long replacement rejected; attribution accepted. The sealed
holdout was not opened. Extra time is valuable only behind a protected 5-second
incumbent and strict same-day resend certificate.

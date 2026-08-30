# SCORE-TERMINAL-STOCK-MARGINAL-RESERVOIR-258 production closure

Date: 2026-08-31

## Frozen promotion evidence

- Development: W/T/L `5/25/0`, aggregate official delta `0/0/+13`, exact
  score/plan/state/ledger equivalence at `5000 ms`, zero safety failure or
  marginal deadline rollback.
- One-time sealed VM holdout: W/T/L `11/43/0`, aggregate official delta
  `0/0/+33`, exact `5000 ms` equivalence, no component loss, zero safety
  failure or marginal deadline rollback.
- Holdout log SHA256:
  `894BD3923E545455706308A08AE6B1A637EE841EE388B71D0802F4B74035392D`.
- Holdout summary SHA256:
  `ECA738C6DD084AB1A0A49F8220A1F104B9B77B6B3D9733CDC0C1D1C3A03DA245`.

## Production integration

The accepted switch is enabled only on the public-continuation
`ProtectedSlackRefiner` copy. The canonical main solve, role selection and
complete protected checkpoint remain capped at `5000 ms`. The integration
matrix passed W/T/L `4/26/0`, aggregate official delta `0/0/+12`, exact
`5000 ms` score/plan/state/ledger equivalence, and zero safety, deadline or
registered failure.

- Integration log SHA256:
  `2AF5397EFAAFAB189EA42518160F992E20F739F055A9FA492436CB55C75DDAB6`.
- Integration summary SHA256:
  `518D29E6335038ECF7DBB21822B323050B3A29341DF20101710BFD3686634C06`.
- Final Windows BTC binary SHA256:
  `4250FD0283D0869DDA5B15AB64DDB127AFB872937C6DF77635AAC01CC9D1508B`.
- Final binary size: `1362944` bytes.
- Unit tests: pass.

The initial target-host run exposed an operational evidence gap: the accepted
mechanism was active in the public-continuation copy, but its existing
`terminalMarginal*` diagnostics were not serialized into the BTC
`protected_slack` event. The integration added those fields and included them
in the aggregate public-continuation work/failure counters. This is telemetry
only; it does not change the plan, score, simulator, validator, deadline or
submission path.

## BTC target-host gates

Both authoritative runs used hard difficulty, exactly three BTC bots,
`10000 ms` public response windows and internal `--response-ms 5000`.
Rank is recorded only as an operational fact and is not promotion evidence.

### m-5299

- Configuration: 7 days, 24x24, 72 steps/day, 6 agents, 12 spots, 6 brands,
  low fuel.
- `7/7` HTTP action ACKs valid; replay-check reconciled `6/6` transitions.
- Maximum action response: `4703 ms`; maximum canonical decision time:
  `3377 ms`.
- Public continuation authorized `7/7`; zero deadline, failure, fallback or
  safety event.
- Terminal marginal reservoir produced no route on this fixture, with zero
  rollback or failure.
- Result: rank 1, official score `6/42/128` (operational only).
- Replay SHA256:
  `DDE69CF2D690174F7827AC6EBC2E6B389C38C0CC12E1A1A2D1CFB8B4904BFA08`.

### m-5300 — active-mechanism stress gate

- Configuration: 10 days, 32x32, 96 steps/day, 8 agents, 24 spots, 8 brands,
  default fuel.
- `10/10` HTTP action ACKs valid; replay-check reconciled `9/9` transitions.
- Maximum action response: `7395 ms`; maximum canonical decision time:
  `3376 ms`.
- Public continuation authorized `10/10`; zero deadline, failure, fallback or
  safety event.
- Terminal-day marginal reservoir evaluated `384` retained routes, generated
  `291` plans, dual-validated all `291`, and accepted one strict official gain
  in one round. There was zero marginal deadline rollback and zero marginal
  failure. `publicContinuationImproved=true` on the terminal day.
- Result: rank 1, official score `8/80/448` (operational only).
- Replay SHA256:
  `6D6B1CCB51D661506640E61E96109C1F576DD5492737579D3433315D9EBD0251`.

## Verdict

Accepted for canonical production integration. The score candidate passed its
preregistered development and one-time sealed holdout gates, production
integration preserved the `5000 ms` checkpoint exactly, and the final binary
passed target-host lifecycle, latency, validation, reconciliation, rollback and
active-mechanism telemetry gates. No designed functionality was removed,
disabled, deferred or reduced, and nothing was deleted.

Canonical production and research-closure commit: `ab3d699`.

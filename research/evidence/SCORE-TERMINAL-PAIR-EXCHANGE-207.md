# SCORE-TERMINAL-PAIR-EXCHANGE-207 — accepted

Registered 2026-08-23, accepted 2026-08-24, parent `690728a`. Frozen manifest
`research/holdouts/SCORE-TERMINAL-PAIR-EXCHANGE-207.csv` SHA256
`D9506BFC73120CB30679271992EDAA71D333E94879AF1A78E47A3CF04B740D79`.

## Mechanism

After the accepted 191 one-agent terminal sparse ascent reaches its natural
fixed point, the remaining protected terminal budget evaluates joint
replacements of two patrols' plans from the already-enumerated sparse route
pools (ordered pairs, maximal routes, hash-deduped, exact-simulated,
independently validated, strict official-lexicographic acceptance only); an
accepted pair re-enters the one-agent ascent until no pair improves or the
deadline. The parent work prefix is byte-order identical; a deadline inside
the ascent means the pair phase never runs (201 lesson). Switch:
`ProtectedSlackRefiner::enableTerminalPairExchange` — production (btc_main)
enables it, default stays off for research A/B parity.

## First development run: invalidated (environment)

Cross-binary local run finished 19/11/30 but the operator's live match
m-3986 executed entirely inside the candidate window with losses
concentrated exactly in the overlapping tier; no verdict per 028/031. (This
contamination was later attributed and generalized by
ATTR-COVERAGE-REGIME-208.)

## Causal development (same binary, quiet GCP VM): PASSED

`run_terminal_pair_207_causal.sh` + preemption-safe
`run_terminal_pair_207_resume.sh`; off log SHA256
`E0728F55BE5454B0445C8228D094071EABBB97C7E11A8E4EC261A3F08E1BD6D4`, on
`9E44DE6CEDD9FC0F4F299374E1D9538A9AF723E1E7244617AC44CD82493599ED`.

- 60 pairs: W/T/L 17/41/2, +42 servings, zero invalid/emergency, zero
  tier-1/2 change.
- 16 acceptances across 12 cases (off control 0); all 12 acceptance cases
  won (+19), none lost; both losses and five wins sat at zero acceptances
  (parent deadline jitter).
- Benefit concentrated in the target very-hard sparse tier (13/10/1),
  positive in every fuel/role/window lane.

## Sealed holdout (108 pairs, 491xxxx, same venue): PASSED

Off log SHA256
`668D419D4B7B8F957EF119BF4D3CB8D11612C101C8036D04E5353BBFFA45C624`, on
`D7AAB15BB570752E1FCAD6219715FD4BCC446B23D68C04CDDF6BAB533E067CF1`.

- W/T/L **24/72/12**, +40 servings; zero invalid/emergency/dominance
  failure; zero tier-1/2 change in all 216 runs.
- Runtime tails byte-similar: max_ms off {3024,3016,3016} vs on
  {3018,3018,3016} — the pair phase never extends the day envelope.
- Acceptance-conditional record: 18 cases / 21 acceptances (off control 0):
  **16W/2L, +30/-3 servings** (losses -1/-2, below jitter magnitude).
- All 18 remaining non-ties are zero-acceptance cases netting +11 with
  symmetric tails (+79/-68) — ambient variance. The easy-tier losses
  (0W/3L) all sit at zero acceptances; the mechanism never fires at 12-14
  spots / 4 agents (inert there, not harmful).
- Lanes: hard 8/27/1, very-hard 15/19/6; all fuel/role/window lanes net
  positive.

## BTC gate: PASSED — debt paid 2026-08-24

Production binary SHA256
`2D79A5CD0029F75E6C50D3CB1F37FE3856407364B95B0C244C92D7175F872DBF`.
Three explicit-advanced probes separated mere lifecycle validity from actual
pair-path execution instead of claiming the first green match:

- `m-4037` (hard, 3 bots, 10 days, 32x32, 100 steps, 5000 ms, 8 agents,
  12 spots, 6 brands, default fuel) finished exact `6/60/382`, 10/10 valid
  ACKs, but `terminalSparse=false` because the dense exact domain supports 12
  spots. Replay SHA256
  `34146F2651949C191BB195AD5AD81204B84B3FB4742C37BB79E5E17781756BEA`.
- `m-4038` changed only spots to 18. It finished exact `6/60/429`, 10/10
  valid ACKs, max response 3922 ms; the sparse terminal refiner improved day
  10 from 43 to 53 servings, but `deadlineReached=true` after the one-agent
  ascent, so the pair loop was not reached. Replay SHA256
  `D7740A57929B6E466538011074D5AC1D6D8B9D279F9CFFF14848CD204F0B67C0`.
- `m-4039` was the decisive sparse pair-loop gate: hard, 3 bots, 7 days,
  24x24, 100 steps, 5000 ms, 4 agents, 18 spots, 6 brands, default fuel.
  Production selected one tanker. Terminal telemetry was
  `terminalSparse=true`, `deadlineReached=false`, 3456 sparse routes, 3168
  generated and dual-valid plans, 5 strict candidates, 3 accepted sparse
  rounds, and an exact improvement from 20 to 27 terminal-day servings.
  Because the unchanged one-agent ascent returned without deadline, production
  control flow necessarily entered the enabled pair loop and completed it.
  All 7/7 actions received HTTP 200 valid ACKs; zero deadline skip, fallback,
  emergency or sparse failure; max response 3312 ms and max solver 3116 ms.
  Replay-check rebuilt exact `6/42/151`. Replay SHA256
  `54200DCF9CC5D6AD74ADF4CA47F5FB819C72FFD481993D6C4346BFD9B1C666EB`.

`m-4039` ranked fourth behind bots at `6/42/169..174`; this is a new tier-3
development counterexample but not evidence against 207, because the protected
terminal refiner added seven servings and preserved lifetime/daily. The missing
servings precede the terminal pair result and require separate attribution.
Bot rank is not promotion evidence; the authoritative 207 BTC claim is runtime,
validity and reserve safety with its pair loop live. **Debt closed.**

## Revert condition

Reproducible valid protected-parent regression, a BTC target-host reserve
violation with the pair phase live, or a future exact-valid replay showing
the protected terminal result below its certified parent. Never tune the pair
phase by seed, map, family, fuel, role, bot or opponent.

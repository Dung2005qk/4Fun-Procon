# ATTR-ROLE-IDENTITY-237

Date: 2026-08-28

This is attribution evidence only. It authorizes no source candidate, holdout,
promotion, or change to the canonical production checkpoint `18ecdd3`.

## Frozen artifacts

- Tournament binary: `build-release/udonshield_btc.exe`
  - SHA256 `43ED5815DA0880652819BF589787C11CAFDC92F4D7D313899C3256E37D570389`
- BTC replay: `artifacts/btc/m-4789.jsonl`
  - SHA256 `7AE05FC74B426B1E7802F392AB4783D6DAC576ACD69AEA360B84FEDCD68CC301`
- Counterfactual decision dumps for masks 1, 2, 4 and 8:
  - mask 1: SHA256 `5BB7768366BFC12908AF94C8E9DBC7A73D2E5201D39835385F9643BC64D0F1B3`
  - mask 2: SHA256 `9DA5F3541173C4362EC385C4B0116A6819DCF751DC71413178D730AA81AE9FCB`
  - mask 4: SHA256 `8858411457F9DFF8214103A11DB62623D03769A73DF727A27DC31967EBFB6A7E`
  - mask 8: SHA256 `A373E4FD1EC56A59DDC487CEA1C283D380BC7491F5CA6382952A81B59111B52E`
- Reversed-order repeat logs:
  - mask 1: SHA256 `850723051FF3124F5ECEDD78C88200C60C115DDC0C560C705AA8A9128E5FDC7D`
  - mask 4: SHA256 `41411A48680E2F8A93970CC1435B42C5B95B97AC9C32A5E7B63E6E8BE475D1C4`

## BTC counterexample

The 30-match hard-difficulty operational screen fixed the public response
window at 10000 ms and stopped at the first loss as preregistered.

- Match 1, `m-4788`: 32x32, 10 days, 100 steps/day, 8 agents, 32 spots,
  8 brands, default fuel, three bots. Team A ranked first at `8/80/531`.
- Match 2, `m-4789`: 24x24, 7 days, 100 steps/day, 4 agents, 18 spots,
  4 brands, low fuel, three bots. Team A ranked fourth at `4/28/110`;
  the bots scored `4/28/112`, `4/28/135`, and `4/28/144`.
- Replay validation: 7/7 action results were HTTP 200 and exact-valid, the
  independent validator agreed on every day, and 6/6 observable transitions
  reconciled. Maximum end-to-end response was 5704 ms. There was no invalid,
  emergency, deadline, public-continuation, midday, or sparse failure.

The production selector chose `PPTP`, role mask 4. Timed `replay-roles` ranked
the one-tanker assignments as follows:

| Mask | Roles | Role-rollout score | Rank |
|---:|:---:|:---:|---:|
| 4 | PPTP | `4/28/85` | 1 |
| 1 | TPPP | `4/28/81` | 2 |
| 2 | PTPP | `4/28/80` | 3 |
| 8 | PPPT | `4/27/87` | 7 |

Every listed rollout reported `rollout_complete=0`.

Same-binary closed-loop replay counterfactuals, with official score and exact
validation on every simulated day, produced the opposite ordering:

| Mask | Roles | Closed-loop score |
|---:|:---:|:---:|
| 1 | TPPP | `4/28/131` |
| 2 | PTPP | `4/28/118` |
| 8 | PPPT | `4/28/116` |
| 4 | PPTP | `4/28/102` |

The decisive pair was initially rerun in reversed order with the same binary and
again returned mask 1 at `4/28/131` and mask 4 at `4/28/102`. Later identical
mask-1 runs returned `4/28/74`, `4/28/134` and `4/28/97`, while mask 4 remained
near `102--106`. The original `+29` is therefore not a stable role-identity
effect. Experiment 239 traced the first bifurcation to shared-deadline
provisional work being allocated in stable-ID order.

## Attribution verdict

The original role-identity attribution is superseded as measurement-confounded.
The BTC loss remains valid, but these closed-loop runs do not prove which tanker
identity is generally better. They instead expose a general provisional/W1
deadline-order gap, recorded in `ATTR-TIMED-PLANNER-BIFURCATION-239`.

The replay counterfactual fixes the observed external day inputs while replacing
the team's own closed-loop agent state. It is therefore development attribution,
not independent promotion evidence and not a claim that mask 1 would reproduce
`131` in a new live match. No source mechanism is authorized from this single
opened replay.

Do not open a tanker-identity successor from this evidence. The only authorized
successor is the shared-planner quality-order mechanism registered after 239,
with a fresh unopened manifest and unchanged role policy.

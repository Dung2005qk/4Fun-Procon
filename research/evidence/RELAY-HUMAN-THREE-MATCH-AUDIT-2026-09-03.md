# Relay human-opponent three-match audit — 2026-09-03

## Scope and provenance

The user identified the three newest relay artifacts as human-opponent games:
`m-10505`, `m-10542` and `m-10543`. The replay payloads identify the opponent
only as `team-B`, so human identity is user-supplied provenance rather than a
field independently certified by the artifacts.

These files are complete runtime replays, not rows recovered from the BTC team
history page:

- `artifacts/btc/m-10505.jsonl`, SHA256
  `88854B29E02782D7AF95F87DFB4FF5AA3AABDD9C02BF61B2DB1AFEE661A9C610`;
- `artifacts/btc/m-10542.jsonl`, SHA256
  `56D1FC3DDEF9034B32E74F97A7A8AA4C961A91FE09664257638B68EE0CD68B5E`;
- `artifacts/btc/m-10543.jsonl`, SHA256
  `B29AB45F490F314B138F63698B434C5C9C2D857CA56207D76ADE56F8E0EB860C`.

## Fixture identity

All three games use the same exact core setup, normalized core SHA256
`0C99E56B5C20674209F8673ACB9EAE6C56CF1AB2A3902156ABFA8BAE1C91DE2F`
and map SHA256
`972788E6CE4440C8DF7543807EEEDE7913A7B971A7B280FDDFABF1E063A67F86`:

- two teams (one opponent), four days, 8x8 map;
- four agents at cells `1,15,29,48`;
- eight Spots at cells `0,6,13,20,26,32,44,52`, four brands;
- 32 steps per day, 60000 ms public response per day, fuel 64;
- busy threshold 5 and jammed threshold 10.

This is therefore one repeated 1v1 lane, not three independent configurations.
Official competition targets 8--10 teams, so the lane may falsify runtime or
validity but has no authority to promote, block or tune multi-team production
logic by itself.

## Exact results and safety

| Match | Our score | Opponent score | Gameplay result | Displayed rank |
| --- | ---: | ---: | --- | ---: |
| `m-10505` | `4/16/60` | `4/16/59` | win by one serving | 1 |
| `m-10542` | `4/16/60` | `4/16/60` | exact gameplay tie | 2, response-time tie-break |
| `m-10543` | `4/16/60` | `4/16/59` | win by one serving | 1 |

Aggregate gameplay W/T/L is `2/1/0`. In `m-10542`, total response time was
10529 ms versus 9154 ms; the 1375-ms tie-break difference is not a gameplay
score gap.

Fresh `build-release/udonshield_btc.exe` SHA256
`B64C7EBDBB96AA02AA0EC52620C12C9F6937FEE546470549B8F5C777A8E3CC52`
replay-checked all three files:

- 12/12 action acknowledgements are HTTP 200 and valid;
- 9/9 inter-day transitions reconcile;
- zero validator disagreement and zero emergency;
- maximum response is 3015 ms and maximum decision total is 2847 ms;
- public continuation was authorized on every day but made zero strict
  replacement, with zero continuation failure, deadline event or terminal
  rollback.

## Decision

The sample is useful positive operational evidence and a narrow human-opponent
sanity check. It is not a counterexample: canonical production never loses a
gameplay tier, and all three runs are exact-valid and deadline-safe. It also
has low generality because the exact same 1v1 fixture was repeated.

No SCORE or attribution successor is authorized from these games. Canonical
accepted experiment 258 remains the competition build. Research may reopen
only from a fresh 8/9/10-team or otherwise authoritative counterexample outside
the already-closed role, certificate, deadline-bound portfolio and high-fuel
long-horizon clusters, or from a genuinely new state-coupled invariant frozen
before source change.

Functionality-preservation answers: (1) no designed functionality is removed,
disabled, deferred or reduced; this is a read-only replay audit; (2) nothing is
deleted, so no active-equivalent replacement is required.

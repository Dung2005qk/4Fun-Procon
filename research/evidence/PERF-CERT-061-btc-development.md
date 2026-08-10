# PERF-CERT-061 BTC development

Date: 2026-08-10

## Frozen fixture and binary

- Match: `m-1862`
- Candidate BTC SHA256:
  `F452AA3936801A68C7E1209D95B5E1724AF8484F480BE44F6475B8677A22039F`
- Replay SHA256:
  `693DC792B9419937DA455BFCDEEFF89684BB8EED2C529F082DD8BC29AF6ADEC8`
- Explicit configuration: hard, three bots, 10 days, 32x32, 100 steps/day,
  configured response cap 5000 ms, eight agents, 12 spots, six brands and
  high fuel 300.

Rank is ignored. The exact score reconstructed by replay-check is `6/60/410`.

## Integrity and hard-cap result

- 10/10 action results were HTTP 200 with `valid=true`.
- Replay-check accepted all ten plans with independent-validator agreement and
  reconciled 9/9 authoritative transitions.
- Zero emergency, deadline skip, server WAIT, retry or no-POST event.
- Maximum solver total was 2339 ms. No decision exceeded either its exact
  authoritative per-day deadline or the configured 5000 ms cap.
- Remaining exact server window when each POST began was at least 1604 ms on
  days 1--9 and 2566 ms on day 10. Thus the designed 1600 ms network reserve
  was preserved in every decision.

The raw server windows at state receipt were 3580--3959 ms because the server
deadline is authoritative and may be shorter than the configured cap. This is
not interpreted as local timing or as permission to expand beyond 5000 ms.

## Attributed tail result

Future-witness certification on days 1--9 consumed 717--846 ms. Its first
global crossing remained column generation, but overrun was only 2--7 ms;
maximum nested exact overrun was 1--9 ms. Structurally this is bounded by the
existing 1024-node poll interval plus recursive state restoration/unwind. The
prior material tails of 23--381 ms globally and 69--609 ms in nested exact did
not recur, and exact team feasibility no longer continued to its 3,000,000-node
cap after cancellation.

Current-day exact diagnostics separately reported slice overruns up to 166 ms,
but the whole planner still preserved at least 1604 ms before POST. This is not
silently treated as proof that every exact subdeadline is closed; it is outside
the registered future-certification cancellation target and remains observable
in the balanced holdout.

## Verdict

Development passed. The candidate closes the attributed material certification
tail on this fresh target-host fixture without invalidity, semantic disagreement,
hard-cap breach or loss of the network reserve. The preregistered balanced
low/default/high BTC holdout may now be opened once with the unchanged binary.

# ATTR-ROW26-HIGH-FUEL-LARGE-THROUGHPUT-292

Verdict: closed as a fresh instance of the known high-fuel/long-horizon
throughput and future-state certificate boundary from experiment 270.

BTC row 26 `m-9592` was hard difficulty with exactly three bots, ten days, a
32x32 map, 96 steps/day, public 10000 ms, eight agents, 32 Spots, ten brands
and high fuel 288.  Canonical accepted production 258 selected seven Patrols
and Tanker agent 7.  It finished fourth at `10/100/460`; the bots scored
`10/100/615`, `10/100/595` and `10/100/576`.  The first official difference
is a 155-serving tier-3 deficit to the winner.

The run is operationally clean: all ten submissions were HTTP 200 and valid,
the exact simulator and independent validator agree on every day, all nine
authoritative transitions reconcile, there was no emergency and stderr is
empty.  Maximum server-measured action response was 7683 ms and maximum main
decision time was 3382 ms.  Replay SHA256 is
`CE913C3772110BE9527B3C0ACD8FF3089EC74F2EDF7A51E98625623AD84C9AD0`;
canonical binary SHA256 is
`5FA10472D46E1136E3A2CFCD87FF26DA97C64575E42E2A385F1134CB44826F01`.

Telemetry localizes the same boundary as 270.  RouteMaster was incomplete and
deadline-bound on all ten days, and the independent generator reached its
deadline on all ten.  Column generation reached its bounded deadline on six
days.  Public continuation reached its deadline on all ten days after
generating 507 plans, 500 exact-valid, but certified zero nonterminal takeover.
The accepted terminal mechanism did recover day 10 from 28 to 61 servings,
lifting the final total from the main checkpoint's 426 to 460, yet the winner
still led by 155.

Experiment 270 already observed the same public class on a ten-day 24x24,
eight-agent, high-fuel match: clean protocol, one Tanker, a 220-serving deficit,
deadline-bound candidate supply and no certifiable nonterminal takeover.
Experiments 269--281 then found many changed-state current-day gains but no
sound general future-state certificate; protected additive successors failed
their frozen gates.  Experiment 291 has now also falsified simple exact
RouteMaster factorization across all four recent counterexamples.  Repeating
role masks, sparse supply, continuation injection, cap increases or a
match-specific dispatcher would be circular or unsound.  No source candidate
or holdout is authorized.

Stdout SHA256 is
`BEAC8E350C33FCA7F98D4FAC69A092FA446DB581B2E9BA5C60E9B50175CC9635`;
empty stderr SHA256 is
`E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`.
The BTC screen may continue at row 27 with a fresh non-high-fuel configuration.
Reopen this class only from a new executable whole-future public-state
dominance relation, authoritative opponent trajectories, or an
operation-equivalent capability improvement that yields such proof on fresh
evidence.

Functionality-preservation answers: (1) no designed functionality was removed,
disabled, deferred or reduced; this is read-only attribution; (2) nothing was
deleted, so no active-equivalent claim is required.

# SCORE-W1-CLOSED-LOOP-114 BTC target-host gate

Date: 2026-08-13
Match: `m-2120`
Candidate executable SHA256: `4A285FF58BA845C699C03048CDDDAB1FA276DA8AE500FF99A12AC2DC96C33A98`
Replay SHA256: `40B85BD5D1C448A4444A1F7EAE507C718106A34583BB0F2DEB070639C67D6A12`

The authenticated BTC UI created a fresh explicit advanced match with hard
difficulty, three bots, ten days, 32x32, 100 steps/day, 5000-ms response, eight
agents, twelve spots, six brands and high fuel 3x (`300`). The frozen candidate
joined through HTTP using the authorized team bot token.

The candidate completed all ten days. Assignment and all 10 action responses
were HTTP 200 and valid with correct day acknowledgements. There were zero WAIT,
submission skips, emergency plans, invalid actions, exact deadline overruns or
hard-cap breaches. Replay-check independently accepted all ten submitted plans;
the exact simulator and independent validator agreed on every day, and all 9/9
authoritative transitions reconciled.

Target-host decision `totalMs` p95/p99/max was `2714/2714/2714 ms`. End-to-end
action response p95/p99/max was `2771/2771/2771 ms`. All values are below the
5000-ms internal hard cap. The exact frontier was supported on 64 agent-days and
complete on 62, with zero exact overrun; the response cache exact-reused a
contingency on nine days.

Final official score was `6/60/415`. The three practice bots scored `6/60/377`,
`6/60/367` and `6/60/362`. Rank 1 is recorded only as the asymmetric BTC failure
gate and is not used as strength or promotion evidence. The paired frozen
holdout and protected matrices remain the quality evidence.

The deadline profile still reports `competitionReady=false` solely because the
separate persistent p99 calibration flag remains false; as in prior target runs,
one ten-day match must not rewrite that calibration. Minimum floors passed on all
days. BTC validity, lifecycle, hard-cap and target-host performance gates pass.

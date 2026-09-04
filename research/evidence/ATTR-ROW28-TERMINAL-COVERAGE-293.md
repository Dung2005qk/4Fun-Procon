# ATTR-ROW28-TERMINAL-COVERAGE-293

## Question

BTC row 28 `m-9594` loses one unit at official tier 2: production scores
`6/29/105`, while every bot reaches `6/30`. Production covers all six brands
on days 1--4 and only five on terminal day 5. The attribution question is
whether the accepted terminal marginal neighborhood can recover the missing
brand from the authoritative day-5 root, or whether the loss was already fixed
by the physical state carried out of day 4.

## Frozen method

- Replay SHA256:
  `F5F0C8CDE7D1EF3BBE584D138E7E1BB6BD127388F139B6C869913A10EADDA933`.
- Manifest SHA256:
  `BD159558588904D7272FE972ECC17F147EFCC415BB12A21244125035A27E332F`.
- Unchanged accepted-258 research probe SHA256:
  `01329BA4EF91B02FE7A60D1D1A9E7D399BCFB0B3FD084127AC11DA1F0566A4C9`.
- Mode: `--recorded-terminal-marginal 5`, with the accepted limits of
  1,250,000 settled states, agent 1 and 32 retained routes.
- The consumed replay has attribution authority only. No plan was submitted,
  no production source changed, and no holdout was opened.

## Result

The recorded terminal root starts from ledger `6/24/99`. The accepted plan
scores day 5 at `5/5/6`, ending `6/29/105`. It claims brands 0, 1, 2, 3 and 5;
brand 4 is absent. All five Patrols enter the day with only 1--4 fuel. Four are
already effectively stationary on scoring cells; agent 1 has fuel 4 and serves
brands 0 and 2.

The unchanged terminal search settled 46 labels and emitted zero alternative
masks, zero retained routes and zero valid candidates. Global, protected,
road-equivalent and traffic-nonincreasing best all remain `6/29/105`.
Terminal sidecar telemetry is exactly zero routes/plans/acceptances with
`deadline=0`, so this is not a live public-window cutoff or a master-selection
rejection.

## Verdict

Closed as a fresh instance of the known state-coupled prior-trajectory gap from
209/260, not a new terminal candidate-supply mechanism. By the start of day 5,
production has spent almost all Patrol fuel and no accepted terminal marginal
move can restore brand 4. Repair therefore requires choosing a different
nonterminal position/fuel trajectory and proving its full remaining-horizon
dominance. Experiments 164/167/168/176/180--184/209/229--232 already show that
the obvious weaker certificates or changed-state takeovers regress or are inert.

No SCORE successor is authorized. Reopen only with a genuinely new sound
state-coupled full-remaining-horizon certificate or an operation-equivalent
capability improvement that computes such a certificate within the public
deadline. Do not tune to `m-9594`, weaken the certificate, increase caps, or
repeat the consumed penultimate/sparse mechanisms.

Functionality-preservation answers: (1) no designed functionality was removed,
disabled, deferred or reduced; (2) nothing was deleted, so no active-equivalent
claim is required.

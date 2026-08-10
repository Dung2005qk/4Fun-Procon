# SCORE-ALNS-039 evidence

Date: 2026-08-09
Parent: `afcd2da`
Verdict: rejected at first causal gate

- Frozen holdout: `research/holdouts/SCORE-ALNS-039.csv`
- SHA256: `83CB73EF1A1EB78C841E2AFB966E12FEF511A1F85EB3680EBD14E71428F43E18`
- Production diff: schedule only. At most one existing proof-guided agent sweep
  ran before the unchanged generic ALNS loop; all repair, exact evaluation,
  comparator and deadline code was reused.
- Unit suite: passed.

On frozen `m-1285` day 10, the candidate executed the intended path but failed
causally:

- parent selected `6/60/321` and matched the frozen 35-serving witness;
- candidate selected `6/60/320` and did not match the witness;
- exact seed/local remained `34/34`;
- candidate proof-guided iterations/routes/accepted/improvements were
  `6/36/6/0`;
- candidate generic ALNS iterations/accepted/improvements fell to `31/22/0`;
- candidate synthesized routes/accepted were `60/10`;
- recombination improvements remained zero.

Thus the early proof sweep consumed search opportunity without producing an
official or secondary improvement, and displaced the regular ALNS work that
created the parent's one-serving gain. This is a direct official-score
regression, not a local timing verdict. The high-fuel control, all frozen
holdout lanes and BTC were left unopened.

The production source was restored byte-identical to `afcd2da`; no commit was
created. Do not retune the number or placement of proof-prefix iterations on
`m-1285`. Reopen proof scheduling only after a different diverse fixture proves
that a specific existing proof repair can improve the incumbent before generic
ALNS.

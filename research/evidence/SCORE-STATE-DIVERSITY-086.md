# SCORE-STATE-DIVERSITY-086

Date: 2026-08-12

Parent: `00711ba`.

The candidate generalized `RouteMaster`'s existing 25% beam-diversity
signature from tanker terminal cells to every assigned agent's identity,
terminal cell and terminal fuel. It did not change quality share, beam width,
candidate/combination caps, score order, simulator, validator or deadline.

On the unchanged 18-case 085 development split, the exact gap was byte-for-byte
unchanged: oracle-vs-candidate W/T/L `4/14/0`, zero invalid, four tier-2 gaps,
maximum gain `+3`, result hash `c7324c01ff35e2e8`. The oracle plan remained
absent from all 16 F0 audits at the first divergent day. Therefore the old
tanker-only grouping is a real design limitation but not the earliest causal
loss on these fixtures. The candidate was rejected without opening holdout and
its entire production diff was removed. Reopening by changing signature fields,
slot counts or beam width is forbidden without a new counterexample proving
that grouping itself is causal.

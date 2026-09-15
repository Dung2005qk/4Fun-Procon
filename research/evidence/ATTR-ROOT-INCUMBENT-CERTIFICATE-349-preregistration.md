# 349 — feasible root incumbent, not complete-pair optimality

Registered2026-09-08 before instrumentation/execution. Attribution only. No
designed functionality removed, disabled, deferred or reduced; nothing deleted.
348 failed its unchanged prevalence gate; it is not rescued by this experiment.

In Pricing::solve, a root best Choice is updated only AFTER the child recursive
solve returns a completed memo node with a complete route suffix. Thus the root
Choice and its recursively memoized descendants describe a full feasible plan,
even when subsequent root branches exhaust the resource budget. Current code
does not reconstruct any plan until the entire root solve returns, so incomplete
SEARCH can discard a complete FEASIBLE solution. Optimality is a separate claim.

Observe only this root Choice by value; its route pointers remain owned by the
Pricing object and memo entries are immutable. Reset the observed Choice before
each new physical pair. On Exhausted, reconstruct outside measured solve while
the object is alive. No recursive solve calls, extra search, or incomplete memo
entries are permitted during reconstruction. Follow the root Choice then only
completed child memo keys; preserve original other-agent actions. Serialize and
dual-validate outside measured work. Do NOT change the actual returned witness.
InvalidWitness never grants publication authority. Expired no-root is empty.

Inputs: all24previously opened339 DEVELOPMENT requests unchanged, not a holdout
or selected loss subset. Freeze BEFORE observational source changes. For each,
fixed transition ceilings 1024,2048,4096,8192,16384,32768,65536,131072,262144.
All other limits and100ms remain unchanged.216queries plus216frozen340controls.
These are diagnostic interruptions, not recommendations to lower production caps.

Gate: >=4distinct roots in>=2families have a reconstructed strictly better full
remaining-horizon plan on exhaustion; all216 original outputs unchanged in
meaning, all completed control/candidate outputs score/plan equal; every observed
plan independently dual-valid with exact ledger and fixed-agent preservation;
zero failures. Unit controls cover incomplete child, expired/corrupt input,
complete parity, terminal/nonterminal reconstruction and every zero resource cap.

A pass permits only a new capability-preserving anytime-publication design with
validation budget charged before publication and exact original-return behavior
when no usable incumbent exists. It does not prove closed-loop score dominance,
broader traffic/tanker strength or promotion. A failure closes this boundary;
do not keep densifying cap grids or tune consumed347holdout/protected cases.

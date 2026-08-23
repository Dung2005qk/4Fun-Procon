# ATTR-PROTECTED-WAIT-FIXEDPOINT-204

Parent: `690728a`

Status: rejected as inert; research-harness instrumentation reverted.

Experiment 187 protects the canonical day decision and permits one exact
state-preserving off-road WAIT detour. The current refiner evaluates all such
single-agent replacements against the original incumbent and returns the best
strict componentwise improvement. This probe keeps that complete first-round
result unchanged, then calls the same refiner on the certified incumbent until
no strict improvement remains, the fixed round cap is reached or the existing
deadline expires.

The causal gate stopped on the already-opened `4411000` witness before touching
the low/high controls. Latest parent `690728a` finished `6/60/482` against its
in-run virtual parent `6/60/481`, with one protected takeover, 24 generated and
valid plans, one liftable plan, zero deadline/invalid/emergency events, and only
one accepted WAIT round over the full match. Calling the unchanged refiner on
the certified round-1 incumbent produced no second strict improvement. Because
round 2 was the preregistered prerequisite, `4310100`, `4310200`, fresh
development and the holdout remained unopened. No production candidate exists.

Causal log SHA256:
`31C1E1DECC1A565CBEE88C1E39F7F3121ADF375536F791F54F6B1A70A2C5BC5F`.

Frozen manifest SHA256:
`9082E0B478F9D920FB033EF77983F3F5263E5B389940842D80AE6BC92CC62B47`.

No designed functionality is removed, disabled, deferred or reduced. No
production caller, deadline, comparator, state transition or validation path is
changed. Local elapsed time has no performance authority.

Reopen only after a new independently observed day contains two compatible
strict protected detours. Do not widen routes, repeat consumed `4411000` or
schedule extra rounds without that witness.

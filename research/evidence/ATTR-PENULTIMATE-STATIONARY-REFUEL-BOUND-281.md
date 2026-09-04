# ATTR-PENULTIMATE-STATIONARY-REFUEL-BOUND-281

## Question

Can the road-state-independent penultimate proof retain terminal Patrol routes
by using a Tanker held at its public start cell as a deterministic one-step
refuel station?

## Frozen mechanism

The two consumed roots, sparse candidate and physical parent upper are exactly
those frozen for experiment 280. The candidate terminal geometry is generated
under all-Jammed roads. Tankers wait for the whole terminal day. Before a
Patrol departs any stationary Tanker cell, the compiled policy inserts one
WAIT, which triggers the official full-fuel rule. A route is truncated only if
its all-Jammed movement/refuel prefix exceeds the day or a fuel segment.

The same geometry is compiled separately for uniform Jammed, Busy and Smooth
road states and must have exact simulator/independent-validator agreement and
identical official score. This is a lower policy over public road state, not a
realized opponent suffix.

## Evidence

- Manifest SHA256:
  `A7894FF1EEA050CD867AF0511702785A7F112CB9ADABAED43C98D91599E03F3E`.
- Probe source SHA256:
  `6EEBDDD2F0169453BE161A3B651D4F3A28AC9A5244B4F951A8B0C528044C0A60`.
- Binary SHA256:
  `218C8B87CACFC3AC3BDB00A30FD6269B82394D6593D62B8C99C090330B50ADFA`.
- Runner SHA256:
  `1F99B55A9828A8AECBE802E05B840085C3CD8DC6F5FCC30E655C15C65A72A327`.
- Complete log SHA256:
  `0FB0E23571548E0F7370FE6E0AF5760FF8E35D964BC7FC33DD88819C254DF461`.

Both cases completed atomically. No Patrol was filtered or truncated; four and
five stationary refuels were used. Final candidate lowers rose from the 280
values `7/63/312`, `8/80/586` to `7/63/335`, `8/80/629`. Parent uppers remained
`7/63/356`, `8/80/675`, leaving 21 and 46 servings.

## Verdict

Closed negative, `0/2` bound closures. No production change, SCORE successor or
holdout is authorized. The penultimate robust-bound family is closed on these
counterexamples; do not tighten the consumed roots, weaken the upper/comparator,
or replay experiment 183. Resume the BTC screen for fresh evidence.

Functionality preservation: no designed functionality was removed, disabled,
deferred or reduced; nothing was deleted; production remained untouched.

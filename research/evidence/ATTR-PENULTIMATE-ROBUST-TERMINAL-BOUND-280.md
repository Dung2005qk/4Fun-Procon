# ATTR-PENULTIMATE-ROBUST-TERMINAL-BOUND-280

## Question

Can a strict sparse gain on the penultimate day be certified across the complete
remaining match without predicting opponents or reusing the realized final-day
state?

## Frozen construction

The frozen manifest contains only consumed replay roots `m-6134` day 8 and
`m-6213` day 9. The candidate is the best componentwise-strict current-day plan
from the fixed 50,000-state/64-route sparse frontier.

The candidate terminal lower uses only its exact final agents and public match
configuration. Opponents are removed and all roads are Jammed. The canonical
terminal solver supplies a geometric plan; all Tankers wait, and any Patrol
whose route touches a stationary Tanker or requires refueling also waits. The
remaining paths are recompiled to fill the day and must produce the same score
with exact simulator/validator agreement under uniform Jammed, Busy and Smooth
roads. Because any mixed road state lies between those movement costs, the same
geometric no-refuel paths remain feasible and visit the same Spots.

The parent upper sets lifetime and terminal daily coverage to the total brand
count and caps terminal servings by `sum_spot min(stock, patrol_count)`. This is
a physical relaxation of all movement, fuel, timing and traffic constraints.

## Evidence

- Manifest SHA256:
  `A7894FF1EEA050CD867AF0511702785A7F112CB9ADABAED43C98D91599E03F3E`.
- Frozen probe source SHA256:
  `126AABACD8AF60AE5627C6782D81AD13B60014B35CB1E40D946C3EEC1E0536F5`.
- Frozen binary SHA256:
  `CAE28CFABB1E942A20865308A5B1DF8E7ACCFC8AB45CD307693538A6B17D0BBE`.
- Runner SHA256:
  `F2FA2E22E0DF645219442BF16AEB4855393625F3E476770DC364700A7B914999`.
- Complete log SHA256:
  `63C9E311CEA8A451C23FBB339A9A5ED4907370EB94A65360317BAE0ECD919BBD`.

Both cases completed atomically with zero invalidity or validator mismatch.
Current sparse gains were `+2` and `+6` servings. Robust terminal lower scores
were `7/63/312` and `8/80/586`; parent upper scores were `7/63/356` and
`8/80/675`. The serving gaps were therefore 44 and 89.

## Verdict

Closed negative, `0/2` certified roots. The physical upper is sound but too
loose. No production change, SCORE successor or holdout is authorized. The
bound must not be weakened after observation. A successor requires a separately
frozen tighter sound state-coupled proof and must not repeat the inert experiment
183 consumer.

Functionality preservation: no designed functionality was removed, disabled,
deferred or reduced; nothing was deleted; production was untouched.

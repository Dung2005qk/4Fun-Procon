# SCORE-TANKER-SHARED-REFUEL-COLUMNS-157

Date: 2026-08-14
Production parent: `cf7e4b4`
Frozen manifest: `research/holdouts/SCORE-TANKER-SHARED-REFUEL-COLUMNS-157.csv`
SHA256: `02BAAB9408D38A0E77A7D1FD9E1F4551EC0833FBFF528DAA5780F1EE614B5B3F`

## Registered gap

156's independent whole-plan caravan improved fixed low-fuel fixtures but caused
a reproducible role-selection loss. At both fixed masks candidate and parent
tied, proving the loss came from exposing a candidate to the reduced role
rollout under a different representation than the production route/master.

The canonical master already accepts multiple patrol columns whose timestamped
`requiredRefuels` are covered by one selected tanker's `providedRefuels`.
Generation does not expose that capability: every split rendezvous currently
creates a different pair-specific `escortGroup` and tanker column.

## Frozen mechanism

Keep every parent column after the unchanged pruning pass. Add only bounded
patrol columns around existing exact independent tanker columns. A patrol reaches
its earliest fuel-feasible tanker action boundary, follows the same exact
timeline for at least one step, and either follows the provider suffix or
detaches to a target ordered by public official-priority inputs. Every shared
step becomes an exact refuel requirement. This lets one unchanged tanker column
cover different patrol joins/detaches through the existing master constraints.

If the tanker starts on a spot, an additional provider shifted by the public
overnight `WAIT(1)` may be emitted while the unshifted provider remains. This is
the canonical representation of the already-proved overnight action, not a
map/family guard.

No functionality is removed, disabled, deferred or reduced; nothing is deleted.
Parent columns remain active and are not re-pruned. The official comparator,
exact simulator, independent validator, master certification, role logic and
5000-ms internal hard cap remain unchanged. Local elapsed has no promotion
authority; BTC target-host is final for performance.

## Gate state

Unit suite passed. The consumed anchor stayed final `3/12/15` and day 1
`3/3/3`. New canonical patrol columns were exact-valid but reached at most one
serving. A research-only no-deadline master audit visited 278 combinations and
still found best `3/3/3`. Eligible independent tanker providers were exactly
single-leg-plus-wait routes (`2.-14`, `0.-14`, `5.-14`, `WAIT(16)`), so the
missing multi-waypoint timeline never existed for the working event constraints
to share.

Verdict: rejected before fresh development. Holdout sealed and unopened. Do not
retune join/detach or provider count. Successor 158 must materialize the already
attributed bounded multi-waypoint tanker construction as canonical columns.

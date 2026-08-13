# CEILING-TANKER-TERMINAL-138

Date: 2026-08-13  
Production parent: `5dccb0f`  
Frozen manifest: `research/holdouts/CEILING-TANKER-TERMINAL-138.csv`  
SHA256: `7D966C853B0B13B0E2E9223310E540B9BC6E869251711ABEA1EB6ECC519FD2A1`

CEILING-ORACLE-070 already tied complete terminal optimization on 144 all-
patrol fixtures. Its remaining explicit uncovered domain is tanker/refuel
coupling. This independent sweep freezes 18 development and 54 sealed holdout
terminal-day fixtures with two patrols, one tanker, four spots, six structural
families and low/default/high fuel.

The research oracle must be a complete joint step DP, not an independent-route
composition. State and transitions preserve move-source costs and durations,
fuel debit at patrol move completion, claims and stock before refuel, per-patrol
one-visit semantics, tanker movement without fuel, consecutive co-location
refuel and terminal co-location refuel. Its reconstructed witness must agree
between the exact simulator and independent validator including claims, final
states and trace. HEAD uses logical 5000 ms; local elapsed is not performance
evidence. Production source remains unchanged and holdout stays sealed until
the development verdict.

The exact score quotient canonicalizes every `wait(N)` to `N` consecutive
`wait(1)` actions. Position and co-location are identical at every step and the
same successor action remains available at step N. A completion at an unvisited
spot can only consume the same team-owned stock earlier; it cannot reduce team
brands or servings, and `wait(N)` itself must mark that patrol visited before
the patrol becomes free. Thus the canonical sequence weakly dominates the long
wait for the official team objective while retaining an official action trace.
No movement, timing, fuel, stock, visited or refuel state is otherwise merged.

The DP also applies exact team-score dominance only when position, pending
actions, remaining stock and prior-step co-location are identical. A state with
componentwise no-less patrol fuel and per-patrol visited sets that are subsets
can reproduce every continuation of the dominated state. Once a spot has zero
stock, its visited bits cannot alter any later score or transition and are
canonicalized away. These rules remove redundant histories without deleting a
potentially optimal continuation.

An initial adapter diagnostic used an eight-cell corridor instead of the
preregistered connected 3x3 component and produced `8/10/0` exact-vs-HEAD. It
was discarded before attribution or holdout because its geometry violated the
registered development domain. The canonical adapter uses all nine walkable
cells of a 3x3 footprint; no result from the discarded run has gate authority.

## Canonical development result

The connected-3x3 sweep completed exact-vs-HEAD `4/14/0`, with zero invalid or
incomplete cases. Low fuel was `4/2/0`: rendezvous, stock-contention,
rare-brand and delayed-claim each gained exactly two servings at tier 3.
Default and high fuel were each `0/6/0`. Exact witnesses agreed between the
simulator and independent validator including score, claims, trace, footprint
and final states. This is a structured low-fuel terminal coupling gap, but not
a production candidate. The 54-case holdout remains sealed pending causal
attribution and a bounded fixture-independent mechanism.

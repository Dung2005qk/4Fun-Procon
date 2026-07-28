# Champion Strategy Research

## Fair Comparison Boundary

HEXUDON is a current NAPROCK 2026 contest section, so there is no prior HEXUDON
world-champion implementation to copy. The closest mature evidence base is
vehicle routing and team-orienteering competition research with standardized
hardware, hidden final instances, and exact common scoring.

The local comparison preserves the same constraints:

- identical generated map and exogenous opponent stream per strategy;
- endogenous traffic from each strategy's own prior actions;
- identical wall-clock budget;
- exact lexicographic score;
- exact simulator plus independent validator;
- fixed development splits and one untouched future holdout;
- no infeasible-population shortcut because every submitted plan must be valid.

## Champion Patterns

The official DIMACS 12th VRP Challenge reports that all CVRP and VRPTW
finalists used metaheuristics. Four of the top five CVRP solvers were based on
Hybrid Genetic Search. The winning VRPTW solver also used HGS. The winning
CARP solver, ALNS++, combined ALNS with set covering over generated routes and
then continued ALNS in promising regions.

Primary sources:

- https://dimacs.rutgers.edu/index.php/news_archive/challenge/
- https://dimacs.rutgers.edu/index.php/programs/challenge/vrp/results/
- https://dimacs.rutgers.edu/index.php/programs/challenge/vrp/papers-videos/
- https://dimacs.rutgers.edu/index.php/programs/challenge/vrp/cvrp/

HGS contributes population diversity, recombination, local search, and SWAP*:

- https://arxiv.org/abs/2012.10384
- https://wouterkool.github.io/publication/hgs-vrptw/

Other primary challenger families reviewed:

- Slack Induction by String Removals:
  https://doi.org/10.1007/978-3-032-27242-3_22
- Iterated local search with path relinking:
  https://arxiv.org/abs/2205.12082
- Adaptive iterated local search:
  https://arxiv.org/abs/2012.11021

## Architecture Mapping

| Champion mechanism | Existing equivalent | Decision |
|---|---|---|
| set-cover/set-packing route recombination | exact lexicographic route master | already production |
| ALNS to set-cover to further ALNS | route-pool feedback challenger | tested; held-out neutral, rejected |
| SWAP* cross-route exchange | exact master recombines per-agent routes globally | already stronger and exact within the pool |
| HGS diversity management | score-only ALNS survivor truncation | tested conservatively; no full benefit, rejected |
| path relinking | master recombination plus ALNS mutation | high semantic overlap; no extra layer added |
| infeasible subpopulations | none | deliberately rejected by fail-closed validity contract |
| SISR ruin and recreate | synthesized single-route repair | promising only if a future profile proves route-generation starvation |

## Final Technical Verdict

The strongest imported idea, iterative ALNS/master feedback, genuinely improved
the weaker rolling HALNS baseline and looked significant across development
maps. It did not generalize against full UDON-SHIELD on the frozen future
holdout. Full UDON-SHIELD already connects ALNS, route-pool augmentation, exact
global recombination, multi-day viability, scenario shielding, traffic
sensitivity, and risk-aware candidate profiling; the later layers absorb most
of the extra candidate value.

Adding HGS-style diversity increased search breadth but also exact simulation
and profiling work. It did not improve the full architecture and therefore
failed the generality/complexity gate.

The production recommendation is to retain the current single-pass pipeline.
Further complexity is justified only by evidence of route-generation
starvation on new official fixtures, not by further tuning on any opened split.

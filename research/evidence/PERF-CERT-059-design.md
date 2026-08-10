# PERF-CERT-059 design

Date: 2026-08-09

Parent is the frozen `DEADLINE-ALIGN-056` source candidate layered on the
canonical cardinality-first score candidate. No `DEADLINE-ALIGN-056` BTC sample
was consumed. `ATTR-CERT-058` identified a separate certification boundary hole
that exact deadline plumbing alone does not structurally remove.

The only proposed source change is cooperative initialization of the four dense
state arrays in `enumerate_exact_high_fuel_routes`. The fixed chunk contains
65,536 states, so one worker initializes at most 576 KiB and four workers at
most 2.25 MiB between deadline polls, independent of seed, map identity, bot or
fuel threshold. The arrays retain identical capacities, types, sentinel values,
indices and traversal order.

The candidate may not alter role selection, route graph, dominance, settled
state cap, worker count, candidate/profile limits, official comparator,
certification fallback, network reserve, submission guard or exact-deadline
formula. Local elapsed time has no authority. Any semantic drift closes the
candidate before BTC.

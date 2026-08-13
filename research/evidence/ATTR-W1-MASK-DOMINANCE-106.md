# ATTR-W1-MASK-DOMINANCE-106

Parent: `f77c101`

On consumed CEILING-CYCLE-PATROL-095 seed 1600000 day 2, the exact oracle plan's
individual route state was compared directly with complete
ExactOrienteeringReachability before RouteColumn pruning.

Agent 0, whose route is missing from W1, visits only spot mask `4` and finishes
at cell 26 with fuel 6. That mask is absent from both `maximalRoutes` and
`terminalVariants`. A strict reachable superset mask exists, but no superset
route preserves the same terminal cell and fuel. Enumeration is complete.

Agent 1 visits mask `63`, finishes at cell 26 with fuel 4, and its route mask is
present in both maximal and terminal sets; no strict superset exists. This
matches the generated portfolio mask `011` exactly.

106 closes as accepted structural attribution. The single-agent projection to
inclusion-maximal spot masks is not a valid horizon dominance relation when the
superset changes terminal state. Keeping every mask blindly could grow
exponentially, so no source candidate is authorized yet. The next read-only
test must establish whether the existing candidate-specific FastViability upper
can rank the exact joint day-2 terminal class on the complete day-outcome
frontier; if it cannot, route-frontier expansion has no justified value.

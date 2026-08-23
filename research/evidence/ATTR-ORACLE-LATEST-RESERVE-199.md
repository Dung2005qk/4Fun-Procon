# ATTR-ORACLE-LATEST-RESERVE-199

Parent: `690728a`

Manifest SHA256:
`DF7EF5A47271F4F171A2CA673F37F38384F499A0770D387116DD3608CE48984F`

The accepted production lifecycle was reproduced on the two completed
experiment-185 witnesses under maximum-dwell, minimum-dwell and status-toggle.
Each policy ran once with the 5000-ms authoritative window, which leaves the
accepted 1100-ms protected submission reserve, and once with the full internal
5000-ms protected-refinement cap exposed by a long authoritative window.

Seed `1721100` remained `5/17/17`, `5/16/17`, `5/17/17` across the three
policies in both window modes. Its exact oracle is `5/17/18`. Seed `1721200`
remained `5/19/22` under every policy and both window modes. Its exact oracle is
`5/20/21`. Every run was valid and retained the same plan hash between short and
long modes. Protected generation, valid protected plans, takeovers, solver
deadline days and protected deadline days were all zero.

The extra accepted refinement interval therefore cannot touch either dense
six-spot gap. Both exact counterexamples survive latest production, and the
cause is not insufficient protected-refiner time.

Result log SHA256:
`E0C732B71F5144C1BC70A11E2D40C76B7B0FC515FE0C122B71A605D42EA91A55`


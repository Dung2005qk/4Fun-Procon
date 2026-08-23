# ATTR-ORACLE-ROOT-CAUSAL-202

Parent: `690728a`

This consumed-only attribution compares the completed experiment-185 exact
day-1 root for seed `1721100`, `3.2.2.2.0.1.5.-3|-16|-16`, with current
production's root, `3.2.2.2.0.0.2.-3|-16|-16`. It inspects exact-support,
portfolio and master retention, current and certified scores, then freezes only
the exact day-1 root and returns days 2--4 to unchanged production under the
three registered causal opponent policies.

No production behavior, fixture, policy, score, deadline, simulator or
validator changes. No designed functionality is removed, disabled, deferred or
reduced, and nothing is deleted.

Manifest SHA256:
`1734EE65A8D79F6388E30799CEA5058B05C33AFBBED452177BCC80046EC193E7`

The exact root is supported and complete for all three agents (`111`), appears
in the exact portfolio, survives the ordinary master and is retained as a
master outcome. The current day score is tied at `5/5/5`; both roots are
certified, neither profile dominates, and both have the same `5/20/24` upper
bound. The exact root has the weaker lower profile `5/16/17` versus the parent
root's `5/17/17`.

Forcing only the exact root and returning days 2--4 to unchanged production
finishes at valid `5/17/17` under maximum-dwell, minimum-dwell and status-toggle.
This matches or improves current production (`5/17/17`, `5/16/17`, `5/17/17`)
but does not reach the exact oracle's `5/17/18`. The missing serving therefore
requires a different later suffix; root generation, exact support, portfolio
retention and master selection are not the missing boundary.

Verdict: `accepted-attribution-no-source-candidate`. The second completed 185
witness independently confirms the same coupled multi-day suffix/evaluator
class as experiment 200. A root-only rule cannot close the gap and is not
authorized by these consumed fixtures.

Result log SHA256:
`E73175319047A24852B0EEFFAD6874BE1B52DB89527946D11CCC79B803540CFC`

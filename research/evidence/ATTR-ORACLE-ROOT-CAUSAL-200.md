# ATTR-ORACLE-ROOT-CAUSAL-200

Parent: `690728a`

Manifest SHA256:
`49B2B829E1D629BFE201976F09873DFEDBF714E9BE1EA07ED5BE684138D71C7E`

The experiment-185 exact day-1 root for consumed seed `1721200` is
`3.2.2.2.0.0.5.5.-2`; current production selects
`3.2.2.2.0.1.5.5.5`. The exact root was dual-valid and was already present in
the production exact portfolio: all three agents were supported and complete,
the master retained both the exact plan and its outcome, and wider master caps
did not remove it.

The exact root has current score `5/5/5`, certified lower/upper
`5/19/19 .. 5/20/31`. The production root has current score `5/5/6`, certified
lower/upper `5/19/20 .. 5/20/31`. Neither dominates the other, but the exact
root gives up one current serving and one certified lower-bound serving without
an upper-bound advantage. Candidate generation, master retention and F0 upper
diversity are therefore not the missing boundaries.

Forcing only the exact day-1 root and returning all later decisions to unchanged
production reached `5/20/21` under maximum-dwell and status-toggle, matching the
exact oracle. Under minimum-dwell the same forced root with the production
suffix reached only `5/18/21`, below both current production `5/19/22` and the
exact oracle `5/20/21`. The exact oracle's minimum-dwell policy has a different
suffix.

The residual gap is a coupled multi-day closed-loop suffix/evaluator gap. A
root-only selection rule would create a tier-2 regression, while experiments
180--182 already showed that static certification and local penultimate
neighborhoods cannot prove closed-loop safety. No production candidate is
authorized. Reopen source work only with a general multi-agent multi-day
closed-loop evaluator that fits the 5000-ms cap and first demonstrates a fresh
timing-independent witness; never route by this fixture, policy or plan.

Result log SHA256:
`39E59B43BA62BA5D1E0F20D82CA85D877C330D929EAC53F7E2C55CC42F8825F3`


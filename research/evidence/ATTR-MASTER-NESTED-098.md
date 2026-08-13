# ATTR-MASTER-NESTED-098

Parent: `f77c101`

This read-only attribution used two separately built historical-harness binaries:
the unchanged Normal master population of 32 and the rejected 096 population of
48. Both ran fixed all-patrol roles with a 5000-ms logical budget. Only the two
consumed causal protection blockers were inspected; elapsed local time was not
used as performance evidence. Temporary output identified each F0 record by a
stable-plan hash and reported current score, provisional lower bound, valid upper
bound, certified lower bound, terminal state, certification role and disposition.

On overnight seed 830021, the parent selected hash
`12994778012826007410`. Its day-1 record was certified and selected, with
current/lower `5/5/11`, valid upper `5/25/55`, terminal cells
`20|35|20|46` and fuel `17|11|9|16`. The 48-population binary selected hash
`7043843721048254636` with the same reported official bounds but terminal cells
`5|35|20|20` and fuel `20|12|9|12`. The parent-selected hash was absent from
all 16 candidate audit records in the wider binary. The wider upstream reservoir
therefore removed a future-useful parent witness before final certification.

On rare-brand seed 830029, the parent selected hash
`9046975828446139618`. Its day-1 record was certified and selected, with
current/lower `5/5/9`, valid upper `5/20/36`, terminal cells `15|55|55|1`
and fuel `9|3|5|10`. In the 48-population binary, the identical hash survived,
was certified and carried the incumbent W1 role, but was left
`certified-not-selected`. Hash `16946503130334369313` was selected instead.
It had the same current, provisional lower, valid upper and certified lower
scores, but terminal cells `6|55|55|1` and fuel `10|3|5|10`. The match later
regressed from parent `5/20/31` to candidate `5/16/28`.

The code path explains the second result. Certification first removes only a
candidate for which another profile passes `certified_dominates`. Remaining
undominated candidates are selected lexicographically by scenario quantiles,
survival signature, certified lower bound and current score, followed by terminal
slack, traffic-safety fields and stable ID. Thus `certified-not-selected` does
not mean that the selected plan was proved superior. Here the reported official
bounds did not separate the two terminal states, and a downstream tie-break
changed the action despite no strict dominance proof.

098 therefore closes two causal failure modes of the non-nested 096 expansion:
the wider reservoir can erase the parent witness, and it can replace a surviving
certified parent witness inside an unresolved tie. A semantically defensible
successor must keep the canonical parent reservoir/ALNS/F0 lane unchanged and
permit a supplemental lane to take over only when it strictly certified-dominates
the selected parent profile. The consumed blockers have attribution authority
only and cannot promote or tune that successor. Any implementation requires a
fresh preregistered development split and sealed holdout.

# SCORE-EXACT-SUFFIX-BEAM-203

Parent: `690728a`

Status: rejected and source reverted. The experiment protected the existing
complete greedy exact-bundle witness and tested an additive deterministic beam
whose three alternatives changed only the first detailed suffix-day bundle;
later days retained the existing greedy exact-bundle selection.

The consumed causal gate produced no change on either known completed oracle
witness. Seed `1721100` remained `5/17/17`, `5/16/17`, `5/17/17` under the
maximum-dwell, minimum-dwell and status-toggle policies. Seed `1721200`
remained `5/19/22` under all three policies. The results and plan hashes were
identical with protected reserves `1100 ms` and `0 ms`; no takeover, generated
plan, deadline or validity event occurred. Therefore a first-suffix-day beam
followed by the current greedy continuation does not reach the coupled
multi-day gap demonstrated by experiments 200 and 202. Fresh development and
the sealed holdout were not opened.

Causal log SHA256:
`47C20C9A7001AB7ECFBCD6DCBF61C7FAC5C9FABD56D4BAD5279B87D171CE7327`.

Manifest SHA256:
`D2F4013BF79BD5CB32AB4EF9C6A4203B9BEC35DB2159183E23CD438B9991FE4D`

No designed functionality is removed, disabled, deferred or reduced. No
production dispatcher, score, simulator, validator, scenario or deadline policy
is changed. The `5000 ms` hard cap remains authoritative. The production source
was restored byte-for-byte to parent `690728a` after rejection.

# SCORE-WITNESS-UPPER-SKIP-080

Date: 2026-08-12

Parent: `SCORE-WITNESS-DUAL-079` research diff on champion `7ef3694`, with
accepted research diffs 072 and 073.

The candidate skipped both optional 079 rollouts only when the retained baseline
scenario score was lexicographically at least its authoritative
`scenarioValidUpperBound`. Since 079 could replace a witness only by a strictly
greater exact official score, every skip was semantically dead work by proof.

One preregistered run used the already-opened 18-case CEILING-MATCH-071
development split under the production 5000-ms internal cap:

```text
summary,split=development,cases=18,oracle_wins=3,ties=15,head_wins=0,invalid=0,tier1=0,tier2=3,tier3=0,max_gain=3,result_hash=6c9c4ac2e6aee047
```

This is exactly the 072+073 and 079 result. No prior tie reopened and no invalid
appeared, but no useful optional trajectory became observable. Therefore the
registered improvement gate failed. The frozen holdout with SHA256
`6329ADA27BCEF1EF6210D984C866309120AD2E9057D09D50F3A02BA300304C24` remained
sealed. BTC was not run because bot rank cannot attribute optional-work
completion when the candidate has no exact development output change.

Verdict: rejected. The 080 guard and all 079 optional source were removed; 072
and 073 remain. Reopen only on a new independent counterexample, not by tuning
cap, beam, ordering or skip conditions on these opened fixtures.

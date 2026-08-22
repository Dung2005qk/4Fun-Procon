# RUNTIME-PROTECTED-RESERVE-197

Verdict: accepted.

Parent: `f9c001967e597626303a8c22f47a46b2fbdaa04b`.

Frozen manifest SHA256:
`4A56330C4D3EA548FD522F7B21B0838521F90EC62C025EFE0ECE6F57D34AAD38`.

## Mechanism and safety contract

Role selection and the canonical day solver retain the parent 1600-ms deadline
calibration. After the selected day plan passes the exact simulator and the
independent validator, only the already accepted protected wait-detour refiner
or terminal sparse coordinate refiner may use the remaining interval before

`min(received_at + 5000 ms, authoritative_action_deadline - 1100 ms)`.

The parent plan remains immutable. A deadline, failure, invalid plan,
non-improvement, or failed nonterminal transition/ledger dominance returns the
parent plan. The outer server window never raises the internal compute cap above
5000 ms.

No designed functionality is removed, disabled, deferred, or reduced. No
production implementation is deleted or duplicated.

## Frozen holdout

The 108-case holdout covered 14/20/26/32 maps, low/default/high fuel,
fixed/native roles, 12/14/18/24/30 spots, and six independent structural
families. Candidate versus the protected parent was:

- overall: `83/25/0`, with `+1021` servings, minimum delta `0`, maximum delta
  `+33`;
- easy: `9/15/0`, `+16`;
- medium: `17/7/0`, `+156`;
- hard: `30/0/0`, `+374`;
- very hard: `27/3/0`, `+475`;
- fixed roles: `51/9/0`, `+628`;
- native roles: `32/16/0`, `+393`.

All 83 wins first differed at tier 3. Lifetime and daily-distinct deltas were
zero, so no lower tier was traded for servings. Every fuel profile, family, role
mode, difficulty and spot-count stratum contained either a gain or an exact tie,
with no loss. There were zero invalid plans, emergencies, dominance failures,
or refiner failures. Four terminal slices reached their local deadline and kept
the certified incumbent. The maximum diagnostic local runtime was 3046 ms;
local timing has no performance authority.

Holdout log SHA256:
`F4F0C53F79DAC114618C9ED6DEA85CF0DAFD180D6C72FE54BE1E396CB3DBA00E`.

## BTC target-host gates

Authenticated `m-3927` used a 15000-ms public outer window. All 10 submissions
were accepted and valid; maximum server response time was 3588 ms. The protected
parent counterfactual scored `6/60/552`; the submitted refined path scored
`6/60/566`. Two days accepted a protected refinement. One external transition
mismatch was retained as lifecycle evidence and was not attributed to the
refiner.

Authenticated `m-3928` used the canonical 5000-ms outer window. All 10
submissions were accepted and valid; maximum server response time was 2963 ms,
and all nine authoritative transitions reconciled. The protected parent
counterfactual scored `6/60/494`; the submitted refined path scored `6/60/515`.
One day accepted a protected refinement.

Bot rank is excluded from promotion. The BTC gates establish only target-host
deadline, ACK, validity, reconciliation and a direct protected-parent score
gain.

Replay SHA256 values:

- `m-3927`: `FCA765AC5BCB98ABF932E3B3CB2CD850D91DCDE50EFED56AA0C3B8B38E3D11A2`;
- `m-3928`: `C77E0C7F5BD5F9E85C97E21A9DAADC125B83CC1418B5C628904D8D20B74750AD`.

## Decision

The candidate is accepted. It restores useful compute inside the canonical
5000-ms cap without replacing the previously certified 1600-ms incumbent by a
non-monotone longer search. Reopen only if target-host transport evidence shows
the 1100-ms refinement reserve unsafe, or a valid protected-parent regression is
reproduced. Do not tune the reserve or refiner by match, bot, seed, family, fuel,
role, or map size.

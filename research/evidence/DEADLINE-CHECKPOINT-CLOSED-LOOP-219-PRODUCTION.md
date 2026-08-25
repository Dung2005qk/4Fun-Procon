# DEADLINE-CHECKPOINT-CLOSED-LOOP-219 — production integration

## Frozen boundary

This gate validates the operation-equivalent production integration of the
already accepted experiment 219. It does not reopen or tune the consumed 219
holdout.

- production protected manifest:
  `research/holdouts/DEADLINE-CHECKPOINT-CLOSED-LOOP-219-PRODUCTION.csv`;
  SHA256 `F5222CFCF850DD1858F4F3973CE4951C268DC71CFC7A882F7C82FBFE32E1BBC3`;
- Linux historical-tournament binary:
  SHA256 `479EE6C9DFD25BB91D32E656ACFBCD4FB04032D2AA25A88D4ED337CEBFF2B3CC`;
- Windows BTC production binary after the terminal-certificate correction:
  SHA256 `96D538120BC9848DEA4EE1F904598260C06A36DC072F6DC97CB0F2FCBBED91FB`;
- `src/btc_main.cpp` SHA256
  `670B37AAD170C2A80315C45029EC2B1FE35FCB842AE3C44FCEEABB4B7BD3EE54`;
- Linux source-rank portability was repaired with a deterministic tie-break in
  `prune_columns`; the corrected fixture passes on Linux and Windows.

## Five-second controls

All `24/24` frozen controls matched `checkpoint_closed_loop_parent` exactly by
official score. There were zero public takeovers, checkpoint failures, public
failures, invalid plans or emergency days. The four log hashes are:

- easy: `51C346D071F383A3B8B6AB46D90C558E6A684E5C6E00B8EA9923F9BBD6EAA809`;
- medium: `BDDC73112303473B4DA55B17AAB3DF849133BC5A693986BE213FEF7EF051CCDD`;
- hard: `AA78BE2CFF94E91EBAB653433966ABDEB9F721CF3B373ACCA9730B52F90F0EA4`;
- very-hard: `1DEB28C7CA12393066055310FD2179EA484C4F75230A0080DEEBFB99BED00089`.

Runner stderr is empty, SHA256
`E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`.

## Independent production protected matrix

The separately frozen 12-case matrix used fresh seeds `5070000..5070005` and
`5071000..5071005`, hard and very-hard maps, all three fuel profiles, fixed and
native roles, and all six traffic families. It completed `12/12` at `3/9/0`
candidate/checkpoint W/T/L with net `+6` servings, all differences at tier 3:

- seed `5071003`, low fuel, native role, rare-brand: `6/60/522 -> 6/60/525`;
- seed `5071004`, default fuel, fixed role, threshold-corridor:
  `6/60/614 -> 6/60/616`;
- seed `5071005`, high fuel, native role, fuel-tight:
  `6/60/629 -> 6/60/630`.

There were three certified public takeovers, zero loss, deadline, checkpoint
failure, public failure, invalid plan or emergency day. Log SHA256 is
`DD3F0624DCD0D939BE928201DE58C4D6C011CF102226B7ECA4EC7380509733C3`;
runner stderr is empty with the standard empty-file hash above.

## BTC target-host lifecycle

- `m-4149`: four-day 5000-ms preflight on the pre-terminal-fix integration;
  `4/4` acknowledgements and exact replay, with no public authorization.
- `m-4151`: 15000-ms, 32x32, ten-day attribution on the pre-terminal-fix
  integration. Nonterminal continuation was accepted on days 5 and 6 for net
  `+3` servings, but the terminal day exposed an integration bug: the runtime
  incorrectly required a future-transition certificate after the final day.
  This run is bug attribution, not promotion evidence.
- The correction keeps transition/ledger dominance on nonterminal days and uses
  exact validity plus official lexicographic strict gain on the terminal day.
- `m-4153`: corrected 15000-ms, 32x32, ten-day lifecycle; `10/10`
  acknowledgements, zero failure/deadline and exact replay.
- `m-4154`: corrected 15000-ms, 24x24, low-fuel seven-day lifecycle; `7/7`
  acknowledgements, zero failure/deadline and exact replay.
- `m-4155`: corrected pure-5000-ms four-day lifecycle; `4/4`
  acknowledgements, exact replay and no checkpoint/public-continuation fields.

Replay SHA256 values are `2D0D9E33145B17CE83DC33F0D1E366A56C68EBA6219D3B0AC16EE8F5A9B8AFF9`
for the `m-4151` bug attribution, `C85C40FC4E8BBE6A8AB2FA257AF2F661A2ABB01E53FC08847D9953BF69F54B56`
for `m-4153`, `3CAB1BDF7F60ABC51F7347DE7144E6A7CED047D325FC51BE0EDF538A83E7F7C6`
for `m-4154`, and `DEFB532BB026419E3FFF34738FA26F220A37E5B9B047207DCE7338F39E318047`
for `m-4155`. BTC bot ranks are not promotion evidence.

## Verdict

The production integration passes the frozen five-second equivalence,
independent protected-score, portability and BTC target-host gates. Experiment
219 is accepted for production. Its consumed development and holdout sets remain
closed and may not be used to tune a successor.

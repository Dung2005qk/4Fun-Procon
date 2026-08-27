# ATTR-CONTINGENCY-HARNESS-PARITY-228

Date: 2026-08-26.

## Gap

Experiment 226 attempted to model production's post-ACK contingency cache in
`replay-counterfactual`. Its lifecycle order was not faithful: after
`record_submitted` and `precompute_next_day_contingencies`, it called
`record_applied_transition`. The latter is the external/fallback transition
path and intentionally clears response artifacts, so the generated cache never
survived to the next replay day. The recorded score distribution therefore
cannot establish the claimed causal benefit of live contingencies.

## Candidate and invariants

For an accepted counterfactual submission, keep `record_submitted` as the sole
transition recorder and retain the generated post-ACK artifacts. This matches
HTTP `MatchSession::acknowledge_submitted`. `record_applied_transition` remains
unchanged and active for external/fallback transitions.

There is no HTTP, sandbox, engine, planner, comparator, simulator, validator,
deadline or production-path change. `--post-ack-ms 0` remains the control path.
The consumed m-4195 replay is attribution-only and cannot promote a successor.

Frozen manifest:
`research/holdouts/ATTR-CONTINGENCY-HARNESS-PARITY-228.csv`, SHA256
`A914D1C9674EA839667EBAAE4826E255C43393D1C41AB0B963726959FAE3C425`.

Frozen replay SHA256:
`3204E239020EB17FDC0995A4ED70208DC388256E604AB284386A33737FCF6B9D`.

Quiet-VM Linux binaries:

- buggy 226 instrument:
  `78D4C6433B4373BB1A039BDC48AE12F8BF9C2F11BE95ACF3B29BE8F7FA5E372C`;
- cache-faithful candidate:
  `DA6A7934CAF3E884A54349738E930AC2D98B98B3A6A124F3CDBC85F283EDAC32`.

## Registered classification

The quiet VM executes round-robin no-ACK control (12), buggy 226 (12), and
cache-faithful candidate (24), fixed role mask 4 and 5000-ms canonical solve.
The candidate must emit valid next-day `cached_contingency` previews whenever
the preceding day generated an artifact. For clustered day-3 states:

- valid full-daily cache adopted and no day-4 dip closes the 222 axis;
- valid cache present but a day-4 dip opens a cache-adoption gap;
- no cache followed by a dip opens a generation/retry gap.

## Result

The quiet-VM run completed all 48 preregistered cases with empty stderr and
unchanged frozen hashes. The no-ACK control was `12/12` at `8/40/130`; the
buggy-226 lane was also `12/12` at `8/40/130`. Both lanes emitted zero
next-day cache previews, proving that the old 226 lifecycle never exposed its
generated artifacts to the following replay day.

The cache-faithful lane emitted 596 next-day previews over 24 runs and therefore
restored the intended structural parity. Its score distribution was
`8/40/130` in 23 runs and `8/37/117` in one run. All 24 runs reached the same
clustered day-3 terminal state. In the losing run, day 4 had eight cached
previews, but six were invalid under the authoritative road state and the two
valid plans reached only `1/3` and `7/11`; no valid cached plan reached the
full eight daily brands. The main solve then reached only `5/13` on day 4.
Across all 24 cache-faithful runs, no day-4 preview was both valid and
full-daily.

This satisfies the preregistered generation/retry classification, not the
adoption classification: the cache survived and was visible, but it did not
contain a full-daily candidate that the solver could adopt in the observed
tail. Experiment 226's claimed causal tail reduction is therefore superseded;
its post-ACK artifacts were cleared and its score distribution was sampling
variance rather than evidence of live-cache protection.

Raw evidence archive SHA256:
`069DCE99186433C2C05A999311F3840CBDF9081CD9C60EF9539FDC1D2DB512B6`.

Verdict: accepted harness-correctness fix; production contingency generation
gap remains open. Any score successor must use fresh development and holdout
seeds and may not use m-4195 for promotion or parameter tuning.

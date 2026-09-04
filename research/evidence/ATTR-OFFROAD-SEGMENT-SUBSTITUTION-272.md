# ATTR-OFFROAD-SEGMENT-SUBSTITUTION-272

Status: closed, rejected attribution; no production source candidate and no holdout.

## Frozen inputs

- Probe source SHA256: `8158F5517102DACF5F45608B6659118218AAFCFF974A2F9C8D17DF304D3EE7EC`
- Probe binary SHA256: `C4AAD641DDB102C39FB002862F212CA91277300AD9159BA81A0EF975AC72093B`
- Atomic runner SHA256: `224EEE82BDD5FD9357A7C42D49BEF49D41F756E4A72291F30AF980BD8D2866BF`
- Complete log SHA256: `1A1A476774600D286944A9F68F8A2669A9DFEFA71839170AB1C9DA25D53D7520`
- Roots fixed before the probe change: `m-6134` days 1/4/8 and `m-6213` days 1/5/8.

## Complete evidence

All six cases completed atomically. The probe considered 16,672 short incumbent spans and found 260 eligible off-road spans containing wait slack. It evaluated 352 path pairs and generated 136 distinct exact-valid, independently validator-valid plans. All 136 preserved exact aggregate road footprint; 135 also preserved same-agent terminal state with no-lower Patrol fuel. There were zero incumbent mutations, invalid plans or validator mismatches.

None of the 135 state-dominating candidates strictly improved an official score component. The registered breadth gate therefore failed 0/6.

## Verdict

The broader short off-road segment substitution is safe and reachable but adds no score supply beyond the accepted wait-detour/refinement stack on these two material counterexamples. Experiment 272 closes without a SCORE successor; consumed replay evidence has no promotion authority.

This experiment removed, disabled, deferred or reduced no designed functionality. It deleted nothing and only observed candidate supply; therefore no replacement-equivalence claim is needed.

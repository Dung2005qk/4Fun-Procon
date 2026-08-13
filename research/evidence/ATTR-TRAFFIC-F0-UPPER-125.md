# ATTR-TRAFFIC-F0-UPPER-125

Parent: `5dccb0f`

On consumed threshold-loop/low seed2300200 day 1, the merged master supplied 32
candidates plus the incumbent. The frozen F0 intake retained 12 quality/protected
ids and four diversity ids. The exact oracle plan was not a quality id and was
also absent from the current distance-only 16-slot result.

Keeping all 12 quality ids fixed, the probe ranked only the same four diversity
slots by the unchanged candidate-specific FastViability upper, then existing
plan distance and deterministic quality ties. The exact oracle candidate with
upper `6/24/28` was retained. F0 stayed at 16; quality evictions were zero; two
diversity ids changed.

Verdict: accepted attribution. The evaluator signal already exists and can close
the road-map F0 mismatch without adding slots or reducing quality intake. A score
candidate must use an all-or-nothing road-upper computation: if every required
upper does not complete before the existing F0 deadline, discard partial values
and execute the exact parent distance-only selector.

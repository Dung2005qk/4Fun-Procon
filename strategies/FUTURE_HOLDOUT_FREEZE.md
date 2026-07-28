# Future Holdout Freeze

The feedback-cycle structure, stage budgets, candidate limits, and comparison
protocol were frozen before opening `future-holdout`.

- budget: `1200 ms` per day;
- maps: seeds `200000..200299`;
- focus: `udon-shield-feedback` versus `udon-shield`;
- traffic: endogenous own footprint plus common opponent footprints;
- validity: exact simulator and independent validator must agree;
- score: official lexicographic tuple;
- no parameter or source changes are permitted after this marker and before
  recording the held-out result.

SHA-256 source fingerprints:

- `src/decision.cpp`: `997E562325EDF94B253A202AB0C7844EB32F55B4203A621E26262BEF7B859A00`
- `src/planner.cpp`: `B2AEE4F7FD6EC5FA79BA2B0293AB0E962CC1E9E96EAB7906B9C7ABEA365A9EC7`
- `include/udon/decision.hpp`: `543C5D00FB12FE303943824B40322020BAA2C6D2C922123CFCB7692C981EA1C1`
- `include/udon/planner.hpp`: `8A2CDD350C8E25909615A43D0D66600227FC5C7FBC3F218E483A2BCEFB5F6F6F`
- `strategies/strategy_suite.cpp`: `B30EE1499C3F9E1F7EEBC2409E8DBFD8978A4909D467C8E8E971A41D347A8680`

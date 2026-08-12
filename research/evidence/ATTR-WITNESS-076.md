# ATTR-WITNESS-076 — future portfolio/master attribution

Date: 2026-08-12
Parent: frozen champion `7ef3694` plus the uncommitted 072+073 prerequisites.
Manifest SHA256:
`6329ADA27BCEF1EF6210D984C866309120AD2E9057D09D50F3A02BA300304C24`.

The probe evaluated each exact oracle day-1 plan as a canonical
`MasterCandidate`, ran unchanged F0/W1 repair, and replayed the complete oracle
continuation through both exact engines. At every future state it checked exact
per-agent route membership, exact team-plan validity/stable ID, bounded master
retention at one and 32 candidates, and semantic score/final-agent equivalence.

All oracle plans were dual-valid. At the eight days where the oracle continuation
was not retained, the active patrol route was absent while both isolated WAIT
routes were present (`portfolio_mask=011`). Neither exact stable ID nor an
equivalent final team state survived master-32. The missing active routes were:

- seed 1300200: day 2;
- seed 1310300: days 2, 3 and 4;
- seed 1310400: days 2, 3, 4 and 5.

Seven routes reappeared when only `maximumColumnsPerAgent` was widened. Their
first observed caps were respectively `12, 12, 8, 8, 16, 4, 12`. The remaining
seed-1310400 day-5 route stayed absent at cap 32 and four paths per target; it is
a separate deeper generation gap, not evidence against the seven cap-pruned
routes.

The source audit explains the boundary. W1 sets `enableHarvestExtensions`, but
`RouteColumnGenerator` enters its harvest-extension block only for caps 12..16;
triple-route generation requires cap 12 and quadruple generation cap 16. W1's
current cap 3 for detailed days and 2 for terminal days therefore makes those
designed sources unreachable.

Verdict: accepted attribution. The primary residual gap is future route
capability/pruning before master ranking. Holdout remained sealed and no
production source changed during the experiment.

# 366-P2 protected complete: frozen active gate not passed

2026-09-10. All108 frozen fixtures executed once, in original A order.216 results,
216 independently checked certificates,108 fixture markers,1368 valid ACKs and
1152 reconciled transitions;1620 evidence files plus completion. VM frozen
summarizer --check and exact local rederivation both PASS. No solver rerun,
second protected pass, selective stopping, safety failure or deadline rollback.

## Exact result, unchanged P2 interpretation

- Overall108:31/73/4, component delta0/-2/+108.
- Active72 at10/15s:29/41/2, component delta0/-2/+108. Gross serving gain141,
  gross serving loss33. Every active loss remains counted.
- Inactive36 at5s:2/32/2, component delta0/0/0; gross+3/-3, worst loss2.
  Four score/trajectory mismatches. These are diagnostic only, both wins and
  losses, NOT a treatment-score veto or gain. No resource frame/control entry.
- Active failures: active_benefit, active_no_upper_tier_loss,
  active_serving_tail, active_strata. Causal breadth/sign and all safety checks
  pass; one-sided temporal-causal winner sign p=2.314336597919464e-07.
- Work:384 entered frames,84 takeovers,2128 queries,839974565 settled states,
  23332 routes/evaluations/validations,2566 certified proposals;deadline0,failure0.
- Complete raw rows, every public stratum, first tiers, plans, gains/losses,
  original P1 diagnostic gate and all certificates are preserved in the exact
  summary and local audit, not reduced to a weighted score.

Two active losses are both10s:
1. seed202609093860460:32x32,10players,native,low fuel,high-stock,10days;
   parent6/60/538 versus candidate6/60/505 (-33servings). Roles already differ
   before day1, first resource takeover day3. This is not established treatment
   harm, but P2 prospectively counts ALL active losses.
2. seed202609093860641:32x32,8players,fixed-all-Patrol,high fuel,roadless
   overnight,10days;parent6/58/363 versus candidate6/56/365 (-2daily,+2servings).
   First submitted divergence and first certified takeover both day1. That
   day-level ordering is a causal-screen label, not proof of root cause; audit
   the finer main/checkpoint/public boundaries before asserting treatment harm.

Verdict: qualification NOT PASSED under the frozen user-amended P2 gate. Do not
silently waive active loss rules, relabel original holdout pass, retry lucky
seeds or claim production readiness. No source candidate was integrated, so
there is no product patch to revert. Canonical258 remains unchanged. No designed
functionality was removed, disabled, reduced or deleted. No new two-patrol
optimizer or product commit. Read-only boundary attribution may explain the
completed losses, but cannot retune or promote on these consumed fixtures.

## Preserved provenance

- Summary B401C7698036FFFF557B0520055D016AB328D3C7C5F83B57BE4CBD8233D97CB4.
- Completion B3939DC611F695D86168BD1D6EA274592DF851A4BE9537DA66FB4E98B08A6C85.
- Archive 7FBA3D84075973544C156397F141188521291701C78ADFB375543518FB1E8F96.
- Local audit BA777D77478BAE8D12664A56D418682DB701CD9BC146B689BE9FCB1D21D3DD89.
- Execution C968D42AE14F881D49DC9A8DC631E42960EF34BE67CF9433565E5E563803C92D.
- Candidate D3684DC3F4ADB5E6E74F5E75412CCB76E681D21F760B6BC0FA61EDC25A029678.
- Protected setup02E4C55AD2C8C506239E0B947BB5E2DA955D329C8BADC2B5979135F93A52DEB7.
- Copy artifacts/research/366/completed-protected-causal. Extraction checks every
  archive member for regular-file type, path containment, no duplicates and no
  overwrite. Reverification uses frozen audit/classifier with host/path adapters
  only, plus unchanged360/361/364/365 and prior366 evidence verification.

Sole monitor monitor-udon-366-protected-causal deleted through the app after
completion. No waiting monitor, live research runner, rebuild or BTC match was
started by this closure. Existing VM and evidence are retained.

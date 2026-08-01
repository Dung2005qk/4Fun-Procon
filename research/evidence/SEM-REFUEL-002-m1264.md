# SEM-REFUEL-002 BTC gate

- Match: `m-1264`
- Configuration: hard, 3 bots, 10 days, 32x32, 100 steps/day, 5000 ms,
  8 agents, 12 spots, 6 brands, low fuel 1x.
- Assignment: mask 128, seven patrols and one tanker.
- Result: rank 1, official score `6/60/261`; next bot `6/60/202`.
- Lifecycle: 10 decisions, 10 action POSTs, 10 HTTP-2xx valid ACKs.
- Safety: zero deadline skip, server WAIT, emergency and exact overrun.
- Timing on BTC target host: max solver 2845 ms; replayed role selection 3956 ms.
- Independent replay: all ten plans valid and validator-agreed; 9/9 transitions
  reconciled; final score `6/60/261`.
- Replay SHA256:
  `2811EC26C075829D63F61B7949394470B568CFDA72F6B30A51838CF9ACD4540A`.

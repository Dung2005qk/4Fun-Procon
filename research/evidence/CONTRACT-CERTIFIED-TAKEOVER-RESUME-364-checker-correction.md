# 364 verifier-only correction; no accepted action rerun

The first session202609093620092/control0 completed its synthetic5day lifecycle,
one terminal POST/ACK,zero assignment repost and no child stderr; exit0. The
original checker then incorrectly rejected its intentionally expired day4
`deadline-emergency` decision. Its budget was0,valid plan,true emergency,34ms;
it was followed by exactly the preregistered deadline skip and server WAIT.
Day5 was nonemergency,dual-valid,main1953ms and ACK valid. No product failure
is established. Original runner/hash/stdout/stderr and this session are preserved.

Original scriptF7AA82A6D67E79C882B609F68A0E4A62C00EAE8250458937235875CFB31B953C
and stage27423C681FB75FEEB81044DD49E09646CE2EFD02F64B174236E45D1D4111B7D9
remain frozen. Correction changes only interpretation of the registered negative
stimulus: its decision MUST be on the precise expired day,valid,deadline-emergency,
zero available budget,followed by one no-POST/server-WAIT transition. The terminal
decision must remain nonemergency and valid within5000ms. Any extra emergency,
wrong day/nonzero expired budget,duplicate POST,invalidity or mismatch still fails.

Use a separate frozen v2 wrapper/verifier with positive/adversarial checker tests.
Revalidate the existing first session and run read-only replay-check, then create
its completion marker without rerunning solver/accepted action. Run each of the
three missing sessions once through the original frozen runner's same live path;
the wrapper completes the corrected strict evidence verification after the old
checker asserts. No source-policy or binary change, metric/score interpretation,
new fixture, relaxed lifecycle duty or lost functionality. No results are deleted.

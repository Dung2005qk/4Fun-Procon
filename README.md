# HEXUDON C++ Core

Đây là lõi C++20 cho agent HEXUDON, tách riêng luật trận đấu, tối ưu kế hoạch và lớp tích hợp mạng. Mục tiêu của mã nguồn là giữ nguyên semantics của đề bài: mọi action plan được gửi đi phải hợp lệ theo simulator độc lập, thay vì chỉ hợp lệ theo heuristic của planner.

## Phạm vi đã hiện thực

- Parser JSON nghiêm ngặt cho `config`, state theo ngày, action plan và match ledger.
- Hình học lưới even-r, phản xạ hướng, road movement theo thời lượng, và action `0..5`/wait âm.
- Simulator theo bước thời gian với thứ tự fuel, pickup/refuel, stock, servings và road-traffic đúng theo state chuyển tiếp.
- Validator độc lập trả về lỗi có cấu trúc và footprint đường theo từng tick.
- Router Pareto theo `(time, fuel)`; sinh route column; master set-packing native có lexicographic branch-and-bound, CAP/PREFIX cuts và luôn xác nhận tổ hợp cuối bằng exact simulator.
- High-fuel production dùng exact orienteering trên toàn chuỗi spot khi search window đủ lớn. Ở ngày cuối, patrol có fuel hiện tại dưới ngưỡng exact high-fuel dùng frontier `cell × spot-mask × (steps,fuel)` giới hạn 1.250.000 settled states và 32 route/agent; mỗi route vẫn exact-feasible và được simulator/validator xác nhận, nhưng frontier bị cắt không được báo là proof hoàn tất. Pool raw-spot legacy luôn được giữ nguyên; nếu lifetime còn thiếu brand, một pool supplemental xếp hạng lexicographic theo missing-brand coverage được phối hợp thành bundle riêng và chỉ thêm nghiệm, không thay thế bundle legacy. Các ngày trước vẫn giữ planner đa ngày để tránh đổi servings hôm nay lấy fuel debt ngày mai.
- ALNS destroy-repair có thể sinh route Pareto mới ngoài portfolio ban đầu cho đủ tám operator; mọi mutation được exact-evaluate và rollback nếu phá reservation đã chứng minh.
- Phân vai pre-match, escort/refuel đồng bộ, repair deadline/fuel, traffic belief, scenario counterfactual và chọn candidate chỉ khi mọi scenario được chứng nhận bởi witness hợp lệ.
- Adapter HTTP BTC chặn ngân sách bằng `response-ms` tính từ lúc nhận state trên máy cục bộ, không kéo dài search theo `endsAt` của đồng hồ server chưa đồng bộ; scheduler vẫn giữ riêng network floor trước khi gửi.
- Proof cấp P chạy ngoài critical path sau ACK, tìm kiếm branch-and-bound trên toàn horizon còn lại dưới frozen persistent scenario và ghi rõ scope route-portfolio, UB, best score, số nhánh cắt và trạng thái complete.
- Ledger nhiều ngày cho distinct-brand và tổng servings; ngày từ `2` trở đi fail-closed nếu thiếu ledger.

## Bảo đảm correctness

Các tối ưu như cache khoảng cách, cắt Pareto, column shortlist, beam pre-match và cut-guided ordering chỉ dùng để giảm không gian tìm kiếm. Chúng **không** là nguồn chân lý cuối: candidate được chọn luôn chạy qua `Simulator` và `Validator`; candidate uncertainty chỉ có upper bound không được phép gửi.

Master hiện là native set-packing thay vì OR-Tools CP-SAT. Tối ưu từ điển được giữ đúng bằng comparator lexicographic và sound upper bound; chỉ khi `searchComplete=true` mới có proof tối ưu trên portfolio đã đóng băng. Deadline hữu hạn vẫn trả exact-valid incumbent và không được gắn nhãn global optimum.

Beam và DFS của master mặc định xếp nhánh theo stock-capped marginal credits sau hai tier brand, thay vì cộng raw claims bị trùng stock. Đây chỉ là search ordering, không phải pruning; exact simulator/validator và tập nghiệm khi search hoàn tất không đổi. Replay audit ghi `masterDiagnostics.stockCappedSearchOrder=true`, còn test A/B giữ được raw-claim mode để chứng minh phản ví dụ bounded-search.

## Build

Yêu cầu CMake, Ninja và compiler C++20:

```powershell
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --parallel
ctest --test-dir build-release --output-on-failure
```

## Runtime integration

- The production host calls `UdonShieldEngine::solve_day_until(...)` with the server deadline; its fail-closed path emits an exact wait-only plan when the certification budget is exhausted.
- `MatchSession` keeps the authoritative `state -> acknowledgement -> precompute/proof` lifecycle in one process: same-day resend retains prior-day traffic memory, acknowledgement preserves only certified contingencies, and post-send work never consumes the next authoritative decision budget.
- `ResponseLedger` retains same-process certified next-day contingency bundles, revalidates them against the next authoritative state, and never mixes a partial cached bundle with newly generated routes.
- `serialize_decision_replay(...)` writes a `udon-shield-replay-v1` audit artifact containing the frozen traffic manifest, risk policy, candidate dispositions, exact trace, claims, and final footprint.
- `ParetoRouter` memoizes exact route-query results using the complete map status and Pareto constraint set as its key; cache hits never bypass validation or alter the returned path set.
- The native set-packing master prunes with sound lexicographic bounds and preserves exact simulator/validator acceptance. Strong proof records are explicitly scoped; an incomplete deadline-bounded run is never reported as optimal.
- Master upper bounds are computed per atomic contingency-bundle mode before taking the lexicographic maximum, so mutually incompatible cached modes cannot inflate the proof gap or alter the exact feasible set.
- Each bundle mode retains a capped antichain of suffix brand masks, preventing one agent's mutually exclusive routes from being combined inside the proof bound. Frontier overflow falls back to the prior sound union bound, while a separate loose guidance bound preserves ALNS exploration outside the frozen portfolio.

Official BTC HTTP/authentication/action-result contract is now implemented by `udonshield_btc`. Live readiness still requires authoritative replay coverage and target-host p99 calibration; successful compilation or synthetic fixtures alone are not treated as server conformance.

## CLI contract adapter

```powershell
# Pre-match role shortlist
./build-release/udonshield_cli roles config.json [beam]

# Day 1 (ledger chưa cần)
./build-release/udonshield_cli plan config.json state-day-1.json available-ms

# Day >= 2 (ledger là bắt buộc)
./build-release/udonshield_cli plan config.json state.json ledger.json available-ms

# Xác minh một plan độc lập
./build-release/udonshield_cli validate config.json state.json plan.json

# Cập nhật ledger sau khi plan được chấp nhận
./build-release/udonshield_cli advance-ledger config.json ledger.json state.json plan.json

# Stateful NDJSON adapter: state, ack, precompute, proof, roles
./build-release/udonshield_cli session config.json
```

Sau một event `ack`, host có thể gửi `{"type":"proof","availableMs":2000}`. Response trả từng proof record với `scope`, `complete`, `infeasible`, `bestScore`, `upperBound`, `combinationsVisited` và `branchesPruned`.

`plan` là adapter JSON một lần chạy. Host production phải giữ cùng một `MatchSession` trong suốt match để traffic belief, response ledger, lịch sử resend, contingency precompute và proof hậu-ACK còn nguyên giữa các ngày.

## BTC official transport

HTTP luyện tập trên Windows dùng WinHTTP và chỉ đọc bearer token từ biến môi trường `HEXUDON_TOKEN`; token không được ghi vào replay, command arguments hoặc source:

```powershell
$env:HEXUDON_TOKEN = '<token nhập cục bộ>'
./build-release/udonshield_btc.exe http --match m-0000 --response-ms 5000 --replay artifacts/btc/m-0000.jsonl
Remove-Item Env:HEXUDON_TOKEN
```

Sandbox nộp bài là C++ JSON Lines thuần, không cần thư viện mạng:

```powershell
./build-release/udonshield_btc.exe sandbox --response-ms 5000 --replay artifacts/btc/sandbox.jsonl
```

Adapter BTC giữ solver nội bộ 1-based nhưng dịch ngày wire `0..D-1` tại biên, điền road bị lược bỏ là `Smooth`, coi fuel đối thủ bị ẩn là năng lực tối đa để không đánh giá thấp đối thủ, và chỉ tổng kết ledger sau ACK hợp lệ hoặc state ngày kế tiếp trong sandbox. Nếu server từ chối candidate đã chứng nhận, runtime hủy pending decision và gửi một kế hoạch WAIT exact-fill; một WAIT bị từ chối tiếp sẽ fail-closed thay vì tiếp tục với trạng thái ledger sai.

HTTP production giữ `1600 ms` trong solver cho network và không gửi candidate nếu cửa sổ authoritative còn dưới `800 ms`. Mỗi lần chờ ACK `/actions` bị chặn trong một lát cắt mặc định `750 ms`; lỗi timeout/resend của WinHTTP chỉ được phục hồi bằng cách gửi lại đúng chuỗi JSON đã đóng băng, sau ít nhất một poll interval và khi hard deadline `5000 ms` vẫn còn chỗ. Ledger chỉ tiến một lần sau ACK được chấp nhận. ACK có trường `day` phải đúng `wireDay + 1`; ACK lệch ngày làm runtime fail-closed thay vì cộng score hoặc tiếp tục từ state sai. `replay-check` cũng chỉ cộng plan sau ACK hợp lệ đúng ngày, không còn tính plan bị từ chối hoặc gửi trễ.

Role assignment runs before `/start`, so HTTP gives it the full configured response budget. The network reserve remains exclusive to authoritative daily `/actions` decisions and is never subtracted from pre-match role rollout.

Replay JSONL chứa setup, assignment, state, actions, action result và standings kèm timestamp/status nhưng không chứa header xác thực. HTTP là transport ưu tiên cho vòng nghiên cứu hiện tại theo hướng dẫn BTC; WebSocket chưa nằm trên critical path vì không thay đổi logic solver hoặc schema hành động.

## Bản đồ mã nguồn

- `src/simulator.cpp`: nguồn chân lý cho transition và scoring.
- `src/validator.cpp`: kiểm tra plan độc lập, lỗi protocol và traffic footprint.
- `src/graph.cpp`: hex routing, cube-distance admissible pruning và Pareto labels.
- `src/planner.cpp`: columns, refuel/escort, lexicographic BnB master, proof-guided ALNS route synthesis, CAP/PREFIX diagnostics và pre-match roles.
- `src/decision.cpp`: deadline profiles, scenarios, certification, multi-day repair/proof và response accounting.
- `src/protocol.cpp`: JSON wire contract và ledger serialization.
- `src/btc_protocol.cpp`: adapter schema BTC chính thức, day-index translation và WAIT recovery.
- `src/btc_main.cpp`: host HTTP WinHTTP, sandbox JSONL, ACK lifecycle và replay capture.
- `tests/test_main.cpp`: regression cho luật cốt lõi, ledger, traffic, docking, certification và witness expiry.
- `bench/benchmark_main.cpp`: benchmark synthetic 32x32/8-agent.
- `bench/strategy_suite.cpp`: replay 4 ngày cho 4 chiến lược trên 1 fixture random và 5 fixture adversarial.

## Số đo hiện tại

Release benchmark synthetic `32x32`, `8` agents, `24` spots:

```text
master-oracle: exact_matches=1000/1000
master-oracle: exhaustive=64000 bounded=49390 reduction=22.8281%
decision_ms=4428 emergency=0 score_today=11,11,31 certified_lb=11,11,31
decision_phases_ms=incumbent:20 fast:0 search:3120 prepare:710 certify:578
```

CTests hiện tại pass `3/3`; unit/regression executable báo `all tests passed`.

Sau cube pruning và proof-guided supplemental repair, confirm 40 map cho full shield
`14/14/12`; holdout 80 map common-role là `22/38/20`. Holdout native dao động
`22/36/22` dưới deadline wall-clock. Invalid rate và tier-1 drop đều bằng `0`;
p95 full solver nằm trong `545–579 ms`. Sign test chưa có ý nghĩa thống kê, nên kết luận đúng là
integration đã loại bỏ thất bại cũ và đang nghiêng tích cực, chưa phải bằng
chứng vượt trội phổ quát.

Các số đo trên là chỉ dấu deadline khả quan, không phải bằng chứng production trên server chính thức. Cần official HTTP/auth/ACK contract, replay state thật, calibration opponent/traffic và target-host p99 concurrent budget trước khi chốt live readiness.

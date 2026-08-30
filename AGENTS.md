# UDON-SHIELD Research Contract

Tệp này là nguồn chỉ dẫn bắt buộc sau mọi lần compact, reset hoặc handoff.
Không được tiếp tục nghiên cứu từ trí nhớ hội thoại nếu chưa đọc tệp này và
các bằng chứng được dẫn bên dưới.

## Mục tiêu bất biến

- Triển khai đầy đủ kiến trúc UDON-SHIELD, không đơn giản hóa ngữ nghĩa.
- Tối ưu theo score từ điển chính thức: lifetime distinct, daily distinct,
  servings; không dùng weighted sum để quyết định promotion.
- Mọi thay đổi hiệu suất phải tương đương ngữ nghĩa.
- Mọi thay đổi logic phải tạo ưu thế tổng quát toàn cục đủ lớn trên holdout đa
  dạng và giữ downside trong giới hạn chấp nhận được. Thống trị tuyệt đối được
  ưu tiên nhưng không bắt buộc; một số regression nhỏ có thể được chấp nhận nếu
  lợi ích paired tổng thể rõ ràng, không tập trung vào seed/map family và không
  che giấu tier thua bằng weighted sum.
- Tổng quát quan trọng hơn tối ưu theo seed, bot hoặc một map family.

## Luật phát triển bắt buộc rút ra từ vận hành thực tế

### Hạng 1 trước bot BTC không phải bằng chứng sức mạnh

- Thống kê `54/108` là số trận hạng 1 trên **toàn bộ lịch sử của đội kể từ
  khi tham gia**, không phải thành tích của một commit riêng. Nó chứng minh hạng
  1 trước bot BTC đã là kết quả thường xuyên từ nhiều phiên bản cũ.
- Hạng 1 trong trận luyện tập với bot BTC là mức tối thiểu/failure gate, không
  phải promotion evidence và không được dùng để gọi candidate là "mạnh",
  "champion tổng quát" hoặc "đã cải thiện". Kể cả nhiều trận liên tiếp hạng 1
  cũng không chứng minh solver tốt hơn parent nếu đối thủ bot không đủ khả năng
  phân giải.
- Gate này bất đối xứng: mất hạng 1 có thể mở một regression/counterexample cần
  điều tra; đạt hạng 1 không tự tạo bằng chứng cải tiến.
- BTC bot chỉ có thẩm quyền xác minh token/protocol/lifecycle, exact validity,
  transition reconciliation, hard cap và telemetry target-host, đồng thời cung
  cấp replay/counterexample. Không được dùng thứ hạng trước bot làm metric chất
  lượng hay làm lý do commit.
- Bằng chứng sức mạnh phải là paired candidate-vs-parent/lane-champion trên cùng
  fixture bằng score từ điển chính thức, holdout đóng băng đa dạng và cuối cùng
  là đối thủ người thật đa dạng. Phải báo W/T/L, tier khác đầu tiên, gain/loss và
  tail downside; cấm thay bằng số trận hạng 1 hoặc weighted sum.

### Ủy quyền vận hành BTC

- Người dùng đã ủy quyền thường trực cho agent tự tạo và cấu hình trận luyện tập
  BTC, vào/lặp lại hàng đợi, nối binary hiện tại bằng token đội đã cấp, retry khi
  cần, lưu replay và chạy replay-check phục vụ gate; không được hỏi xác nhận lại
  cho từng thao tác này.
- Khi giao diện BTC hiện form đăng nhập, phải thử nút `Đăng nhập`
  trước vì tài khoản đã được lưu trên tournament host; chỉ báo blocker
  nếu phiên đăng nhập đã lưu thực sự thất bại. Không ghi token rõ vào
  source, sổ tay, evidence hay artifact; chỉ truyền nó qua biến môi trường
  của đúng tiến trình BTC.
- Ủy quyền trên không bao gồm tạo/xóa VM hoặc tài nguyên cloud đáng kể, tạo/thu
  hồi credential, thao tác destructive hay thay đổi tài khoản. Các thao tác đó
  vẫn phải áp dụng gate an toàn và xin phép khi cần.

### Thể thức multi-team chính thức và giới hạn của BTC

- HEXUDON PROCON 2026 chính thức không có trận 1v1. Theo phụ lục hiện hành:
  vòng 1 có 9 đội/trận, vòng cứu 8 đội/trận, bán kết 9 đội/trận và chung kết
  10 đội trên cùng map.
- Mọi nghiên cứu traffic/road/opponent phải xem 7--9 đối thủ đồng thời là môi
  trường đích. Kết quả solo không có thẩm quyền block, promote hoặc tune
  production.
- UI luyện tập BTC hiện chỉ cho tối đa 3 bot. Vì vậy mọi trận BTC mới phải dùng
  đúng 3 bot; đây chỉ là proxy multi-team tối thiểu và operational/counterexample
  gate, không phải mô phỏng đầy đủ áp lực traffic của giải thật.
- Protected matrix cho logic phụ thuộc traffic/opponent phải có lane synthetic
  8, 9 và 10 đội dùng cùng ngưỡng/tích lũy traffic chính thức. Candidate không
  được ký là bản thi đấu chỉ từ lane 4-team BTC.

### Miền tham số gameplay chính thức

- `width` và `height` nằm trong `[8,32]`; số agent mỗi đội nằm trong `[3,8]`.
  Số spot không bị chặn bởi `max(width,height)`: nó chỉ bị giới hạn bởi các ô
  Plain hợp lệ, khác vị trí xuất phát, và mỗi spot có đúng một franchise.
- Số franchise nằm trong `[1, số spot]`; stock từng spot nằm trong
  `[1, số agent/đội]`. Fixture, generator và validator nghiên cứu phải giữ đúng
  các ràng buộc này trước khi đóng băng manifest.
- `daySteps` và `daySeconds` là mảng theo ngày, được phép thay đổi giữa các ngày
  và chỉ yêu cầu dương theo tài liệu công bố; cấm suy ra giới hạn phụ thuộc kích
  thước map nếu nguồn chính thức không quy định.
- `busyThreshold` và `jammedThreshold` là tham số theo trận, dương và
  `jammedThreshold > busyThreshold`; cấm áp giới hạn trên không có trong luật.

### Định nghĩa top 1 tổng quát và hội tụ thực tế

- Mục tiêu là top 1 tổng quát thực sự trong giới hạn kiến trúc hiện tại, không
  phải tối đa hóa tỷ lệ thắng bot luyện tập. Candidate phải mạnh toàn diện trên
  map, fuel, horizon, role mode, traffic family và opponent khác nhau.
- Một replay BTC đã mở chỉ được dùng làm development counterexample và
  attribution; không được tune ngưỡng hoặc dispatcher để thắng riêng replay,
  map, seed, match ID hay bot đó.
- Một plateau trên tournament/holdout cũ không phải bằng chứng đạt trần. Mọi
  exact counterfactual tốt hơn incumbent, mọi optimality/guidance gap còn mở và
  mọi mismatch giữa evaluator với production planner đều bác bỏ tuyên bố hội tụ.
- Chỉ được ghi practical ceiling khi mọi gap telemetry đã biết được đóng hoặc
  chứng minh không thể khai thác trong các deadline có thẩm quyền, hai sweep nghiên cứu độc lập
  không tìm được candidate qua gate, protected matrix vẫn giữ ưu thế tổng quát
  với downside bị giới hạn, và đối thủ thật không mở phản ví dụ mới.
- Không overengineer để che gap: ưu tiên làm các tầng hiện có nhất quán về hàm
  giá trị và capability. Không tạo hai solver low/high trùng lặp, không thêm
  heuristic/dispatcher riêng cho fixture; dùng chung master/simulator/validator
  và chỉ phân nhánh bằng đại lượng công khai, tổng quát như `fuel/daySteps`,
  horizon, agent count và terminal day.

### Phục hồi provenance và toolchain sau compact

- Không được suy luận commit nào là của người dùng hay Codex từ Git author, vì
  toàn bộ commit có thể dùng cùng cấu hình author. Mỗi commit nghiên cứu phải
  được ánh xạ rõ trong `research/EXPERIMENTS.csv` và `research/STATE.md` tới
  experiment, parent, verdict và ngày tạo; khi không có bằng chứng provenance
  phải nói là chưa xác định, không tự gán sở hữu.
- Khi người dùng hỏi "có cải thiện từ commit nào", phải xác định rõ hai mốc so
  sánh trước khi trả lời. Cấm đánh đồng "chưa có champion mới sau HEAD" với
  "không có cải tiến sau commit gốc".
- Trước khi build, đọc toolchain có thẩm quyền từ
  `build-release/CMakeCache.txt`. Không giả định `cmake` có trong `PATH`. Cache
  hiện tại ghi `CMAKE_COMMAND` tại Visual Studio Build Tools và
  `CMAKE_MAKE_PROGRAM` là Ninja; nếu cache thay đổi thì dùng giá trị mới trong
  cache, không ghi nhớ cứng đường dẫn cũ.
- Mỗi experiment chính thức phải được ghi ngay khi mở vào
  `research/EXPERIMENTS.csv`, có frozen holdout hash trước source change và được
  cập nhật verdict khi đóng. Probe tạm có ảnh hưởng tới attribution hoặc quyết
  định reopen phải được chuẩn hóa vào `research/STATE.md`/`research/evidence/`;
  không để kết luận chỉ tồn tại trong hội thoại hoặc file `.tmp-*`.

### Thay đổi runtime không được miễn cổng score

#### Quyết định deadline hiện hành từ 2026-08-25

- Thể lệ không cố định thời gian phản hồi ở `5000 ms`; `daySeconds` và action
  deadline công khai của từng trận mới là giới hạn phản hồi có thẩm quyền.
- `5000 ms` có đúng một nghĩa hiện hành: hard cap của canonical main solve,
  role selection và việc tạo complete protected checkpoint. Nó không phải hard
  cap của toàn bộ response khi trận công bố một cửa sổ dài hơn.
- Production mặc định vẫn kết thúc ở checkpoint `5000 ms`. Chỉ một cơ chế
  continuation đã qua đầy đủ development, sealed holdout, protected matrix và
  BTC target-host gate mới được dùng phần public window còn lại; một candidate
  đang nghiên cứu không tự động thay đổi policy production.
- Không được đưa thời gian dư vào main search hoặc chạy lại solver. Phải đóng
  băng complete checkpoint `5000 ms`; continuation chỉ được thay checkpoint qua
  exact simulator, independent validator, comparator từ điển chính thức và
  certificate transition/ledger dominance trên mọi ngày không cuối. Ở ngày cuối,
  vì không còn future state, exact validity cùng official lexicographic
  non-regression/strict gain là certificate có thẩm quyền; không được áp đặt
  terminal-cell hay road-footprint equality làm mất tối ưu cuối trận. Deadline,
  failure, invalidity, no-gain hoặc mất certificate phải trả checkpoint đã đóng băng.
- Activation chỉ được phụ thuộc vào `daySeconds`, authoritative `endsAt`, thời
  điểm nhận state và transport safety; cấm route theo map, seed, fuel, family,
  bot, opponent hoặc match ID. Cửa sổ thiếu/không đáng tin vẫn fail-closed ở
  `5000 ms`.
- Mọi candidate deadline động phải có lane `5000 ms` chứng minh parent
  equivalence và ít nhất một public-window lane được đăng ký trước để chứng minh
  lợi ích causal. Chỉ mở thêm `45000/60000 ms` khi lane ngắn hơn còn
  deadline-limited hoặc chưa đạt fixed point; cấm vét thêm các lớp thời gian chỉ
  để tìm một lane thắng. BTC target-host vẫn bắt buộc trước promotion và local
  latency không có thẩm quyền về hiệu suất.

- Nhãn `runtime`, `correctness`, `network`, `deadline`, `cache` hoặc
  `performance` không miễn một commit khỏi promotion gate. Bất kỳ thay đổi nào
  tới deadline calibration, network reserve, budget partition, search order,
  cache hit/miss, scheduling hoặc lượng công việc hoàn thành trước cutoff đều
  phải được coi là score-affecting, trừ khi có bằng chứng operation-equivalent
  hoặc byte-equivalent trên runtime path thực.
- Commit trộn correctness với budget/search policy phải tách attribution. Phần
  correctness vẫn phải được giữ, nhưng phần policy chỉ được promote sau paired
  candidate-vs-direct-parent tại checkpoint `5000 ms`, các lớp public-window
  đã đăng ký và so với protected
  lane champion. Validity, timeout-free, unit-test green hoặc hạng 1 trước bot
  không thay thế score gate.
- `networkFloor` chỉ bảo vệ khoảng từ lúc kết thúc compute tới authoritative
  action deadline. Hai deadline phải tách rõ:
  `checkpoint_end = min(received_at + 5000 ms,
  action_deadline - transport_safety)` và, chỉ cho continuation đã được promote,
  `continuation_end = action_deadline - transport_safety`. Khi public window
  thiếu/không đáng tin hoặc không có continuation đã được promote, response dừng
  ở checkpoint. Outer window ngắn luôn thu hẹp checkpoint để giữ submission hợp
  lệ; outer window dài không bao giờ nới canonical main solve quá `5000 ms`.
- Khi search dài hơn có thể đổi trajectory, không được lấy kết quả cuối một cách
  máy móc. Phải giữ incumbent đã chứng nhận và chỉ cho phần search/refinement bổ
  sung thay thế khi exact simulator, independent validator và official
  lexicographic comparator chứng minh cải thiện; deadline, failure, invalid hoặc
  không cải thiện phải trả incumbent byte-identical.

## Trình tự khôi phục context bắt buộc

### Gate đọc tài liệu tuyệt đối

Sau khi đọc `AGENTS.md`, trước khi làm **bất kỳ hành động nào khác** ngoài thao
tác chỉ đọc để thu thập context, bắt buộc phải đọc đầy đủ các tài liệu nền và
tài liệu liên quan trực tiếp đến nhiệm vụ. "Hành động" bao gồm nhưng không giới
hạn: sửa file, tạo patch, build, test, benchmark, chạy BTC, dùng web, tạo match,
đưa ra thiết kế mới, kết luận logic, đánh dấu đạt trần, commit hoặc push.

Bộ tài liệu nền bắt buộc phải đọc trong mọi phiên làm việc:

1. `AGENTS.md`;
2. `Đề bài.md`;
3. `Kiến trúc.md`;
4. `API.md`;
5. `README.md`;
6. `old/results/HISTORICAL_TOURNAMENT.md`;
7. `old/CHECKPOINTS.csv`;
8. `research/STATE.md` và `research/EXPERIMENTS.csv` ;
9. mọi `AGENTS.md` sâu hơn áp dụng cho file sẽ chạm tới;
10. source, test, benchmark, replay và tài liệu gate liên quan trực tiếp tới
    runtime path hoặc giả thuyết đang xử lý.

Không được coi việc đã đọc tài liệu trong hội thoại cũ, memory hoặc trước compact
là thay thế cho việc đọc lại trong phiên hiện tại. Không được đọc lướt theo từ
khóa rồi tuyên bố đã hiểu toàn bộ; phải đối chiếu yêu cầu, kiến trúc, runtime
wiring, test và bằng chứng benchmark liên quan trước khi hành động.

Nếu tài liệu bắt buộc bị thiếu, không mở được, mâu thuẫn hoặc chưa xác định được
phạm vi áp dụng, phải dừng ở chế độ read-only, ghi rõ blocker và chưa được phép
nghiên cứu hay sửa logic. Không được tự điền phần thiếu bằng giả định.

### Trình tự sau khi đọc tài liệu

Trước mọi thử nghiệm hoặc sửa logic:

1. Xác minh `git status`, HEAD, parent commit và phạm vi dirty tree.
2. Đọc hoặc tạo `research/STATE.md` và `research/EXPERIMENTS.csv`.
3. Truy vết runtime path thực tế của đúng gap đang xét.
4. Xác định đúng một gap đang mở và bằng chứng cho gap đó.
5. Ghi rõ invariants, baseline và protected lanes trước khi thử nghiệm.
6. Nếu không hoàn thành đủ năm bước trên, cấm chạy thử nghiệm logic.

Không được dùng câu "quay lại high-fuel", "tiếp tục role", "thử lại ALNS"
nếu `research/STATE.md` không ghi axis đó đang mở cùng counterexample mới.

## Mốc lịch sử đã xác nhận ngày 2026-07-31

- Luật cho phép thời gian phản hồi thay đổi theo từng trận; cấu hình công khai của
  chính trận đang chạy là nguồn chân lý cho deadline.
- Trận đội-vs-đội cấu hình chuẩn `m-1042` hiển thị `60000 ms` mỗi ngày.
- Preflight BTC `m-1181` dùng `5000 ms`; HEAD vượt cả bốn gate, chậm nhất `279 ms`,
  kết quả `#1 · 1 loại · 20 phần`.
- Baseline short-deadline: `6132f41` (`baseline-btc`).
- Mốc đạt plateau general-score: `02df79d` (`exact-highfuel`).
- Current correctness/proof checkpoint: `6f84a06` (`current`).
- Báo cáo chuẩn: `old/results/HISTORICAL_TOURNAMENT.md`.
- Fixed-role synthetic, 2500 ms, 120 map: `02df79d`, `7ee0c8f`, `94d1ab8`,
  `1c0a0cd` và `6f84a06` đồng score 120/120.
- HEAD so với baseline ở lane trên: 33/64/23; `p=0.2288`, chưa chứng minh
  vượt trội thống kê.
- Native-role, 2500 ms, 60 map: HEAD so với baseline 20/28/12;
  `p=0.2153`, chưa chứng minh vượt trội thống kê.
- Fixed-role, 500 ms, 60 map: baseline thắng HEAD 57/2/1; đây chỉ là stress
  degradation nhân tạo, không phải lane chọn production khi cấu hình thi đấu là 60000 ms.
- Native-role, 500 ms, 60 map: baseline thắng HEAD 55/3/2; diễn giải tương tự.
- Mọi lane tournament hiện có: invalid = 0, emergency = 0.
- BTC-scale local: các bản mới khóa cùng lifetime/daily; servings gần cutoff
  dao động theo tải máy, không được dùng làm verdict production.

## Kết luận hiện tại

Từ ngày 2026-08-27, canonical production checkpoint là `18ecdd3`. Đây là điểm
dừng thi đấu thực dụng trên toàn bộ bằng chứng hiện có, không phải chứng minh tối
ưu toán học toàn cục. Không có source candidate hay experiment đang mở.

Các giới hạn còn lại và điều kiện mở lại:

1. **Exact position-then-sweep residual:** 209 chứng minh một số restrained
   positioning trajectory tốt hơn production nhưng chúng đổi trạng thái cuối
   ngày và không có certificate đơn điệu. Phần deep-sweep có certificate đã được
   210/215 khai thác; phần residual chỉ được mở lại khi có state-coupled proof mới
   chạy được trong deadline, không phải bằng cách tăng cap hay dùng realized
   suffix.
2. **Safe multi-day integration đã cạn hướng hiện tại:** 164, 167, 168, 183 và
   184 cho thấy strict proof trơ, còn paired actual-vs-actual có regression
   closed-loop. 229--232 tiếp tục loại diversification, frozen-proof injection,
   ACK rebasing và protected proof consumer. Không lặp lại các cơ chế này nếu
   không có invariant mới và fresh counterexample.
3. **Public-window capability đã active:** main/role/complete checkpoint vẫn có
   hard cap `5000 ms`; accepted continuation dùng phần cửa sổ công khai còn lại,
   giữ checkpoint khi deadline, failure, invalidity hoặc không có certified gain.
4. **BTC target-host gate hiện sạch:** `m-4476` chạy hard 24x24, low fuel, bảy
   ngày, cửa sổ 15000 ms với 7/7 ACK valid, 6/6 transition reconciled, response
   tối đa 6258 ms và zero safety failure. Thứ hạng bot không phải bằng chứng
   promotion.
5. **Rủi ro ngoài bằng chứng:** đối thủ thật mạnh hoặc cấu hình thi đấu mới vẫn
   có thể mở counterexample. Khi đó phải ghi replay/config mới và mở experiment
   mới trên seed/holdout chưa tiêu thụ; không được tuyên bố trần lý thuyết.

## Production line và research branches

- Production chỉ có một chuỗi commit canonical, không chấp nhận regression đã biết.
- Mọi nghiên cứu logic mới bắt đầu từ parent đóng băng mới nhất được ghi trong
  `research/STATE.md` và `research/EXPERIMENTS.csv`; không dùng một hash lịch sử
  cố định và không tái chạy toàn bộ lịch sử hoặc build lặp lại thay cho nghiên cứu.
- Chỉ được commit candidate khi nó thắng toàn diện parent và các lane champion
  theo nghĩa ưu thế tổng quát paired đủ lớn với downside bị giới hạn: zero
  invalid/emergency, canonical main không vượt `5000 ms`, toàn response không
  vượt authoritative deadline sau transport safety, kết quả được phân tầng theo
  map/fuel/horizon/opponent và gate BTC cuối cùng đạt. Thống trị tuyệt đối được ưu tiên,
  nhưng không loại máy móc một candidate chỉ vì một số ít map thua nhẹ. Mỗi trận
  vẫn so score từ điển chính thức; quyết định toàn cục báo W/T/L, tier khác đầu
  tiên, độ lớn gain/loss và tail downside, không cộng các tier bằng weighted sum.
  Candidate chỉ thắng cục bộ một lane vẫn phải bị reject hoặc giữ ngoài production
  cho tới khi có dispatcher khách quan đã qua holdout.
- Mỗi thử nghiệm bắt đầu từ đúng parent champion và nằm trong snapshot/branch riêng.
- Thử nghiệm thất bại không được trộn artifacts hoặc code vào production.
- Commit cũ không bị xóa; champion theo lane phải được giữ để A/B bất kỳ lúc nào.
- Một candidate tốt một lane nhưng giảm lane khác không được thay thế global.
  Chỉ được giữ sau dispatcher dựa trên điều kiện quan sát khách quan như deadline,
  map size, agent count, day count hoặc tỷ lệ fuel/daySteps, và dispatcher phải qua
  holdout độc lập. Cấm route theo seed hoặc tên family.

## Protected acceptance matrix

Mọi logic candidate phải được so trực tiếp với parent và các lane champions trên:

- checkpoint lane bắt buộc: `5000 ms` là hard cap cho canonical main solve, role
  selection và complete protected checkpoint ở mọi candidate;
- public-window lanes chỉ dành cho protected continuation đã đăng ký; main solve
  vẫn bị chặn ở `5000 ms`, continuation bị chặn bởi authoritative deadline trừ
  transport safety, và lane `5000 ms` phải parent-equivalent;
- preflight `5000 ms` kiểm token/protocol/tốc độ BTC và có đầy đủ quyền gate;
- budget chẩn đoán degradation: `500/1200/2500 ms`; các lane này vẫn phải không
  crash/invalid, nhưng không được tự mình promote hoặc block production nếu BTC
  không công bố deadline tương ứng;
- fuel: low, default và high;
- map: generated 8x8 và BTC-like 32x32;
- roles: fixed và native/exhaustive;
- horizon: 4/5 ngày và 10 ngày;
- traffic: sáu family cùng endogenous own traffic hai ngày;
- exact simulator và independent validator;
- BTC target-host cho latency/p95/p99 khi thay đổi liên quan deadline/performance.

BTC là môi trường phán quyết cuối. Local chỉ dùng để phát triển, falsify giả thuyết,
kiểm semantics và sàng lọc candidate. Không được dùng latency, throughput hay dao
động cutoff trên máy local để kết luận hiệu suất, competition readiness hoặc commit
readiness. Mọi claim hiệu suất phải có telemetry từ BTC target-host trong cùng
deadline/config; nếu mất kết nối BTC thì được tiếp tục nghiên cứu local nhưng cấm
promotion và commit candidate.

Performance-only change phải giữ byte-equivalent action hoặc ít nhất exact score,
state transition và validator result trên frozen equivalence suite. Logic change
phải chứng minh ưu thế paired tổng quát trên protected lanes và frozen holdout,
đồng thời lượng hóa regression theo tier khác đầu tiên và tail downside. Không đặt
một tỷ lệ W/L cố định thay cho phán quyết tổng quát; regression hiếm và nhỏ có thể
chấp nhận, còn regression lớn/có hệ thống theo family hoặc fuel thì không. Không
được đổi metric sau khi mở holdout.

### Luật chống overfit holdout

- Holdout phải được sinh và hash trước source change; candidate, binary, manifest,
  comparator, acceptance gate và các strata bắt buộc phải đóng băng trước lần mở
  đầu tiên.
- Holdout chỉ được mở đúng một lần và chỉ có quyền accept/reject chính candidate
  đã đóng băng. Kết quả từng phần chỉ được dùng cho kill condition đã đăng ký và
  giám sát lỗi vận hành; cấm dùng để sửa code, đổi ngưỡng, đổi dispatcher, đổi
  metric, thêm/bớt strata hoặc quyết định dừng ở một prefix thuận lợi.
- Sau khi đã quan sát, toàn bộ holdout trở thành consumed evidence. Không được
  chuyển seed/case thua sang development, không được dùng lại nó để promote một
  successor, và không được gọi một patch được thiết kế từ các case đó là xác nhận
  độc lập.
- Candidate bị reject chỉ được mở lại bằng một experiment mới có cơ chế được suy
  ra từ invariant/counterexample tổng quát, manifest mới lấy từ seed pool chưa mở
  và reopen condition đã ghi trước. Candidate được accept vẫn phải qua production
  integration equivalence, protected lanes và BTC; nếu integration đổi score
  logic thay vì chỉ hiện thực đúng cơ chế đóng băng thì phải mở experiment và
  holdout mới.

## Hồ sơ thử nghiệm bắt buộc

Mỗi dòng trong `research/EXPERIMENTS.csv` phải có:

- experiment id và ngày;
- parent commit và candidate commit/diff;
- gap/counterexample quan sát được;
- cơ chế đề xuất;
- invariants không được phá;
- development split và kết quả;
- frozen holdout hash và kết quả;
- BTC result nếu liên quan runtime;
- verdict: accepted, rejected hoặc inconclusive;
- điều kiện duy nhất cho phép mở lại nếu rejected.

Không được tune tiếp trên holdout đã mở. Không được gọi một axis là "đạt trần"
chỉ vì một thử nghiệm thất bại. Một lane chỉ được ghi practical ceiling khi hai
research sweep độc lập không có candidate qua gate và không còn gap telemetry có
thể tác động tới score.

## Lệnh nghiên cứu tiếp theo được phép

`research/STATE.md` là nguồn duy nhất chỉ ra experiment/gap đang mở và bước tiếp
theo. Governance bootstrap đã hoàn tất; không được quay lại lệnh bootstrap lịch sử
hoặc hash `6f84a06`. Mỗi vòng chỉ xử lý gap đã đăng ký từ parent đóng băng hiện hành.
Lịch sử chỉ dùng làm lane champion để A/B khi candidate chạm đúng lane, không chạy
lại toàn bộ như công việc chính. Public window dài chỉ cấp thời gian cho protected
continuation theo luật deadline hiện hành; không cấp thêm thời gian cho main solve.
Short-deadline `500 ms` chỉ được mở lại nếu BTC công bố deadline tương ứng hoặc
telemetry production cho thấy cửa sổ thực tế bị co tới mức đó.

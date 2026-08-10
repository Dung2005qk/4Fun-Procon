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
  chứng minh không thể khai thác trong hard cap, hai sweep nghiên cứu độc lập
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

## Trạng thái đã xác nhận ngày 2026-07-31

- Luật cho phép thời gian phản hồi thay đổi theo từng trận; cấu hình công khai của
  chính trận đang chạy là nguồn chân lý cho deadline.
- Trận đội-vs-đội cấu hình chuẩn `m-1042` hiển thị `60000 ms` mỗi ngày.
- Preflight BTC `m-1181` dùng `5000 ms`; HEAD vượt cả bốn gate, chậm nhất `279 ms`,
  kết quả `#1 · 1 loại · 20 phần`.
- Quyết định vận hành ngày 2026-07-31: `5000 ms` là hard cap chính cho toàn bộ
  solver/role-selection candidate. Cửa sổ server `60000 ms` chỉ là outer deadline
  và không cho phép search, certification hay promotion tiêu quá `5000 ms`.
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

UDON-SHIELD chưa đạt trần kiến trúc thực tế. Các tuyên bố "đạt trần" trước đây
chỉ là trần cục bộ của một axis hoặc một holdout, không phải trần toàn hệ thống.

Các gap đã xác nhận:

1. **Evaluation-budget mismatch:** tournament lịch sử chưa chạy đầy đủ tại hard cap
   chính `5000 ms`; cửa sổ server `60000 ms` không được chuyển thành compute budget.
   Kết luận từ `500/2500 ms` không được dùng để chọn production.
2. **General-score plateau:** từ `02df79d` đến HEAD, nhiều cơ chế correctness,
   bound và runtime tốt hơn nhưng không tăng score trên 120 map primary.
3. **Không có promotion monotonic toàn cục trước tournament:** thay đổi từng axis
   từng được chấp nhận mà chưa bắt buộc chạy lại toàn bộ protected matrix.
4. **Chưa chứng minh ưu thế thống kê so với baseline tại ngân sách thi đấu:**
   hướng kết quả tích cực ở 2500 ms không thay thế tournament 60000 ms.
5. **BTC-scale tier 3 chưa ổn định khi đo local:** cần BTC target-host để kết luận
   latency và servings gần deadline.
6. **Hiệu lực runtime của cơ chế phức tạp chưa được attribution đầy đủ:** code có
   thể tồn tại và được gọi nhưng chưa chứng minh mỗi cơ chế làm thay đổi incumbent
   hoặc score trong lane cần thiết.
7. **Đối thủ thật chưa đủ đa dạng:** chưa có bằng chứng tổng quát trước nhiều kiểu
   đối thủ người chơi/adversarial khác nhau.

## Production line và research branches

- Production chỉ có một chuỗi commit canonical, không chấp nhận regression đã biết.
- Mọi nghiên cứu logic mới bắt đầu từ checkpoint đóng băng mới nhất `6f84a06`,
  không tái chạy toàn bộ lịch sử hoặc build lặp lại như một thay thế cho nghiên cứu.
- Chỉ được commit candidate khi nó thắng toàn diện parent và các lane champion
  theo nghĩa ưu thế tổng quát paired đủ lớn với downside bị giới hạn: zero
  invalid/emergency, không vượt hard cap, kết quả được phân tầng theo map/fuel/
  horizon/opponent và gate BTC cuối cùng đạt. Thống trị tuyệt đối được ưu tiên,
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

- budget chính và hard cap: `5000 ms` cho solver và role selection ở mọi candidate;
- outer deadline `60000 ms` của cấu hình thi đấu chuẩn chỉ dùng kiểm lifecycle/network;
  internal compute budget vẫn bị chặn ở `5000 ms`;
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

Hiện tại cấm nghiên cứu logic mới cho tới khi hoàn thành governance bootstrap tối thiểu:

1. tạo `research/STATE.md`;
2. tạo `research/EXPERIMENTS.csv`;
3. tự động hóa gate candidate-vs-parent/champion từ `old/harness`; không bắt buộc
   tái đấu toàn bộ checkpoint lịch sử trước mỗi vòng nghiên cứu;
4. ghi champion theo từng lane và reopen conditions;
5. chứng minh một dry-run có thể từ chối candidate giả lập gây invalid,
   regression tier 1/tier 2 hoặc runtime vượt hard cap tại lane `5000 ms`, kể cả
   khi outer server window của lane là `60000 ms`.

Sau bootstrap, bắt đầu từ `6f84a06`, dùng telemetry/counterexample BTC ở hard cap
`5000 ms` để chọn đúng một gap score rồi phát triển candidate. Lịch sử chỉ dùng làm
lane champion để A/B khi candidate chạm đúng lane, không chạy lại toàn bộ như công
việc chính. Outer-window `60000 ms` không cấp thêm compute.
Short-deadline `500 ms` chỉ được mở lại nếu BTC công
bố deadline tương ứng hoặc telemetry production cho thấy cửa sổ thực tế bị co tới
mức đó; không được ưu tiên nó chỉ vì stress-test cũ thua.

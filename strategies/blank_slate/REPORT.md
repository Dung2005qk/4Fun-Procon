# Báo cáo blank-slate strategy challenge

> **Trạng thái lịch sử — đã được tích hợp.** Báo cáo này đo phiên bản
> UDON-SHIELD trước khi `blank-portfolio` được đưa vào role rollout và production
> day search. Không dùng kết luận “blank mạnh hơn production” như trạng thái
> hiện tại; xem `../../AUDIT_KIEN_TRUC_TOAN_CUC.md` cho build và holdout sau tích
> hợp. Raw output bên dưới được giữ nguyên để bảo toàn provenance nghiên cứu.

Ngày chốt: 2026-07-26.

## Kết luận

`blank-portfolio` là chiến lược mạnh nhất trong benchmark hiện tại. Trên
holdout 300 map chưa từng mở, nó thắng `udon-shield` **253/25/22**, đạt tỷ lệ
thắng 92.00% trên các cặp phân thắng bại, Wilson 95% `0.8818..0.9466`, sign
test hai phía `p = 6.263041e-51`, không có plan invalid và không mất điểm
lifetime trên bất kỳ map nào.

Ứng viên thắng ở cả sáu family. Vì vậy nó vượt toàn bộ tiêu chí đã khai báo
trước trong `HOLDOUT_FREEZE.md`, gồm cả gate tổng quát chứ không chỉ gate tổng
điểm.

Đây là bằng chứng thực nghiệm mạnh trên mô hình benchmark đã định nghĩa, không
phải chứng minh tối ưu toàn cục theo nghĩa toán học và không loại trừ model
shift trong test bí mật của ban tổ chức.

## Giao thức công bằng

- Mỗi cặp dùng cùng seed, map, stock, day length và common-opponent footprint.
- Mỗi solver chịu traffic do chính route của nó tạo ra qua lookback hai ngày.
- Score được so sánh đúng thứ tự từ điển chính thức:
  `(lifetime distinct, total daily distinct, total servings)`.
- Không dùng weighted surrogate để quyết định thắng thua.
- Mỗi plan được chạy bằng `ExactStepSimulator` và đối chiếu lại bằng
  `IndependentDayValidator`.
- Có hai phép đo: native roles để kiểm tra toàn pipeline và common roles để
  cô lập chất lượng day planner.
- Ngân sách chính là 1200 ms/ngày; stress test dùng 500 ms/ngày.
- Holdout `300000..300299` được hash và khóa protocol trước khi mở; sau khi mở
  không có thay đổi thuật toán.

## Ba hướng độc lập

### Event-conflict search

- Sinh route cấp agent bằng Pareto path theo đồng thời travel step và patrol
  fuel.
- Xây lịch rendezvous với tanker và cho phép tiếp nhiên liệu lặp, không còn
  giới hạn giả tạo hai lần.
- Tìm kiếm best-first trên tích Descartes các route.
- Khi exact simulation phát hiện tranh chấp stock, chỉ mở rộng các agent liên
  quan tới claim không được phục vụ.
- Mọi incumbent tốt hơn đều được kiểm bằng validator độc lập.

Hướng này lấy ý tưởng phân rã low-level/high-level từ
[Conflict-Based Search](https://ojs.aaai.org/index.php/AAAI/article/download/8140/7998),
nhưng không tuyên bố CBS-optimal: conflict của bài này là stock/resource và
fuel rendezvous, không phải vertex collision chuẩn MAPF.

### Backward-deadline planning

- Xếp ưu tiên rare brand, brand chưa có, khoảng cách và stock.
- Xây chuỗi target ngược từ endpoint khẩn cấp rồi gán cho patrol.
- Hoàn toàn deterministic và rất nhanh.

Hướng này bị loại vì chất lượng thấp ổn định; tốc độ không bù được mất score.

### Macro MCTS

- State macro gồm hub, chuỗi spot từng patrol và số lần spot đã được gán.
- UCT cân bằng exploitation/exploration.
- Rollout dùng deterministic seed và tối đa 1600 lượt/ngày.
- Mỗi leaf được biến thành plan hoàn chỉnh rồi exact-simulate, không dùng
  learned value giả.

Nền tảng UCT đến từ
[Bandit Based Monte-Carlo Planning](https://link.springer.com/chapter/10.1007/11871842_29).

### Portfolio cuối

- Chia ngân sách 50/50 cho event-conflict và macro MCTS.
- Exact-evaluate cả hai plan dưới cùng state/ledger.
- Chọn bằng score từ điển chính thức, sau đó mới dùng terminal fuel và khoảng
  cách tới brand còn thiếu làm tie-break.

Hai thành phần thực sự bổ sung nhau. Trên validation 120 map, portfolio thắng
event-conflict `56/60/4` và thắng MCTS `89/19/12`.

## Kết quả trước holdout

Tất cả số dưới đây là `win/tie/loss` so với `udon-shield`.

| Split | Roles | Budget | Event | Backward | MCTS | Portfolio |
|---|---:|---:|---:|---:|---:|---:|
| Train 30 | common | 1200 ms | 22/2/6 | 7/1/22 | 22/2/6 | **25/1/4** |
| Validation 120 | common | 1200 ms | 71/8/41 | 25/3/92 | 73/11/36 | **94/9/17** |
| Confirm 120 | native | 1200 ms | 79/8/33 | 32/2/86 | 62/10/48 | **87/12/21** |
| Confirm 120 | common | 500 ms | 84/4/32 | 30/2/88 | 70/7/43 | **97/7/16** |

Ở stress test 500 ms, portfolio có p95 120 ms còn `udon-shield` là 243 ms.
Lợi thế không đến từ việc dùng thêm thời gian.

## Holdout 300 map

### Tổng thể

| Strategy | W/T/L | Lifetime delta | Daily delta | Servings delta | p95 | max | Invalid |
|---|---:|---:|---:|---:|---:|---:|---:|
| `blank-portfolio` | **253/25/22** | 0 | +124 | +1512 | **120 ms** | 194 ms | 0 |
| `udon-shield` | baseline | 0 | 0 | 0 | 362 ms | 831 ms | 0 |

- Match win/tie/loss rate: `84.33% / 8.33% / 7.33%`.
- Decisive win rate: `92.00%`.
- Wilson 95%: `0.8818..0.9466`.
- Exact paired sign test: `p = 6.263041e-51`.
- Trung bình mỗi match: `+0.413` daily distinct và `+5.04` servings.
- p95 giảm `66.9%`; max giảm `76.7%`.

### Theo family

| Family | W/T/L | Decisive win rate | Sign-test p |
|---|---:|---:|---:|
| Balanced | 44/5/1 | 97.78% | 2.614797e-12 |
| Rare-brand | 39/10/1 | 97.50% | 7.457857e-11 |
| Threshold-corridor | 41/4/5 | 89.13% | 4.405936e-8 |
| Fuel-tight | 36/1/13 | 73.47% | 1.402689e-3 |
| High-stock | 49/1/0 | 100.00% | 3.552714e-15 |
| Overnight | 44/4/2 | 95.65% | 3.075229e-11 |

Fuel-tight vẫn là family yếu nhất tương đối, nhưng holdout cho thấy nó không
còn là family thua: candidate thắng có ý nghĩa thống kê.

## Các thử nghiệm bị loại

### Tanker hai hub

Mobile two-hub không cải thiện fuel-tight và làm train common giảm từ
`25/1/4` xuống `24/2/4`; p95 tăng từ 125 ms lên 197 ms. Phần này bị xóa.

### Role rollout một ngày có all-patrol

Nó chọn phương án tốt ở ngày 1 nhưng bỏ qua việc nhiên liệu không reset giữa
các ngày. Train giảm xuống `4/2/24`. Đây là ví dụ trực tiếp cho thấy one-day
role rollout là logic thiếu đối với state nhiều ngày.

### Role rollout bốn ngày quá nông

Chia 500 ms cho mọi assignment và mọi ngày làm mỗi solve quá nông; portfolio
chỉ còn `21/2/7` trên train. Phần này bị loại.

### Rollout chỉ chọn danh tính tanker

Confirm tăng nhẹ từ `87/12/21` lên `88/13/19`, nhưng mất gần 500 ms khởi tạo và
fuel-tight vẫn `7/0/13`. Lợi ích không đủ bù độ phức tạp nên bị loại.

### Backward-deadline

Validation common chỉ đạt `25/3/92`. Nó được giữ làm negative control, không
phải production candidate.

## Vì sao portfolio thắng

1. Event-conflict tập trung compute vào conflict thật phát hiện bởi exact
   simulator, thay vì mở rộng đồng đều route pool.
2. MCTS tìm được các chuỗi spot khác cấu trúc mà best-first local route
   combination khó sinh.
3. Exact portfolio selection loại rủi ro chọn nhầm do reward gần đúng.
4. Candidate tối ưu trực tiếp score hiện tại nên ít overhead hơn kiến trúc
   shield/proof/scenario rộng.
5. Hai solver có failure mode khác nhau; số liệu pairwise xác nhận portfolio
   không chỉ sao chép solver mạnh hơn.

Kết quả tương thích với kinh nghiệm từ vehicle routing: metaheuristic mạnh
thường cần nhiều cơ chế tìm kiếm bổ sung nhau, nhưng chỉ nên giữ phần cải thiện
được xác nhận. Tham khảo
[Hybrid Genetic Search for the CVRP](https://arxiv.org/abs/2012.10384) và
[DIMACS Vehicle Routing Challenge](https://dimacs.rutgers.edu/index.php/programs/challenge/vrp/).

## Giới hạn và quyết định triển khai

- Đã chứng minh: ứng viên thắng rõ, nhanh hơn và tổng quát qua sáu family trong
  simulator hiện tại với seed tách biệt.
- Chưa chứng minh: global optimum của bài toán; parity với phân phối map bí mật;
  ưu thế khi ban tổ chức thay luật hoặc traffic/opponent model.
- Chưa tích hợp vào CLI production. Code hiện nằm trong benchmark strategy để
  giữ đối chứng độc lập.

Khuyến nghị kỹ thuật là đưa event-conflict và macro MCTS vào production như
hai candidate generator mới, giữ exact simulator/validator và các lớp
multi-day safety hiện có. Không nên xóa shield, scenario hoặc proof chỉ vì
benchmark day-planner hiện tại thua.

## Artifact và tái lập

- Freeze: `strategies/blank_slate/HOLDOUT_FREEZE.md`
- Planner: `strategies/blank_slate/planners.hpp`,
  `strategies/blank_slate/planners.cpp`
- Harness: `strategies/strategy_suite.cpp`
- Holdout raw output: `strategies/blank_slate/final_holdout_300_native.txt`
- Holdout SHA-256:
  `5f28a11ae1d1267d50febb3edd43a94a17a338650f71db5a17e60fe178f62631`
- Validation: `ctest --test-dir build-release --output-on-failure`:
  3/3 test passed.

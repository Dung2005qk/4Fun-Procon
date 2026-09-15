# 367 — Nguyên nhân các chênh lệch inactive 5000 ms

2026-09-10. Kết luận: **không tìm thấy lỗi đảo thứ tự score ở comparator cuối;
đã xác định các điểm phát sinh chênh lệch trong search có cutoff và đánh giá
tương lai không đồng đều. Đây không phải tác hại của resource suffix362. Tuy
nhiên, “không do362” không đồng nghĩa “mọi dao động đều nhỏ hoặc không đáng sửa”.**

Không sửa production, không giảm/xóa chức năng, không chạy solver/build, không
đụng VM hoặc đọc điểm protected366-P2. Chỉ phân tích evidence đã hoàn tất. P2 tiếp
tục đúng amendment của người dùng; báo cáo này không tạo thêm một cổng minh oan.

## Phạm vi và kiểm chứng

Toàn bộ42 fixture inactive:12 của362DEV,12 của366DEV,18 của366holdout đã consumed.
168 lần chạy gốc,1128 decision ngày,168 so sánh AB/AA. Rehash cả3654 file của ba
completion manifests trước phân tích. Không chọn riêng các trận thua. Có29 so
sánh khác trajectory, nhưng chỉ11 fixture độc lập:14 so sánh/5 fixture khác ở
main;15 so sánh/6 fixture khác sau main. AB và AA dùng lại cùng lần chạy nên
không phải những counterexample độc lập bổ sung.

Ở mọi điểm khác đầu tiên: roles, normalized incoming state, ledger và manifest
đều giống nhau. Không có resource frame trong168 lần inactive. Đây bổ sung cho
compiled/lifecycle proof360/361/364/365, không thay thế proof bằng telemetry rỗng.
Kết quả đầy đủ nằm trong [inventory367](C:/Users/LMC/Desktop/4Fun/research/evidence/ATTR-INACTIVE-EXECUTION-DIVERGENCE-367.json)
và [explanation367](C:/Users/LMC/Desktop/4Fun/research/evidence/ATTR-INACTIVE-EXECUTION-DIVERGENCE-367-explanation.json).

| Evidence inactive | AB W/T/L | AA W/T/L | AB/AA khác trajectory |
|---|---:|---:|---:|
| 362DEV | 0/21/3 | 0/23/1 | 5/2 |
| 366DEV | 3/21/0 | 1/23/0 | 4/2 |
| 366holdout | 6/27/3 | 3/29/4 | 9/7 |

AA là cùng chế độ giữa hai lượt; hướng so luôn B so với A. Những số này chứng
minh sự biến thiên cũng xảy ra khi không đổi treatment, **không** chứng minh phân
phối nhiễu đối xứng, không có bias vận hành, hay p-value cho ưu thế362.

## Ba trận thua366holdout: đã truy được gì?

### 1. Seed202609093760089: mất2 servings, nằm ở refinement chứ không phải main

Main chọn cùng plan, cùng certificate6/20/156 và cùng current score6/6/53.
Ngày1 parent refinement thử394 plan, đạt1 acceptance và53→54 servings;
candidate thử7 plan,0 acceptance, giữ53. Cả hai báo deadline reached.
Ngân sách refinement1644/1643ms, main2254/2256ms. Kết quả ngày1 còn cùng
terminal agent state và road footprint, nhưng lệch1 serving.

Ngày2 main vẫn cùng plan, refinement58 so với57 servings: lệch thêm1.
Ngày3 và4 hai bên thu bằng nhau66 và75; tổng253 so với251. **Đã giải thích đủ
hai servings bằng hai cơ hội refinement không cùng được hoàn thành**, không
phải cộng score sai, gửi mất ACK, hoặc final comparator bỏ nghiệm tốt đã chứng nhận.

Nguồn [slack_refiner.cpp:944](C:/Users/LMC/Desktop/4Fun/src/slack_refiner.cpp:944)
dùng tối đa4 worker lấy agent từ atomic queue, mỗi query có cùng absolute
search deadline; sau join mới xét route theo thứ tự agent. Vòng xét route kiểm
deadline ở từng lượt tại[1004](C:/Users/LMC/Desktop/4Fun/src/slack_refiner.cpp:1004).
Target-terminal followup chỉ mở khi vòng trước đạt fixed point tại[1096](C:/Users/LMC/Desktop/4Fun/src/slack_refiner.cpp:1096).
Đó là đường phụ thuộc thời gian/công việc có thật. Log không có per-worker CPU
time, preemption hay thứ tự hoàn thành từng query, nên **không thể gọi chính xác
OS event nào gây394 so với7 là đã chứng minh**. Chênh1ms ngân sách không đủ để
tự khẳng định rằng chính1ms đó gây toàn bộ khác biệt.

### 2. Seed202609093760236: mất3 daily; đã xác định cả F0 operand và tie-break

Đây là trường hợp cụ thể hơn kết luận363 trước đó. Manifest chỉ có **một
scenario**. Vì vậy provisional lower bound ở audit chính là outcome score và
mọi provisional quantile; không còn mơ hồ do thiếu các quantile khác.

Hai plan cùng current6/6/58 đều tồn tại trong cả hai post-F0 pools:

| Plan identity prefix | F0 ở parentB | F0 ở candidateB | Patrol fuel reserve |
|---|---|---|---:|
| 7EFD070B4044 | 6/18/119 | 6/18/119 | 1275 |
| 917991FF80C2 | 6/15/100 | 6/18/119 | 1276 |

ParentB:7EFD thắng bằng F0 score. CandidateB:9179 đã đạt cùng F0 score, rồi
thắng tie-break thêm1 fuel. Mọi brand lifetime đã có, nên cả hai remaining-brand
distance bằng0; overnight spot count cùng8. Đây đúng thứ tự đang được lập trình
trong[better_provisional_evaluation](C:/Users/LMC/Desktop/4Fun/src/decision.cpp:1076)
và[compare_terminal_slack](C:/Users/LMC/Desktop/4Fun/include/udon/planner.hpp:231),
**không phải comparator đảo dấu**.

F0 chạy tuần tự với chung absolute deadline; cùng một plan9179 có mức đánh giá
tương lai khác nhau. Các entry cuối pool rơi về đúng current-only6/6/58. Trong
source, F0 kiểm deadline trước scenario, trước ngày và trong generator/master;
khi hết thời gian nó giữ phần outcome đã có. Xem[provisional_profile](C:/Users/LMC/Desktop/4Fun/src/decision.cpp:2073),
[f0Deadline](C:/Users/LMC/Desktop/4Fun/src/decision.cpp:4344),
[vòng profiling](C:/Users/LMC/Desktop/4Fun/src/decision.cpp:5038).
309/308ms candidate preparation đưa hai lượt tới vùng biên F0. Điều đã quan sát
chắc chắn là **evidence depth khác nhau của cùng candidate**, không phải full
future objective tự thay đổi. Không có log từng check để chỉ ra chính query nào
bị ngắt; cache/work-prefix và elapsed inputs cũng có thể tham gia.

W1 chỉ chứng nhận floor leader/upside/incumbent. Ở mỗi lượt, plan được chọn
bên kia nằm ở trạng thái `not-shortlisted`, không phải bị final comparator loại
dù đã có certificate tốt hơn. Các certificate riêng của7EFD và9179 lần lượt
6/44/291 và6/44/289; chúng không cùng nằm trong một final certified pool.
Không được lấy hai certificate từ hai lần chạy làm bằng chứng đã chạy một
counterfactual swap đủ điều kiện. Xem[W1 queue](C:/Users/LMC/Desktop/4Fun/src/decision.cpp:5122).

Hậu quả thực: ngày1 cả hai gửi6/6/63 nhưng agent7 dừng ở682 so với353, fuel157
so với158. Quỹ đạo về sau khác. Tới cuối ngày7 parent phủ đủ6 brand ở các ô
đang đứng; candidate chỉ phủ5, thiếu brand5. Ngày8–10 cả hai gửi cùng WAIT-only
actions; candidate vì đứng ở tập ô khác chỉ nhận5 daily mỗi ngày thay vì6.
Đây là đúng nguồn của daily−3, trong khi tổng servings lại+10. Không gán toàn bộ
hậu quả10 ngày cho một tie-break bằng phép can thiệp chưa chạy; nhưng chuỗi
boundary→trajectory→ba daily thiếu đã được quan sát trực tiếp.

Kiểm tra AA: parentA6/57/349→parentB6/60/349; candidateA6/60/350→candidateB6/57/359.
Daily regression đổi phía theo execution, không cố định theo treatment.

### 3. Seed202609093760273: mất11 servings, khác pool trước W1

AB_A:397→408; AB_B:408→397. ParentAA cũng397→408, candidateAA408→397.
Hai plan thắng ở hai lượt không tồn tại trong post-F0 audit của lượt kia.
Do audit được ghi **sau** admission, chưa thể phân biệt chắc chắn chưa sinh ra
với bị pre-F0 prune. Không được gọi đó là final comparator chọn sai.

Work prefix thực sự khác: master11823 so với11799 combinations, cả hai
deadline-limited; independent rollouts260 so với262; synthesized routes50 so
với52. Hai phía cùng current6/6/39 nhưng W1 chọn hai upside challenger với
certificate6/53/185 so với6/50/142. Mỗi plan thắng đúng các đối thủ đã chứng
nhận trong lần chạy của nó. Sau đó trajectory khác qua nhiều ngày; final408
so với397. Không có một “miss11-serving move” đơn lẻ được chứng minh.

## Comparator có lỗi không?

Đã kiểm lại **792 decision singleton-scenario** trên toàn bộ evidence, gồm
confidence gate, current-floor filter và official lexicographic score: **0 lần
chọn thấp hơn một candidate certified còn eligible ở tier quantile**.
336 decision nhiều scenario thiếu full per-candidate distributions để dựng lại
toàn bộ comparator nên không tuyên bố đã chứng minh tất cả tie-break của chúng.
Các trường hợp tied score cũng không được báo nhầm là lỗi chỉ vì khác stableId.

9 synthetic tests kiểm comparator inversion, thứ tự tier, current floor, tie,
multiscenario abstention, malformed singleton, missing selection và hash/path
tampering đều pass. Đây là test của forensic checker, không phải product gate.

## Có fix được không; có cần fix ngay không?

**Không có correctness hotfix nhỏ đã được chứng minh cần áp dụng lúc này.**
Không có chứng cứ suffix362 chạy ngầm, comparator đảo thứ tự hay protocol làm
mất score trong các boundary này. Không sửa candidate đang frozen hoặc đổi P2.

Nhưng có một vấn đề chất lượng tổng quát có thật: wall-clock search tạo pool
khác, và F0 có thể so những witness đã được làm sâu không đồng đều. Một khác
biệt nhỏ ở admission đổi terminal placement, rồi tác động daily ở cuối horizon.
Do đó không gọi−3 daily là “nhiễu vô hại không bao giờ cần quan tâm”.

Hướng đáng xét nếu mở nghiên cứu tiếp là **độ nhất quán của evidence cấp cho W1
ở biên deadline**, với prefix công việc có thể kiểm chứng trên candidate chung;
không phải ưu tiên seed này, sửa một fuel tie-break cho thắng replay, tắt worker,
giảm capability hoặc mặc định tăng cap. Đổi lịch profiling/admission là thay đổi
score policy, chưa phải sửa semantics-equivalent, nên cần prospective evidence
mới và phải chứng minh không mất breadth/gain hiện có. Các thử nghiệm admission
239/240/316/327 đã được363 đối chiếu; báo cáo này không tự cho phép lặp lại chúng.

Đóng367 với verdict **located execution/work-sensitive boundaries; no proven
new correctness defect; exact historical OS trigger unresolved**. Đường nâng
parent362/366 tiếp tục độc lập theo P2; bất kỳ nghiên cứu tối ưu mới nào vẫn phải
đăng ký riêng sau quyết định parent, không chen vào lượt protected đang chạy.

## Provenance và khả năng lặp lại

- Inventory SHA256 `D839273028262C905C88A985AA6FFAB700CEFB89E2A71EF080F128BEF4384FD8`.
- Explanation SHA256 `BF44A5495A2BEF42C2A82E29AD5086A98B03A1A7198C06F5435FC9F61C8D1C27`.
- Audit script SHA256 `8312229161FAC8F347772D49828BC2AA5099373B606BCBFE0264253DA01E4A95`.
- Explain script SHA256 `126CA5E1A997C231103DEA5886E9B73EF35CC9DCBC28635471C19FCEA916ED09`.
- Tests SHA256 `DA441C70F9E844355496C5EE33FECA06FD988CD49D26C49C3A438E04921C748F`.
- [Preregistration](C:/Users/LMC/Desktop/4Fun/research/evidence/ATTR-INACTIVE-EXECUTION-DIVERGENCE-367-preregistration.md),
  [audit script](C:/Users/LMC/Desktop/4Fun/research/probes/audit_execution_divergence_367.py),
  [explanation script](C:/Users/LMC/Desktop/4Fun/research/probes/explain_execution_divergence_367.py),
  [synthetic tests](C:/Users/LMC/Desktop/4Fun/research/probes/test_execution_divergence_367.py).
- Per-completion/source hashes và mọi replay path nằm trong JSON. Canonical
  decision.cpp khớp normalized frozen source; toàn bộ prefix slack_refiner trước
  method resource mới khớp. Không tuyên bố cả research file giống production.
- Analyzer lần đầu dừng trước khi ghi kết quả vì giả định trường authorization
  nằm trong protected frame; đã sửa theo schema thực bằng setup/window checks.
  Explain analyzer dừng trước ghi khi assert cả refiner-file giống nhau; diff
  chứng minh phần khác chỉ là method resource bổ sung, đã sửa phạm vi equality
  thành toàn canonical prefix. Không chạy lại solver, bỏ dữ liệu hay tạo kết quả
  thắng giả từ các lỗi công cụ này. Output dùng exclusive-create, không overwrite.

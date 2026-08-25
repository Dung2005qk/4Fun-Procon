# Kiến trúc UDON-SHIELD

## 1. Phân rã hệ thống

UDON-SHIELD được tổ chức thành các lớp có hướng phụ thuộc một chiều:

```text
Protocol Host
    │
    ▼
Match Session
    │
    ▼
Decision Engine
    ├── Role Selection
    ├── Traffic Belief and Scenario Generation
    ├── Viability Analysis
    ├── Route Portfolio Generation
    ├── Route Master and Adaptive Improvement
    ├── Future Witness Construction
    └── Certified Lexicographic Selection
    │
    ▼
Protected Slack Refinement
    │
    ▼
Exact Simulator and Independent Validator
    │
    ▼
Submission, Acknowledgement and Replay
```

### 1.1. Protocol Host

Protocol Host sở hữu giao tiếp với môi trường bên ngoài. Thành phần này:

- parse cấu hình trận và trạng thái ngày từ biểu diễn wire;
- chuyển chỉ số ngày, action và trạng thái đường sang domain model nội bộ;
- quản lý vòng đời HTTP, retry và deadline của request;
- serialize role assignment và day plan;
- ghi các event giao thức vào replay;
- chỉ chuyển acknowledgement hợp lệ vào `MatchSession`.

Protocol Host không chứa logic chọn route. Nó chỉ cung cấp dữ liệu có thẩm quyền,
giữ nguyên body của một submission trong quá trình retry và áp dụng fail-closed khi
không xác định được trạng thái của request.

### 1.2. Match Session

`MatchSession` là ranh giới trạng thái của một trận. Một session sở hữu một
`UdonShieldEngine` và duy trì ba miền trạng thái tách biệt:

- trạng thái đã được môi trường xác nhận;
- decision đang chờ acknowledgement;
- decision gần nhất đã được acknowledgement.

Session không cho phép xử lý authoritative state mới khi còn một submission chưa
được phân giải. Decision chỉ được ghi vào ledger sau acknowledgement. Rejection
loại bỏ pending decision mà không thay đổi lịch sử score, traffic hoặc state dự
báo.

### 1.3. Decision Engine

`UdonShieldEngine` điều phối toàn bộ quyết định chiến lược. Engine sở hữu các
thành phần có state xuyên ngày:

- route cache;
- traffic belief;
- response ledger;
- certified contingencies;
- predicted next-agent state.

Mỗi lời gọi giải một ngày tạo một `DecisionResult` bất biến về mặt audit, gồm
candidate được chọn, profile, scenario manifest, viability bounds, deadline
profile và diagnostics của các tầng tìm kiếm.

### 1.4. Planning Layer

Planning Layer gồm:

- `ParetoRouter` cho đường đi đa tài nguyên;
- `RouteColumnGenerator` cho tập cột lộ trình;
- `RouteMaster` cho phối hợp nhiều agent;
- `AdaptiveRouteImprover` cho cải tiến có cấu trúc;
- `blank_slate::Planner` cho nguồn candidate độc lập;
- exact orienteering cho các frontier có thể chứng nhận;
- `ProtectedSlackRefiner` cho cải tiến WAIT bảo toàn transition của incumbent.

Mọi thành phần planning chỉ tạo candidate. Quyền chấp nhận candidate thuộc về
simulator, validator và tầng certification.

### 1.5. Correctness Layer

Correctness Layer gồm hai triển khai độc lập:

- `ExactStepSimulator` thực thi chuyển trạng thái và tạo trace;
- `IndependentDayValidator` kiểm tra cùng day plan bằng đường logic độc lập.

Candidate chỉ được truyền lên tầng selection khi hai thành phần đồng ý. Candidate
cuối được kiểm tra lại với full trace trước khi serialize.

---

## 2. Mô hình domain

### 2.1. MatchConfig

`MatchConfig` là cấu hình bất biến của trận, gồm:

- hình học bản đồ lục giác và adjacency;
- terrain của từng ô;
- tập road và các ngưỡng phân loại traffic;
- tập spot, brand và stock;
- vị trí xuất phát của agent;
- giới hạn fuel;
- số ngày và số step theo ngày;
- số đội và số agent.

Chi phí của một move được xác định từ ô nguồn và road status hiện tại. Vì vậy,
quan hệ kề có thể đối xứng nhưng chi phí tài nguyên của hai chiều được biểu diễn
riêng.

### 2.2. DayState

`DayState` là snapshot có thẩm quyền ở đầu ngày:

- ngày hiện tại;
- trạng thái của agent đội mình;
- trạng thái công khai của đội khác;
- road status;
- deadline của snapshot.

Engine không nối tiếp dự báo nội bộ một cách mù quáng. Nếu agent state hoặc ngày
không khớp dự báo, state nhận được được dùng làm điểm bắt đầu mới và mismatch được
ghi vào audit.

### 2.3. MatchLedger

`MatchLedger` lưu phần score đã được xác nhận:

- bitset brand đã thu trong toàn trận;
- tổng daily distinct;
- tổng servings.

Ledger chỉ thay đổi sau acknowledgement. Candidate score của ngày được tính bằng
cách áp dụng `DayScore` lên ledger hiện tại, không ghi trực tiếp vào ledger trong
quá trình search.

### 2.4. Action và DayPlan

Một `DayPlan` chứa một `AgentPlan` cho mỗi agent. `AgentPlan` là chuỗi:

- `MOVE(direction)`;
- `WAIT(duration)`.

Simulator mở rộng action theo timeline step. Một day plan hợp lệ phải thỏa toàn bộ
luật di chuyển, fuel, duration, pickup, refuel và kết thúc đúng biên ngày.

### 2.5. OfficialScore

Score được biểu diễn bằng bộ ba:

\[
S=(L,D,Q)
\]

trong đó:

- \(L\) là số brand khác nhau của toàn trận;
- \(D\) là tổng số brand khác nhau theo ngày;
- \(Q\) là tổng servings.

Mọi comparator trong solver dùng thứ tự từ điển trên bộ ba này. Các cận, profile,
route master và resubmission gate cùng dùng một quan hệ thứ tự.

### 2.6. Candidate

`MasterCandidate` là một phương án nhiều agent hoàn chỉnh, gồm:

- day plan;
- simulation result;
- score sau ngày hiện tại;
- stable identity;
- nguồn sinh candidate;
- metadata của route bundle.

`CandidateEvaluation` ghép candidate với `CandidateProfile`. Candidate và profile
không được hoán đổi giữa hai scenario manifest khác nhau.

---

## 3. State machine của trận

```text
Configured
    │
    ▼
Roles Selected
    │
    ▼
Awaiting Authoritative Day State
    │
    ▼
Planning
    │
    ▼
Pending Submission
    ├── rejected ───────────────► Awaiting Authoritative Day State
    │
    └── acknowledged
             │
             ▼
       Ledger and Belief Update
             │
             ▼
       Post-ACK Contingency Work
             │
             ▼
       Awaiting Authoritative Day State
```

### 3.1. Khởi tạo

Protocol Host parse `MatchConfig`, tạo `MatchSession`, sau đó yêu cầu engine chọn
role. Role assignment được gửi qua protocol trước ngày đầu và được dùng để prewarm
route cache.

### 3.2. Nhận trạng thái ngày

Session nhận `DayState` và `MatchLedger` có thẩm quyền. Engine reconcile state với
dự báo của decision đã acknowledgement trước đó, cập nhật traffic belief và bắt
đầu pipeline quyết định.

### 3.3. Pending submission

Khi decision được phép gửi, session giữ bản sao pending của:

- decision;
- authoritative state đã dùng để giải;
- ledger đã dùng để tính score.

Ba đối tượng này tạo thành transaction boundary. Chúng được xác nhận cùng nhau sau
acknowledgement hoặc bị loại cùng nhau sau rejection.

### 3.4. Acknowledgement

Sau acknowledgement, engine:

- ghi response vào response ledger;
- ghi candidate và profile được chấp nhận;
- ghi own-road footprint;
- lưu predicted next-agent state;
- trích certified contingency từ future witness;
- cấp phần ngân sách còn lại cho post-ACK work.

---

## 4. Kiến trúc chọn vai trò

Role selection là một pipeline riêng trước ngày đầu nhưng dùng chung primitive với
day solver.

### 4.1. Enumeration

`RoleAssignmentEnumerator` tạo các patrol/tanker assignment. Mỗi assignment có:

- vector role theo agent;
- số patrol;
- cheap upper bound;
- rollout score;
- cờ validity và completeness của rollout.

Cheap bound được dùng để sắp thứ tự và tạo shortlist, không được dùng làm score
cuối.

### 4.2. Probe

Các assignment có cấu trúc ưu tiên được rollout một ngày. All-patrol và assignment
tanker trung tâm được bảo vệ trong beam để cheap bound không loại toàn bộ một lớp
chiến lược trước khi có evidence sâu hơn.

### 4.3. Full-horizon rollout

Beam được rollout qua horizon bằng cùng:

- route generator;
- route master;
- exact simulator;
- independent validator;
- lexicographic score.

Mỗi rollout mang trạng thái agent, traffic và ledger qua từng ngày. Assignment
được so bằng kết quả rollout hợp lệ và trạng thái completeness của evidence.

### 4.4. Incomplete-evidence fallback

Khi không thể hoàn thành toàn bộ full-horizon comparison, selector chuyển sang
fallback cấu trúc dựa trên dữ liệu công khai của trận và các assignment đã có.
Fallback không tạo một score model khác; nó chỉ sắp lại beam để tránh coi partial
rollout như bằng chứng đầy đủ.

---

## 5. Kiến trúc deadline

`DeadlineScheduler` chuyển available time thành `DeadlineProfile`. Profile chia
ngân sách theo trách nhiệm:

- seed incumbent;
- fast viability;
- search;
- certification;
- network reserve.

Deadline được truyền xuống router, column generator, master, ALNS và witness
repairer. Mỗi tầng phải trả về trạng thái completeness riêng; hết deadline không
được biến partial result thành exact result.

Deadline class điều chỉnh độ rộng portfolio, số candidate, operation cap và proof
passes. Nó không thay đổi luật score, simulator semantics hoặc admission gate.

Nếu ngân sách không đủ cho seed, certification và transport reserve, engine chỉ
dùng fallback đã exact-validate.

Protocol Host duy trì ba mốc deadline. Checkpoint deadline là giá trị nhỏ hơn giữa
thời điểm nhận authoritative state cộng `5000 ms` và action deadline trừ transport
safety. Canonical main solve, role selection, certification và complete protected
checkpoint bị chặn bởi mốc này. Action deadline là giới hạn phản hồi có thẩm quyền
do trận công bố. Continuation deadline bằng action deadline trừ transport safety và
chỉ tồn tại khi public window dài hơn checkpoint window.

Public continuation không nới ngân sách hoặc chạy lại main solve. Nó bắt đầu từ
complete checkpoint đã exact-validate, dùng một snapshot của refiner state và chỉ
được thay checkpoint qua admission certificate. Nếu public window không tồn tại,
không đáng tin, đã hết hoặc không tạo strict gain, checkpoint là response cuối.

---

## 6. Pipeline quyết định theo ngày

`UdonShieldEngine::solve_day` thực hiện một chuỗi pha cố định. Các pha sau có thể
thu hẹp theo deadline nhưng không đổi thứ tự trách nhiệm.

### 6.1. Reconciliation và incumbent

Engine chuẩn hóa available time, đối chiếu authoritative state với predicted
state, cập nhật traffic belief và phân loại deadline. Exact wait plan được dựng và
kiểm tra trước mọi search. Greedy planner có thể thay exact wait bằng một incumbent
tốt hơn sau khi plan mới qua master, simulator và validator.

### 6.2. Manifest và viability

Scenario manifest được đóng băng cho toàn decision. Viability analyzer tạo
reservation, lower/upper bounds và frontier. Certified contingencies từ ngày trước
được đối chiếu với state mới; contingency repair thành công được đưa vào candidate
pool, còn contingency không tương thích bị loại.

### 6.3. Cấu hình capability

Engine dựng `ColumnGenerationOptions` từ state, horizon và deadline. Các capability
harvest, exact orienteering, fuel-constrained route, terminal route và future
extension được mở trong cùng generator. Incumbent và contingency hợp lệ được thêm
làm seed để search mới không làm mất phương án đã bảo vệ.

### 6.4. Candidate generation

Candidate được tạo từ hai luồng:

- route portfolio được giải bởi `RouteMaster`;
- day plan độc lập từ `blank_slate::Planner`, sau đó được
  `RouteMaster::evaluate_exact_plan` chuyển thành `MasterCandidate`.

Hai luồng hợp nhất bằng stable identity. Candidate độc lập không có đường admission
riêng.

### 6.5. Feedback và adaptive improvement

Candidate đầu tiên cung cấp critical-road evidence cho một lượt promoted portfolio.
Route augmentation đưa các route mới từ candidate trở lại portfolio. Master,
recombination và `AdaptiveRouteImprover` tiếp tục mở các neighborhood trong cùng
search boundary.

### 6.6. Candidate preparation

Các candidate được exact-evaluate, loại duplicate và xếp thứ tự theo current-day
score cùng upper evidence. Engine giữ incumbent, candidate tốt nhất hiện tại và
các challenger còn khả năng thay đổi tier quyết định.

### 6.7. Provisional profiling (F0)

Future Witness Repairer tạo profile provisional bằng operation-bounded rollout.
Provisional comparator chỉ xác định thứ tự cấp ngân sách certification; nó không
quyết định submission.

### 6.8. Witness certification (W1)

Certification tập trung vào floor leader, incumbent hoặc last-sent candidate và
challenger có valid-upper upside. Future suffix của từng candidate được repair trên
mọi scenario bắt buộc. Candidate còn outcome chưa certified bị loại khỏi final
pool.

### 6.9. Final selection

Profile được finalize cùng nhau, certified-dominated candidate bị loại và
lexicographic risk comparator chọn trong tập còn lại. Candidate được chọn chạy lại
full-trace simulator và independent validator. Engine sau đó hoàn tất optimality
gap, audit và timing của decision.

### 6.10. Protected checkpoint refinement

Sau final selection, Protocol Host mở một chuỗi neighborhood bị ràng buộc bởi
transition của incumbent để tạo complete checkpoint. Trên ngày không phải ngày
cuối, chuỗi refinement có ba tầng tuần tự:

1. wait-detour thay một đoạn `WAIT` của patrol bằng round trip qua spot rồi quay
   lại anchor trong cùng số step;
2. mid-day coordinate ascent thay toàn bộ day route của từng patrol bằng route
   trong sparse global pool, chấp nhận từng strict improvement rồi lặp tới fixed
   point;
3. sau fixed point đó, target-terminal sparse search dựng pool riêng theo terminal
   đang được incumbent bảo vệ của từng patrol, kể cả terminal không phải spot, rồi
   chạy lại cùng coordinate ascent trên phần thời gian còn lại.

Target-terminal search dùng cùng resource labels, Pareto dominance, route rank,
state cap và deadline với sparse global search. Khác biệt duy nhất là điều kiện
emit: label chỉ được giữ tại terminal do caller cung cấp thay vì chỉ tại một spot.
Global pool và toàn bộ ascent của nó luôn hoàn tất trước; target-terminal pool là
suffix cộng thêm và không thể làm mất incumbent của tầng trước.

Ở ngày cuối, terminal sparse refinement thay route của từng patrol bằng route
sparse và lặp one-agent ascent tới fixed point. Vì không còn trạng thái ngày sau,
terminal cell, remaining fuel và road footprint không thuộc certificate cuối
trận. Phần thời gian còn lại được dùng cho pair exchange: hai patrol được thay
đồng thời bằng hai route từ pool đã exact-evaluate; một pair được chấp nhận xong
thì one-agent ascent chạy lại trước vòng pair tiếp theo.

Trên ngày không cuối, candidate chỉ thay incumbent khi simulator và validator đồng
ý, thứ tự kind và terminal cell của mọi agent giữ nguyên, fuel cuối của mỗi patrol
không giảm, road footprint bằng nhau, lifetime brand là superset, cumulative daily
distinct và servings không giảm, đồng thời daily distinct hoặc servings tăng
nghiêm ngặt. Trên ngày cuối, admission yêu cầu exact validity và official
lexicographic strict gain. Nếu deadline, validity hoặc certificate tương ứng không
đạt, incumbent được giữ nguyên byte-for-byte.

Khi protected candidate được acknowledgement, runtime duy trì state và ledger của
virtual parent. Decision Engine tiếp tục giải từ virtual parent để trajectory của
bounded search không bị thay đổi bởi state giàu hơn. Plan đó được exact-simulate và
independent-validate lại trên authoritative state trước khi gửi. Nếu authoritative
state không còn forward-simulate virtual parent, virtual state bị loại và
authoritative state trở lại làm đầu vào duy nhất.

### 6.11. Closed-loop public continuation

Protocol Host duy trì đồng thời ba nhánh sau mỗi acknowledgement:

- authoritative branch phản ánh state và ledger thực tế do server xác nhận;
- checkpoint branch phản ánh complete protected checkpoint tại mốc `5000 ms`;
- virtual-parent branch phản ánh output của canonical Decision Engine trước các
  protected refinement.

Canonical Decision Engine chỉ chạy một lần trên virtual-parent branch và phải hoàn
tất trước checkpoint deadline. Protected refinement trong checkpoint window tạo
checkpoint plan. Trước continuation, checkpoint plan được phát lại bằng exact
simulator trên authoritative branch để tạo richer-state checkpoint; bản phát lại
này phải giữ transition và ledger dominance đối với checkpoint branch trên ngày
không cuối. Snapshot của refiner sau đó tiếp tục mở các neighborhood còn dở tới
continuation deadline; main search, role selection và certification không được
khởi động lại.

Trên ngày không cuối, continuation chỉ được tiếp quản khi exact simulator và
independent validator xác nhận strict official-score gain cùng transition/ledger
dominance so với richer-state checkpoint. Trên ngày cuối, điều kiện tiếp quản là
exact validity và strict official lexicographic gain vì không còn transition ngày
sau cần bảo vệ. Nếu không có public window có thẩm quyền, hết deadline, không có
strict gain hoặc certificate không đạt, serialized response được lấy nguyên từ
checkpoint. Chỉ plan thắng certificate cuối cùng mới được gửi; runtime không gửi
checkpoint trước rồi nộp đè.

---

## 7. Kiến trúc traffic belief

### 7.1. Phân tách nguồn traffic

Traffic của một road được phân thành:

- own footprint đã biết từ các plan được acknowledgement;
- carry interval của đối thủ suy ra từ public road status;
- footprint ngày hiện tại của đối thủ trong từng scenario.

Own footprint là deterministic. Phần đối thủ được biểu diễn bằng interval và
scenario, không bằng một dự báo điểm duy nhất.

### 7.2. Lịch sử

`TrafficBelief` duy trì:

- own footprint history;
- previous own footprint;
- public endpoint history của đội mình;
- public opponent history;
- opponent carry interval;
- ngày quan sát và ngày submission gần nhất.

Khi chuỗi ngày hoặc acknowledgement không liên tục, belief hủy phần history không
còn đủ điều kiện để suy luận carry.

### 7.3. Scenario manifest

`ScenarioGenerator` đóng băng một `ScenarioManifest` trước khi so candidate.
Manifest chứa:

- likely scenario;
- public adversarial scenarios;
- pessimistic fallback khi tập scenario khả thi không bao phủ đầy đủ;
- scenario weights và construction metadata;
- evaluator và risk metadata.

Likely scenario được dựng từ public endpoints và đường đi khả thi. Adversarial
scenario tập trung vào road có khả năng đổi trạng thái, road separator và corridor
có dấu hiệu lặp. Việc phân bổ dwell của đối thủ giữ ràng buộc khả thi theo từng
agent, tránh cộng độc lập các footprint không thể đồng thời xảy ra.

Private sensitivity scenarios chỉ cung cấp diagnostics cho critical-road
promotion. Chúng không được thêm vào manifest dùng để chọn candidate.

### 7.4. Chuyển traffic sang ngày sau

Với mỗi scenario và own current footprint, `predict_next_road_statuses` kết hợp:

- previous own footprint;
- own footprint của candidate;
- opponent carry;
- opponent current footprint.

Kết quả được dùng để mô phỏng future witness. Khi authoritative state ngày sau
đến, prediction bị thay thế bởi road status thực.

---

## 8. Kiến trúc viability

`FastViabilityAnalyzer` tạo cận về khả năng bảo toàn score toàn horizon trước khi
route search mở rộng.

### 8.1. Brand feasibility

Analyzer tính cho từng brand:

- ngày khả thi muộn nhất;
- ngày an toàn muộn nhất;
- slack theo horizon;
- số agent-day slot khả dụng;
- matching feasibility giữa agent-day và brand.

Các brand có slack thấp tạo `MandatoryReservation`. Reservation được truyền vào
column generator, master và adaptive improvement.

### 8.2. Viability bounds

`ViabilityBounds` gồm:

- lower bound;
- upper bound;
- pessimistic upper bound;
- coverage cap;
- safe coverage;
- conditional tier bounds;
- viability frontier;
- deadline/completeness state.

Lower bound chỉ được gắn với một witness hợp lệ. Upper bound chỉ có quyền prune
khi envelope và quan hệ dominance tương ứng được chứng minh.

### 8.3. Frontier

Mỗi `ViabilityFrontierPoint` liên kết:

- mức coverage;
- risk rank;
- lower và upper score;
- future witness;
- scenario identity.

Frontier giữ các điểm không bị trội đồng thời về bound và risk. Nó bảo toàn các
trade-off khác nhau thay vì ép mọi candidate vào một scalar objective.

---

## 9. Kiến trúc routing đa tài nguyên

### 9.1. Pareto labels

`ParetoRouter` tìm đường trên trạng thái tài nguyên. Một label biểu diễn:

- cell hiện tại;
- elapsed steps;
- fuel đã dùng;
- critical-road footprint;
- predecessor để tái tạo route.

Hai label chỉ dominance khi một label không tệ hơn ở mọi tài nguyên liên quan.
Nhờ vậy router giữ được đường nhanh nhưng tốn fuel, đường chậm nhưng tiết kiệm fuel
và đường tránh traffic như các phương án riêng.

### 9.2. Resource lower bounds

Router duy trì lower bounds tới target cho step và fuel. Lower bounds được dùng để:

- loại label không thể tới target trong day boundary;
- xác định khả năng quay về terminal hoặc rendezvous;
- giới hạn exact orienteering;
- tính cheap upper bound cho column extension.

### 9.3. Route cache

Cache được khóa bởi source, target, road state và resource context. Cache chỉ lưu
kết quả routing; nó không lưu candidate score hoặc selection outcome. Khi context
không tương đương, query không được dùng lại.

---

## 10. Kiến trúc route column

`RouteColumnGenerator` chuyển các path và seed plan thành `RouteColumn` hoàn chỉnh.

### 10.1. Nội dung một column

Một column mang cả quyết định và chứng cứ phối hợp:

- agent identity và role;
- action sequence;
- exact timeline;
- terminal cell và fuel;
- spot visits và first-claim events;
- score contribution;
- road footprint;
- refuel demand và refuel provision;
- escort segments;
- bundle identity;
- terminal và overnight features.

Column không chỉ là danh sách spot. Mọi timing cần cho stock, refuel và
synchronization được materialize trước khi vào master.

### 10.2. Nguồn sinh column

Portfolio hợp nhất các nguồn:

- direct Pareto routes;
- greedy seed;
- incumbent plan;
- cached contingencies;
- harvest extensions;
- exact hoặc anytime orienteering frontier;
- refuel/rendezvous routes;
- terminal variants;
- critical-road promoted routes.

Các capability fuel khác nhau dùng chung generator. Chúng chỉ thay đổi frontier
được phép mở theo state, horizon và deadline, không tạo pipeline chọn candidate
riêng.

### 10.3. Harvest và terminal semantics

Harvest extension chèn visit chỉ khi exact timeline còn hợp lệ. Overnight harvest
được biểu diễn bằng terminal position của ngày hiện tại và action thật ở đầu ngày
sau. Terminal refuel được biểu diễn như state transition giữa hai ngày; fuel nhận
được không được sử dụng trong prefix trước transition.

### 10.4. Portfolio pruning

Pruning bảo toàn:

- incumbent column;
- fallback column;
- complete escort/refuel bundle;
- mandatory reservation coverage;
- terminal diversity;
- score và resource nondominance.

Một member của bundle không được giữ riêng nếu phần còn lại là điều kiện để column
hợp lệ.

---

## 11. Kiến trúc RouteMaster

`RouteMaster` chọn một tổ hợp column tạo thành team plan.

### 11.1. Biến quyết định

Master chọn một column cho mỗi agent. Tổ hợp được mở rộng theo thứ tự bound và được
đánh giá như một partial assignment.

### 11.2. Ràng buộc

Master kiểm tra:

- đủ đúng một plan cho mỗi agent;
- action duration hợp lệ;
- stock capacity theo spot và thời điểm;
- duplicate claim semantics;
- refuel demand có provision tương ứng;
- patrol và tanker đồng bộ tại rendezvous;
- escort bundle đầy đủ;
- terminal compatibility;
- mandatory brand reservation;
- road and traffic context nhất quán.

### 11.3. Bound và cut

Search sử dụng:

- per-agent score upper bounds;
- exact stock credit;
- capacity cuts;
- prefix cuts;
- bundle-aware upper bounds;
- synchronization feasibility;
- incumbent lexicographic bound.

Bound chỉ thay đổi thứ tự và phạm vi search. Một tổ hợp hoàn chỉnh vẫn phải exact
simulate và independent validate trước khi trở thành `MasterCandidate`.

### 11.4. Diagnostics

Master xuất diagnostics cho:

- số node và combination đã xét;
- nguyên nhân prune;
- bound/cut đã kích hoạt;
- bundle và synchronization rejection;
- deadline reached;
- search completeness.

Diagnostics không tham gia comparator.

---

## 12. Kiến trúc cải tiến thích nghi

`AdaptiveRouteImprover` nhận portfolio và candidate từ master, sau đó tạo
neighborhood có ngữ nghĩa domain.

### 12.1. Operator

- `RareBrandRescue`: khôi phục brand có viability slack thấp;
- `OvernightHarvest`: thay đổi terminal để tạo claim qua biên ngày;
- `DockOrRendezvous`: thay đổi điểm và timing refuel;
- `MergeSplit`: gộp hoặc tách nhiệm vụ giữa agent;
- `StockMultiVisit`: phối hợp nhiều visit trên cùng stock process;
- `CriticalRoadBypass`: sinh route tránh road nhạy cảm;
- `TerminalShift`: thay terminal state;
- `ViabilityRepair`: sửa candidate vi phạm reservation hoặc future feasibility.

### 12.2. Candidate admission

Mỗi operator sinh route hoặc bundle mới, đưa chúng qua master và correctness layer.
Operator không chỉnh trực tiếp score profile và không thể thay incumbent bằng một
partial plan.

### 12.3. Proof-guided iteration

Viability gap, critical-road diagnostics và candidate upper bounds được dùng để
chọn operator cho vòng tiếp theo. Feedback chỉ hướng dẫn nơi mở neighborhood; nó
không thay đổi comparator hoặc validity semantics.

---

## 13. Kiến trúc future witness

### 13.1. Provisional profile

`FutureWitnessRepairer` dựng profile ban đầu cho từng current-day candidate trên
cùng scenario manifest. Với mỗi scenario, nó:

- áp dụng current candidate;
- dự báo road state tiếp theo;
- rollout phần horizon còn lại;
- tạo future plan sequence;
- tạo lower và upper envelope.

Profile provisional được dùng để xếp hàng certification. Nó không có quyền trở
thành submission.

### 13.2. Witness repair

Witness repair mở lại future suffix của các candidate có khả năng ảnh hưởng kết
quả chọn. Mỗi ngày trong suffix được giải bằng cùng planner, simulator và validator.
Một witness chỉ được đánh dấu certified khi toàn bộ suffix hợp lệ và score khớp.

### 13.3. Cached contingency

Sau acknowledgement, first-day plan của certified future witness được tách thành
contingency cho ngày sau. Contingency được khóa bởi day và scenario identity. Khi
authoritative road state mới đến, contingency được repair hoặc loại; nó không được
coi là hợp lệ theo một scenario khác.

### 13.4. Strong proof

Strong proof search cố đóng lower/upper gap của phần horizon còn lại. Proof record
phân biệt:

- completed proof;
- incomplete proof;
- certified lower bound;
- valid upper bound.

Incomplete proof không được dùng như chứng minh tối ưu.

---

## 14. Kiến trúc profile và selection

### 14.1. CandidateProfile

Profile chứa:

- `ScenarioOutcome` theo từng scenario;
- future witness;
- coverage cap, safe coverage và confidence coverage;
- score quantiles;
- survival signature;
- certified lower bound;
- provisional lower bound;
- valid upper bound;
- per-scenario upper bounds;
- conditional tier bounds;
- viability frontier.

Scenario weights và thứ tự scenario là một phần của profile identity.

### 14.2. Profile finalization

`LexicographicRiskComparator` finalize toàn bộ profile cùng lúc. Tầng này:

- tính quantile trên cùng weighted support;
- dựng survival signature cho các profile đồng quantile;
- dựng conditional bounds theo coverage;
- loại các frontier point bị trội;
- kiểm tra envelope consistency.

### 14.3. Certified dominance

Candidate A certified-dominate candidate B khi certified outcome distribution của
A không kém valid upper envelope của B trên toàn weighted support và tốt hơn nghiêm
ngặt ở ít nhất một ngưỡng. So sánh chỉ hợp lệ khi hai profile có cùng scenario
weights và B có valid upper bounds đầy đủ.

### 14.4. Final selection

Selection thực hiện theo thứ tự:

1. giữ các profile qua confidence gate;
2. nếu được yêu cầu, bảo toàn current-day score floor;
3. loại candidate bị certified dominance;
4. so các candidate còn lại bằng risk-aware lexicographic comparator;
5. yêu cầu mọi scenario outcome có certified witness;
6. chạy full-trace simulator và independent validator lần cuối.

Tập selection luôn chứa exact fallback. Vì vậy confidence gate và dominance prune
không được làm tập candidate rỗng.

### 14.5. Resubmission

Một decision mới trong cùng ngày chỉ được thay submission trước khi:

- current-day score cải thiện theo thứ tự từ điển;
- mọi tier trước tier khác đầu tiên được bảo toàn;
- certified lower bound mới vượt valid upper bound cũ tại tier quyết định;
- hoặc profile mới certified-dominate profile cũ.

Stable identity ngăn gửi lại cùng một candidate.

---

## 15. Kiến trúc correctness

### 15.1. ExactStepSimulator

Simulator thực thi đồng bộ tất cả agent theo timeline và chịu trách nhiệm:

- kiểm action syntax;
- tính duration từ terrain và road status;
- kiểm destination;
- trừ fuel;
- xử lý wait;
- xác định claim event;
- áp stock semantics;
- xử lý refuel;
- tạo final agents;
- tạo road footprint;
- tạo `DayScore` và `StepTrace`.

Simulation trả error code có agent và step liên quan khi plan không hợp lệ.

### 15.2. IndependentDayValidator

Validator tái kiểm tra plan bằng triển khai độc lập và trả một `SimulationResult`
có cùng contract. `agrees_with` đối chiếu validity, final state, score, footprint,
claim và trace cần thiết.

### 15.3. Fallback

Fallback là team plan chỉ gồm wait actions lấp đầy ngày. Nó được dựng từ
authoritative state và được kiểm qua cả simulator lẫn validator. Fallback tham gia
pipeline như incumbent được bảo vệ, không phải exception bỏ qua logic.

### 15.4. State reconciliation

Predicted next agents chỉ là cache optimization. Khi authoritative state khác dự
báo, engine:

- dùng authoritative agents;
- xóa predicted next-agent state không còn khớp;
- giữ ledger đã acknowledgement;
- cập nhật audit về reconciliation.

---

## 16. Kiến trúc submission và replay

### 16.1. Serialization boundary

Sau final validation, candidate được chuyển thành canonical wire actions. Body đã
serialize là bất biến cho toàn bộ vòng retry của submission đó.

### 16.2. Acknowledgement boundary

Acknowledgement phải khớp transaction đang pending. Chỉ acknowledgement hợp lệ
mới gọi `record_submitted`. Transport failure, stale response hoặc rejection không
được cập nhật ledger.

### 16.3. Replay model

Replay là event log append-only. Các event liên kết:

- setup;
- role assignment;
- authoritative state;
- decision replay;
- protected-slack diagnostics;
- checkpoint actions khi closed-loop continuation được kích hoạt;
- serialized actions;
- action response;
- final result.

Resume reconstruct authoritative, checkpoint và virtual-parent branch từ chuỗi
event đã acknowledgement. `checkpoint_actions` gắn complete checkpoint với đúng
day state; `actions` gắn plan thực tế đã gửi. Với replay cũ không có checkpoint
event, checkpoint branch được đồng nhất với authoritative branch. Pending action
không có acknowledgement không được coi là state đã xác nhận.

### 16.4. Decision audit

Decision replay chứa:

- input state và ledger;
- scenario manifest;
- viability bounds;
- deadline profile;
- candidate profiles;
- route/master/improver diagnostics;
- certification state;
- selection disposition;
- simulator và validator result;
- timing theo pipeline stage.

Audit là dữ liệu quan sát của pipeline, không phải input cho score comparator trong
chính decision đã tạo nó.

---

## 17. Quan hệ phụ thuộc và bất biến kiến trúc

### 17.1. Hướng phụ thuộc

```text
types
 ├── protocol
 ├── simulator
 ├── validator
 └── graph
       └── planner
             └── decision
                   └── runtime
                         └── protocol host
```

Strategy modules phụ thuộc vào domain/planning contracts. Protocol Host phụ thuộc
vào runtime nhưng runtime không phụ thuộc vào HTTP transport.

### 17.2. Bất biến state

- `MatchConfig` không đổi trong một session.
- Ledger chỉ phản ánh decision đã acknowledgement.
- Traffic belief chỉ ghi own footprint của decision đã acknowledgement.
- Không tồn tại hai pending submissions trong một session.
- Candidate profile chỉ có nghĩa trên scenario manifest đã tạo nó.
- Cached contingency chỉ được dùng sau khi đối chiếu authoritative state.
- Virtual parent chỉ tồn tại sau acknowledgement của một protected improvement.
- Checkpoint branch chỉ phân kỳ sau acknowledgement của một plan khác complete
  checkpoint và phải có `checkpoint_actions` tương ứng trong replay.
- Authoritative state phải forward-simulate virtual parent trước mỗi decision dùng
  virtual state.
- Authoritative state phải dominance checkpoint branch trước khi closed-loop
  continuation dùng checkpoint đó làm incumbent.

### 17.3. Bất biến planning

- Mọi team plan có đúng một agent plan cho mỗi agent.
- Mọi candidate hoàn chỉnh đều qua exact simulator và independent validator.
- Upper bound không được dùng như certified witness.
- Partial search không được ghi là complete.
- Incumbent exact-valid không bị loại bởi heuristic estimate.
- Bundle phụ thuộc synchronization được giữ hoặc loại như một đơn vị.
- Các capability theo fuel, horizon và deadline dùng chung domain semantics và
  comparator.

### 17.4. Bất biến selection

- Score được so theo một thứ tự từ điển thống nhất.
- Các candidate chỉ được so xác suất trên cùng frozen scenario manifest.
- Candidate được chọn có certified witness cho mọi scenario bắt buộc.
- Certified dominance yêu cầu envelope tương thích.
- Final selection luôn được exact-validate lại.
- Protected refinement chỉ thay incumbent khi có strict score gain và, trên ngày
  không cuối, componentwise transition/ledger dominance. Ngày cuối dùng exact
  validity và official lexicographic strict gain.

### 17.5. Bất biến transport

- Submission retry không thay body.
- Acknowledgement phải khớp pending day.
- Rejection không advance state.
- Resume chỉ dùng event đã được acknowledgement xác nhận.
- Outer action deadline dài hơn không được nới canonical main solve, role
  selection, certification hoặc complete checkpoint quá `5000 ms`; chỉ protected
  continuation đã được promote mới dùng suffix tới action deadline trừ transport
  safety.

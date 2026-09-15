# 368: đã xác định đường dẫn gây chênh lệch, chưa chứng minh lỗi correctness

2026-09-10. Chỉ đọc hai trận thua **active** trong protected366-P2 đã hoàn tất.
Không chạy solver, không tạo trận, không sửa source/budget, không mở holdout,
không thay metric và không giảm/xóa chức năng. Không có sản phẩm mới để commit.

## Kết luận điều tra

1. Loss460 (-33 servings) bắt đầu từ **chọn role trước khi gọi treatment**.
   Không thể dùng tổng -33 làm ước lượng riêng tác hại resource suffix.
2. Loss641 (-2 daily, +2 servings) phát sinh sau **main ngày5 chọn khác plan**
   với cùng state/ledger/manifest/deadline. Pool và độ sâu F0 khác giữa hai lượt;
   tới ngày9 vị trí patrol của candidate không phủ brand0/5, chỉ nhận4 loại so
   với6. Đây là nguồn chính xác của daily-2 trong log.
3. Không phát hiện comparator đảo score hoặc resource certificate sai. Chưa
   biết chính xác sự kiện scheduler/cache/cutoff lịch sử nào tạo khác biệt
   công việc: log không ghi đủ để suy ra điều đó. Không gọi mọi downside là
   vô hại, cũng không gán nó chắc chắn cho resource treatment.

**Không có hotfix nhỏ, semantics-equivalent đã được chứng minh cần áp dụng.**
Đổi admission, chia lại thời gian F0 hoặc bỏ chức năng để làm hai case hòa sẽ
đổi score policy và còn có nguy cơ mất các thắng hiện có. Hai case là consumed
protected evidence, không được dùng để tune successor.

## Loss202609093860460: khác ngay ở vai trò

32x32,10 đội,native,low fuel,high-stock,10 ngày,public10000ms.
Parent6/60/538; candidate6/60/505. Role parent `[0,0,0,0,0,1,0,1]`, candidate
`[0,0,0,0,1,1,0,0]`. Lần resource gain đầu tiên ngày3; state/main plan đã khác
ngày1, ledger khác từ ngày2. Biên đọc treatment của binary360 nằm **sau** main
và public-prefix, không thể sửa một role đã chọn trước đó.

Serving delta theo ngày: `+5,+7,-3,-5,+4,+10,-11,-12,-6,-22`, tổng-33.
Không có bằng chứng về một move đơn lẻ làm mất33, hoặc treatment làm đổi role.
Chênh lệch là kết quả của hai quỹ đạo đã khác trước treatment, không phải một
counterfactual giữ nguyên toàn bộ main execution.

## Loss202609093860641: từ main ngày5 đến coverage ngày9

32x32,8 đội,fixed-all-Patrol,high fuel,roadless overnight,10 ngày,10000ms.
Parent6/58/363; candidate6/56/365.

Ngày1--4: main state,ledger,manifest và main plan giống nhau. **Mỗi pre-resource
plan của candidate đúng bằng plan parent đã gửi trong ngày tương ứng**. Resource
chỉ thêm servings `+2,+1,+1,+1`; certificates đều giữ terminal/fuel/ledger dominance.

Ngày5: main input vẫn giống nhau, nhưng **post-F0 audit có16 candidate mỗi bên,
chỉ10 identity chung và6 identity riêng mỗi bên**. Đây là post-admission audit,
không cho biết từng plan riêng chưa được sinh hay đã bị prune trước audit.
Hai plan được chọn sau cùng đều tồn tại trong cả hai pool:

| Plan identity prefix | F0 trong parent | F0 trong candidate | W1 trong candidate |
|---|---|---|---|
| C09E6BCFE317 (parent chọn) | 6/42/300 | 6/42/300 | 6/56/336 |
| 46BFADD2A2AD (candidate chọn) | 6/30/240, không shortlist | 6/42/301 | 6/56/337 |

Cả hai plan có current score6/30/240. Trong parent,46BF đứng index11 của audit
và chỉ còn current-only F0; trong candidate nó đứng index9, có witness tương lai
tốt hơn và được shortlist. Khi cả hai đã được W1 chứng nhận trong candidate,
46BF thắng đúng comparator vì6/56/337 > 6/56/336. Không phải final comparator
loại một nghiệm certified tốt hơn đang eligible.

Main wall-clock work cũng khác: master10580 so với10574 combinations, beam500
so với509, DFS10080 so với10065; cả hai `deadlineReached=true`, `searchComplete=false`.
Independent rollouts538/536; evaluations521/519. Main total2313/2304ms,
candidate preparation311/308ms, certification52/44ms. Không suy luận rằng
chính3ms preparation là nguyên nhân duy nhất: pool, thứ tự và công việc đã khác.

Nguồn `src/decision.cpp:4344--4362` tạo các deadline F0/master; `5038--5093`
sort theo stableId rồi profile tuần tự với chung absolute F0 deadline;
`5122--5146` chọn floor/upside/incumbent cho W1. `provisional_profile` ở2064+
giữ witness hiện có khi hết thời gian. Đây là đường phụ thuộc thời gian đã
được code và telemetry xác nhận, không phải một phỏng đoán rằng comparator đảo dấu.

Ngày5 terminal agent0/4/5 đổi. Parent terminal cells
`[790,80,247,24,235,352,247,479]`; candidate
`[1011,80,247,24,790,193,247,479]`. Main state bắt đầu khác ngày6, main ledger
khác ngày7. Không lấy final replay để tuyên bố witness W1 cũ là dự đoán chắc
chắn cho toàn bộ cách solver sẽ replan về sau.

Ngày9, main plan bằng submitted plan ở mỗi bên. Parent nhận các brand
`0,1,2,3,4,5`: agent0 ở732 và agent5 ở627 giữ brand0; agent2 ở167 và agent3 ở339
giữ brand5; agent4 đi từ139 qua204 tới235, lấy thêm brand1. Candidate gửi
WAIT-only ở `[204,8,109,235,577,574,139,109]`, nhận đúng `1,2,3,4`, thiếu0 và5.
Main claims ghi10 servings/6brands so với8servings/4brands. Cộng các delta ngày
khớp exact final `0/-2/+2`; toàn bộ daily loss nằm ở ngày9.

## Đã kiểm cả hậu ACK, không chỉ so state bề mặt

Checkpoint serialized sau ngày1--4 có count khác (parent16/17/18/18 so với
candidate11/14/14/14). **Count này không phải số lần gọi background**: host chỉ
ghi khi serialized body thay đổi. Tuy nhiên ở cuối từng ngày1--4:

- `cachedContingencies` bằng toàn bộ object, gồm cached certified suffix.
- Strong-proof identity,score,bounds,complete/infeasible bằng nhau; chỉ khác
  hai counter `combinationsVisited` và `branchesPruned`.
- Hai counter không được main đọc để xếp hạng. Strong-proof records được dùng
  trong background proof; không có consumer chính thức biến counter thành score.

`MatchSession::acknowledge_submitted` lưu acknowledged **main decision**;
`UdonShieldEngine::record_submitted` đặt background budget bằng5000 trừ main
compute, không lấy tổng thời gian response để chia lại main ngày sau.
`totalResponse` chỉ được tích lũy/báo cáo. Frozen host360 ACK/update giữ virtual
main ledger và checkpoint ledger tách khỏi actual submitted ledger như thiết kế.

Điều này loại được giả thuyết cụ thể "resource làm đổi cached witness hoặc
feedback response-time thành budget main ngày5" ở trace này. Nó **không** ghi
lại mọi internal router cache, allocator, CPU state hoặc lịch OS. Resource chạy
trước ACK vẫn có thể đổi lượng thời gian idle còn lại và trạng thái thực thi.
Không có counterfactual cùng lịch clock nên chưa nhận diện được tác động timing
gián tiếp riêng của resource, khác với xác định boundary và loss arithmetic.

## Phạm vi kiểm tra và quyết định

20 decision singleton của641:0 strict eligible certified-rank inversion.
20 decision nhiều scenario của460 không đủ full candidate distributions để
tái dựng mọi comparator/tie-break; checker abstain, không tuyên bố chứng minh rộng.
8 synthetic analyzer tests pass, gồm fail-closed cho thiếu delta/sai mask,
không bỏ qua cache/proof completion thay đổi, không tính claim denied thành serving.
Full1620-file completion inventory được rehash trước phân tích.

Đóng368: **timed main-admission/trajectory divergence located; no proven
correctness hotfix; exact historical scheduling cause unresolved**. Không mở
thêm vòng minh oan5s, không chạy lại matrix để săn một lượt đẹp, không mở2xe
khi362/366 chưa thành parent. Dữ liệu không cho phép nói "suffix chắc chắn gây
hai thua", nhưng cũng không tự sửa frozen366-P2 thành pass: gate active được
đăng ký vẫn tính mọi active loss, kể cả loss bắt đầu trước treatment.

366/362 giữ nguyên dưới dạng research candidate, không bị xóa; production vẫn
258. Muốn sửa admission phải là cơ chế tổng quát mới và evidence prospective,
không phải hotfix trên hai protected case này. Muốn bỏ active execution losses
khỏi quyết định366-P2 là **thay đổi acceptance contract sau quan sát**, không
phải kết quả tự động của forensic; cần quyết định riêng, minh bạch.

## Exact provenance

- P2 summary `B401C7698036FFFF557B0520055D016AB328D3C7C5F83B57BE4CBD8233D97CB4`.
- P2 completion `B3939DC611F695D86168BD1D6EA274592DF851A4BE9537DA66FB4E98B08A6C85`.
- Inventory368 `9040D953220C58BD597B8EA89CF89D447219753C7764BEB48A9FEF89726F69D6`.
- Explanation368 `46652AD1F9EEBA99A777609ACB2E80C6B4030B79C6ACD2E274BF77609526D939`.
- Source hashes, full selected membership, checkpoints, daily claims và work
  nằm trong hai JSON cùng prefix. `src/decision.cpp` khớp normalized frozen357;
  toàn bộ accepted slack-refiner prefix không đổi trước method resource bổ sung.
- HEAD `c76a8eaa4f200e3eeeb1a58ef1d1fb3d0c13579c`.
- Canonical BTC `F97F168FE76FEF1B6226D2C2CFDAF954F116D39F427B6F157594151C461E1275`.

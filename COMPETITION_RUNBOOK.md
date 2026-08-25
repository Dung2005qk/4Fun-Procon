cd 'C:\Users\LMC\Desktop\4Fun'

$env:HEXUDON_TOKEN = 'bot-16e63de59d414edd0147e7f1'
.\build-release\udonshield_btc.exe http --match m-4196 --response-ms 5000 --replay artifacts/btc/m-4196.jsonl


Remove-Item Env:HEXUDON_TOKEN

## QUY TẮC VẬN HÀNH BẮT BUỘC (từ ATTR-COVERAGE-REGIME-208, 2026-08-23)

1. **TUYỆT ĐỐI không chạy bất kỳ compute nào khác trên máy thi đấu trong lúc
   trận đang diễn ra** (experiment harness, build, VM tooling, browser nặng).
   Bằng chứng đo được: tranh chấp CPU toàn phần làm mất -43 tier-2 daily và
   -185 servings trên một trận 10 ngày (32/312/868 -> 32/269/683), tái tạo
   được theo ý muốn; đây chính là nguyên nhân chính của các trận benchmark
   "sập" (m-3986 rank 4 chơi đúng lúc 207-dev đang chạy local).
2. Sau trận sạch đầu tiên trên máy thi đấu: quét replay
   `columnGeneration.agentParetoQueries` — nếu có patrol 0-query trên máy
   sạch, mở lại trục generation-rebalancing theo điều kiện reopen của 208.
3. Đồng hồ hệ thống: đã chỉnh tay về đúng lúc 23:19 2026-08-23
   (`Set-Date -Adjust -0.37s`, offset đo bằng
   `w32tm /stripchart /computer:time.windows.com`). Service w32time HỎNG
   trên máy này ("no time data was available" kể cả sau unregister/register
   dù UDP 123 thông) — không tự đồng bộ được; CMOS trôi ~vài giây/tháng.
   TRƯỚC TRẬN THẬT: đo lại offset bằng stripchart, nếu lệch >0.2s thì
   `Set-Date -Adjust` lần nữa; đồng thời calibrate bằng timestamp server
   BTC (recipe 055) — sai số này nằm trong dự trữ 1100ms của 197.
4. `--response-ms`: đặt đúng bằng tham số response time của trận (hiện các
   trận practice tạo ở 5000). Cơ chế an toàn hai chiều (161/163/166, xác
   minh 2026-08-24 tại btc_main.cpp:2043 solveDeadline =
   min(receivedAt+clamp(flag,5000), server endsAt)):
   - BTC cấp ÍT hơn flag: server `endsAt` tự thắt solve deadline — an toàn,
     nhưng vẫn nên truyền đúng giá trị thật (phòng khi frame thiếu endsAt
     và tránh phụ thuộc đồng hồ máy vốn đã hỏng w32time).
   - BTC cấp NHIỀU hơn 5000: engine hard-cap 5000 tại
     `competition_compute_budget` (types.hpp:29) bất kể flag — CÓ CHỦ ĐÍCH:
     166 đã thử đưa thẳng 15000/60000ms vào lớp Long và THUA 0/4/2
     (53->50 servings); không bao giờ nới cap để "tận dụng" thời gian thừa.
5. BINARY THI ĐẤU CHÍNH THỨC (cập nhật 2026-08-25, sau SCORE-ROLE-221):
   SHA256 `4EB926039A50D28F2202BFBE840866D770FD1928119183441C0034377BAA2FE4`
   — thêm short-horizon role fallback vào production (trận <=5 ngày không
   bao giờ chọn đội hình >=2 tanker khi có single-tanker trong beam; nguyên
   nhân trực tiếp của 2 trận thua rank-4 m-4195/m-4196). Sau khi build lại
   từ source, LUÔN xác minh bằng:
   a) udonshield_tests pass toàn bộ;
   b) replay-check trên m-4043/44/45 phải cho đúng 6/42/127, 6/60/364,
      6/60/144;
   c) replay-roles --short-role-fallback 1 trên m-4195 và m-4196 phải cho
      rank=1 với >=3 patrol (PPTP/PPPT-class), và trên m-4149/m-4155 phải
      giữ PPPP.
6. SAU TRẬN NGẮN (<=5 ngày) ĐẦU TIÊN với binary mới: kiểm tra frame
   assignment trong replay — kỳ vọng >=3 patrol (giá trị 0 = patrol,
   1 = tanker). Nếu thấy 2 tanker trở lên trong trận ngắn, báo ngay:
   đó là điều kiện revert của 221.
cd 'C:\Users\LMC\Desktop\4Fun'

# Sau khi bấm nut copy token da luu tren BTC, chi dua token qua environment
# cua dung tien trinh. Khong ghi token ro vao source, runbook hay evidence.
$env:HEXUDON_TOKEN = Get-Clipboard
.\build-release\udonshield_btc.exe http --match <MATCH_ID> --response-ms <PUBLIC_RESPONSE_MS> --replay artifacts/btc/<MATCH_ID>.jsonl
Remove-Item Env:HEXUDON_TOKEN
Set-Clipboard -Value ''

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
5. BINARY THI ĐẤU CHÍNH THỨC (tái chứng nhận 2026-08-28, source checkpoint
   18ecdd3 — sau 227
   diagnostic-isolation + fix final-day ACK theo quan hệ official):
   SHA256 `43ED5815DA0880652819BF589787C11CAFDC92F4D7D313899C3256E37D570389`
   — so với bản 1FC14A9E... (chốt 2026-08-26): (i) 227 tách diagnostic trace
   khỏi hot-loop chọn role production (compile-time, +229 servings vs parent
   sạch baebad8 trên holdout VM); (ii) 18ecdd3 sửa invariant hậu-ACK ngày
   cuối dùng quan hệ lexicographic official thay vì componentwise (loại rủi
   ro stale-reject action terminal đã được server chấp nhận). Lineage cũ:
   212EE9A4... (cùng source 18ecdd3, target-gated trên m-4476),
   1FC14A9E... (225+instrument), 4EB92603... (221). Sau khi build lại
   từ source, LUÔN xác minh bằng:
   a) udonshield_tests pass toàn bộ;
   b) replay-check phải cho đúng: m-4043 6/42/127, m-4044 6/60/364,
      m-4045 6/60/144, m-4290 6/42/159, m-4476 4/28/123;
   c) replay-roles --short-role-fallback 1 trên m-4195 và m-4196 phải cho
      rank=1 với >=3 patrol (PPTP/PPPT-class), và trên m-4149/m-4155 phải
      giữ PPPP.
   Battery a/b/c chạy lại đầy đủ và PASS trên đúng binary 43ED5815...
   ngày 2026-08-28. CTest pass 3/3; master oracle 5000/5000; population
   oracle 1000 seed/16000 candidate khớp SHA256 12B18893.... Target-host
   m-4787: hard 24x24, 7 ngày, 100 bước/ngày, 4 xe, 18 quán/4 chuỗi,
   low fuel, 15000 ms, 3 bot; 7/7 HTTP 200 valid, 6/6 transition
   reconciled, score 4/28/141, response max 5546 ms, main solve max
   3152 ms, compute window 5000 ms mọi ngày, zero emergency/failure.
   Public continuation chạy 7/7 ngày, 3564/3564 plan dual-valid và giữ
   checkpoint khi không có strict gain. Replay được lưu tại
   `research/evidence/BTC-FINAL-BUILD-20260828-m4787.jsonl`, SHA256
   `3A754B2EB745AC40C1113B65604A89815A2C4BB12C82ACD284D9AD57725F5158`.
   Các nghiên cứu hiệu suất 233--236 đã đóng; không còn candidate production
   đang mở — chỉ mở lại theo điều kiện reopen trong research/STATE.md.
6. SAU TRẬN NGẮN (<=5 ngày) ĐẦU TIÊN với binary mới: kiểm tra frame
   assignment trong replay — kỳ vọng >=3 patrol (giá trị 0 = patrol,
   1 = tanker). Nếu thấy 2 tanker trở lên trong trận ngắn, báo ngay:
   đó là điều kiện revert của 221. Thêm từ 225: nếu trận ngắn có
   fuel <= daySteps mà assignment lại là toàn-patrol (0 tanker),
   báo ngay — điều kiện revert của 225.

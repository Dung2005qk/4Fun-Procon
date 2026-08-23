cd 'C:\Users\LMC\Desktop\4Fun'

$env:HEXUDON_TOKEN = 'bot-16e63de59d414edd0147e7f1'
.\build-release\udonshield_btc.exe http --match m-3986 --response-ms 5000 --replay artifacts/btc/m-3986.jsonl


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
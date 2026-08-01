# Tài liệu API trận đấu

Base URL: `https://procon.ptit.edu.vn`. Bot xác thực bằng token đội qua header `Authorization: Bearer <token>`; không lưu token thật trong repository.

## Ba cách kết nối

- HTTP polling: `/api/v1/matches/{id}/...`; poll state và gửi actions.
- WebSocket: `/ws/v1/matches/{id}?token=...`; server đẩy state và bot gửi actions trên cùng kết nối.
- Sandbox: stdin/stdout JSON Lines; log chỉ ghi ra stderr.

WebSocket và sandbox gửi khung theo thứ tự `setup -> assignment -> day_state/actions -> result`. Message không có trường `type`; phân biệt bằng các trường dữ liệu.

## Contract quan trọng

- `pos = row * width + col`; `map.cells` dùng `0=đất`, `1=đường`, `2=núi`, `3=ao`.
- Hình học là even-r, hàng chẵn lệch phải; setup không chứa danh sách ô kề.
- Ngày wire bắt đầu từ `0`.
- `/setup`, `/state`, `/assignment` có thể trả HTTP `425` trước khi mở trận.
- Poll tối thiểu `200 ms`; HTTP `429` tương ứng `E_RATE_LIMIT`.
- Fuel đối thủ có thể bị ẩn; chiến lược không được phụ thuộc vào giá trị đó.
- `action_result.day` là ngày authoritative 1-based. Runtime phải đối chiếu với `state.day + 1`; ACK lệch ngày không được cập nhật ledger vì plan có thể đã bị áp vào một state khác.

## Luồng HTTP

- `GET /api/v1/matches/{id}/setup`
- `POST /api/v1/matches/{id}/assignment`
- `GET /api/v1/matches/{id}/start`
- `GET /api/v1/matches/{id}/state`
- `POST /api/v1/matches/{id}/actions` — có thể gửi lại cùng body khi mất ACK; runtime không được tái lập kế hoạch hoặc cập nhật ledger giữa các lần gửi lại
- `GET /api/v1/matches/{id}/result`

Setup chứa `daySteps`, `map`, `spots`, `agents`, `fuelLimits` và các tham số trận. Assignment là mảng phẳng với `0=patrol`, `1=tanker`, cố định cả trận.

Day state chứa `day`, `agents`, `others`, `traffics`. Kế hoạch là mảng-của-mảng; `0..5` là hướng và số âm `-N` là WAIT `N` bước. Tổng thời lượng mỗi agent không vượt `daySteps`; runtime nên đệm WAIT cho đủ.

Hướng theo chiều kim đồng hồ từ trên-trái:

- `0`: trên-trái
- `1`: trên-phải
- `2`: phải
- `3`: dưới-phải
- `4`: dưới-trái
- `5`: trái

Chi phí tính theo ô nguồn: đất `2 bước/1 fuel`, núi `3/2`, đường smooth `1/2`, busy `2/2`, jammed `4/2`; ao không đi được. Một agent sai làm cả submission bị từ chối.

Các lỗi action chính: `E_NOT_ADJACENT`, `E_POND`, `E_STEP_OVERFLOW`, `E_NO_FUEL`, `E_BAD_FORMAT`, `E_RATE_LIMIT`.

Không dùng toàn bộ `endsAt - now` cho solver. Runtime hiện giữ floor HTTP `1500 ms` và bỏ gửi khi còn dưới `1000 ms`; các số này là guard an toàn từ quan sát BTC, chưa phải tuyên bố p99 competition-ready.

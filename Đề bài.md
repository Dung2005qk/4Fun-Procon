&#x20;Thể lệ thi đấu HEXUDON

Tối ưu hoá lộ trình HEXUDON — Procon Việt Nam 2026. Nội dung tóm lược từ đề bài chính thức. Mục có ký hiệu 🔶 phụ thuộc thông số BTC công bố cho từng trận.



1\. Tổng quan

2\. Đơn vị thời gian

3\. Bản đồ \& địa hình

4\. Đường bộ \& lưu lượng giao thông

5\. Điểm đến, chuỗi nhượng quyền \& kho dự trữ

6\. Tác nhân \& nhiên liệu

7\. Tiến trình một trận

8\. Tham số đầu ngày

9\. Xác định người thắng

10\. Lưu ý thi đấu

🔎 Tìm nhanh trong mục lục…

1\. Tổng quan

Trong phần thi Tối ưu hoá lộ trình HEXUDON, mỗi đội điều khiển hai loại tác nhân — xe tuần tra và xe tiếp nhiên liệu — di chuyển trên một bản đồ lưới lục giác. Trên bản đồ có các điểm đến nơi xe tuần tra thu thập "udon". Xe tuần tra tiêu thụ nhiên liệu để di chuyển và thu udon; xe tiếp nhiên liệu bổ sung nhiên liệu cho xe tuần tra.



Đội nào ghé thăm các địa điểm hiệu quả nhất và thu được nhiều loại udon nhất sẽ giành chiến thắng.



2\. Đơn vị thời gian

Đơn vị nhỏ nhất để ra lệnh là một "step" (bước).

Một trận đấu chia thành nhiều "ngày"; mỗi ngày có một số bước nhất định.

Số bước mỗi ngày có thể thay đổi theo từng ngày.

Một trận đấu kéo dài từ 4 đến 10 ngày.

3\. Bản đồ \& địa hình

Bản đồ gồm các ô lục giác; mỗi ô tiếp giáp tối đa 6 hướng. Ô được đánh số từ 0 đến (rộng × cao − 1) theo hàng.

Kích thước bản đồ: mỗi chiều tối thiểu 8, tối đa 32 ô.

Bốn loại địa hình: Đồng bằng, Núi, Ao, Đường bộ. Tác nhân đi qua được đồng bằng/núi/đường (số bước khác nhau), không đi qua ao.

Địa hình mỗi ô không đổi suốt trận. Bản đồ trong setup chỉ gồm width/height/cells (không có sẵn danh sách ô kề) — bot tự tính hình họclục giác even-r (hàng chẵn lệch phải). Xem code mẫu ở trang Tài liệu API.

Bảng chi phí di chuyển (Bảng 1)

Địa hình / trạng thái	Số bước	Nhiên liệu (xe tuần tra)

🟩 Đồng bằng	2	1

🟧 Núi	3	2

🟦 Ao	Không đi được	—

⬜ Đường — Thông thoáng	1	2

⬜ Đường — Đông đúc	2	2

⬜ Đường — Ùn tắc	4	2

Chi phí tính theo ô hiện tại (ô nguồn) khi nhận lệnh di chuyển. Xe tiếp nhiên liệu di chuyển không tốn nhiên liệu. Giá trị theo Bảng 1 chính thức của BTC.



4\. Đường bộ \& lưu lượng giao thông

Ô đường có ba trạng thái: Thông thoáng, Đông đúc, Ùn tắc, xác định bởi lưu lượng giao thông của hai ngày trước đó.

Lưu lượng = tổng số bước dừng lại của mọi tác nhân của tất cả các đội trên ô đó trong hai ngày trước, chia cho số đội.

So với ngưỡng đông đúc và ngưỡng ùn tắc 🔶 (thay đổi theo trận): nhỏ hơn ngưỡng đông đúc → thông thoáng; từ ngưỡng đông đúc đến dưới ngưỡng ùn tắc → đông đúc; ≥ ngưỡng ùn tắc → ùn tắc.

Ngày 1 mọi đường đều thông thoáng; Ngày 2 chỉ tính theo lưu lượng Ngày 1.

Trạng thái đường do máy chủ cấp đầu mỗi ngày, giống nhau cho mọi đội và không đổi trong ngày.

5\. Điểm đến, chuỗi nhượng quyền \& kho dự trữ

Điểm đến nằm trên một số ô đồng bằng. Xe tuần tra đến điểm sẽ tự động thu một phần udon.

Mỗi xe tuần tra chỉ thu udon ở lần ghé đầu tiên mỗi điểm mỗi ngày (ghé lại trong ngày không thu thêm).

Điểm đến chia theo chuỗi nhượng quyền; mỗi chuỗi ứng với một loại udon. Vị trí \& chuỗi không đổi suốt trận.

Mỗi điểm có kho dự trữ tối đa (1 đến số tác nhân/đội), bổ sung đầy vào đầu mỗi ngày. Kho về 0 thì không thu được nữa.

Kho dự trữ độc lập theo từng đội — đội khác thu không làm giảm kho của bạn.

6\. Tác nhân \& nhiên liệu

Mỗi đội có 3 đến 8 tác nhân, khởi đầu ở các ô đồng bằng không có điểm đến.

Hai loại: Xe tuần tra (thu udon, tốn nhiên liệu) và Xe tiếp nhiên liệu (nạp nhiên liệu, không thu udon). Chọn loại cho từng tác nhân đầu trận, sau đó không đổi.

Tác nhân di chuyển sang ô kề (6 hướng) hoặc đợi tại chỗ một số bước. Lệnh tới ô không kề hoặc ô ao → không hợp lệ.

Xe tuần tra có dung lượng nhiên liệu tối đa 🔶 (như nhau mọi đội). Hết nhiên liệu phải đợi được tiếp. Lệnh di chuyển khi thiếu nhiên liệu/thiếu bước → không hợp lệ.

Xe tiếp nhiên liệu ở cùng ô với xe tuần tra ≥ 1 bước sẽ nạp đầy xe tuần tra; nó có nhiên liệu vô hạn và di chuyển không tốn nhiên liệu.

7\. Tiến trình một trận

Trước trận: cấp cấu hình bản đồ; người chơi chỉ định loại cho từng tác nhân.

Vào trận: máy chủ cấp thông tin bản đồ kèm lựa chọn tác nhân của mọi đội.

Trong thời gian phản hồi của Ngày 1, gửi hành động cho Ngày 1.

Hết thời gian Ngày 1, máy chủ cấp thông tin Ngày 2 (phản ánh vị trí \& giao thông cuối Ngày 1).

Lặp lại tương tự cho tới hết số ngày đã định.

8\. Tham số đầu ngày

Ngày 1: tác nhân ở vị trí cấu hình; nhiên liệu = tối đa; mọi đường thông thoáng; kho đầy.



Ngày 2 trở đi: vị trí = cuối ngày trước; nhiên liệu = còn lại cuối ngày trước; trạng thái đường theo lưu lượng hai ngày trước (Ngày 2 chỉ theo Ngày 1); kho được bổ sung đầy.



9\. Xác định người thắng

Xét theo thứ tự ưu tiên:



Nhiều loại udon khác nhau nhất trong trận.

Tổng lũy kế số loại udon thu mỗi ngày cao nhất.

Tổng số phần udon thu được nhiều nhất.

Tổng thời gian phản hồi (câu trả lời hợp lệ cuối mỗi ngày) thấp nhất.

Nếu vẫn hoà: tung xúc xắc hoặc tuyên bố hoà.

10\. Lưu ý thi đấu

Mỗi trận nhiều đội đấu đồng thời; số đội tuỳ cặp đấu, công bố trong hướng dẫn cuối cùng.

Số ngày, số bước/ngày và thời gian phản hồi xác định riêng cho từng trận.

Máy chủ báo mỗi câu trả lời là hợp lệ hay không (lỗi định dạng…). Câu trả lời hợp lệ cuối cùng được áp dụng.

Được gửi lại trong thời hạn, nhưng gửi quá nhiều/tệp quá lớn gây gián đoạn có thể bị coi là gây rối và bị loại.

Poll ≥ 200ms; quá nhanh nhận HTTP 429. Có thể có độ trễ nhỏ khi cập nhật bản đồ.


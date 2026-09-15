# Quyết định bản thi đấu vòng loại 2026-09-10

> **Lịch sử, chưa còn là quyết định cuối.** Sau báo cáo này người dùng yêu cầu
> tiếp tục điều tra362/366 vì chưa đến giờ thi. Phần chốt/pause bên dưới được
> supersede bởi trạng thái hiện hành trong research/STATE.md và attribution369.
> Các hash, kết quả test và evidence đã ghi vẫn giữ nguyên.

## Quyết định cuối: dùng accepted258; không promote362/366 cho vòng loại

Theo yêu cầu trực tiếp ngày2026-09-10, chốt lựa chọn build sau khi protected366-P2
và forensic368 hoàn tất; tạm dừng nghiên cứu, tiếp tục sau khi người dùng cung cấp
replay vòng loại. Không mở matrix, nghiên cứu2xe, trận BTC hoặc monitor mới.

Tôi, Codex, chọn và chịu trách nhiệm về khuyến nghị kỹ thuật dùng **accepted258
trên canonical HEAD c76a8eaa4f200e3eeeb1a58ef1d1fb3d0c13579c** cho vòng loại
chiều nay. Đây là bản đã qualification trong phạm vi evidence hiện có, không phải
bảo đảm vô địch hoặc tuyên bố solver đạt trần toán học/không còn gap.

Preservation: không xóa, tắt, bỏ qua hoặc giảm chức năng product. Không có phần
candidate362 nào đã tích hợp cần revert. Toàn bộ research source/binary, evidence
và VM hiện có được giữ nguyên. Không chỉnh AGENTS.md hay COMPETITION_RUNBOOK.md
của người dùng. Không commit/push vì không có cải thiện product mới được promote.

## Vì sao không nâng362/366 ngay hôm nay?

Candidate có giá trị, không phải thất bại toàn bộ: DEV active24/24/0,+98 servings;
holdout active31/41/0,+147; protected active29/41/2,delta0/-2/+108. Toàn bộ108
protected fixtures chỉ chạy một pass,216sides1368ACK1152transitions,zero safety.
Ngày ký quyết định đã rehash toàn bộ1620 evidence files khớp completion manifest.

Nhưng protected10s12/22/2,delta0/-2/+29; protected15s17/19/0,delta0/0/+79.
Serving gross gain141/gross loss33; một active loss mất2 daily distinct, một loss
mất33 servings. Frozen active_benefit/active_no_upper_tier_loss/active_serving_tail/
active_strata chưa đạt. Các win/loss inactive5s **không** được dùng để chặn hoặc
credit cơ chế mới. Không mở lại điều tra minh oan5s.

Đã cân nhắc nguyên tắc ưu thế tổng quát: một vài regression nhỏ không tự động
loại một cơ chế. Ở đây, tuy win breadth và serving benefit rõ, phần protected
vẫn giảm tier daily-distinct và attribution timing toàn phiên chưa được nhận diện
đầy đủ. Không cộng108 servings để xóa2 daily bằng weighted sum, không bỏ riêng
hai loss, không dùng lại seed đã thấy làm xác nhận độc lập. Riêng15s đẹp không
đủ quyền bật dispatcher mới sau khi nhìn kết quả hoặc suy ra mọi cửa sổ>10s.

Forensic368 đã tách rõ: loss33 xuất hiện từ role selection trước treatment;
loss2daily xuất hiện sau main ngày5 đổi pool/F0/W1 rồi thiếu brand0/5 ngày9.
Không chứng minh resource certificate sai, comparator đảo score hoặc một hotfix
semantics-equivalent. Cũng chưa chứng minh timing gián tiếp của resource hoàn
toàn vô hại. Kết luận là **không đủ căn cứ qualification để thay bản thi đấu**,
không phải khẳng định258 mạnh hơn362 trên mọi map.

Thêm nữa, candidate chưa hoàn tất production integration/equivalence và genuine
BTC target-host gates trên exact production artifact. Linux loopback/VM không
phải BTC chính thức. Không bỏ những bước này để ký một binary chưa qualification
trước vòng loại. Không sửa gate sau quan sát rồi gọi là original pass.

Verdict: **NO-GO new parent362/366 for this qualifier; GO existing258 build
selection**. Frozen verdict và các báo cáo cũ không bị sửa thành pass.

## Exact artifact dùng cho vòng loại

- File: `C:\Users\LMC\Desktop\4Fun\build-release\udonshield_btc.exe`.
- SHA256: `F97F168FE76FEF1B6226D2C2CFDAF954F116D39F427B6F157594151C461E1275`.
- Size:1383936 bytes. Hash kiểm trước và sau kiểm thử đều không đổi.
- Canonical behavior: accepted258; main/role/checkpoint vẫn5000ms. Public
  continuation đã được promote dùng phần thời gian hợp lệ theo daySeconds/endsAt;
  không phải toàn response luôn bị chặn5s. Không bật resource362/366.
- Giữ `--response-ms 5000`; dùng **match ID vòng loại mới**, không tái dùng ID
  trận luyện tập trong runbook. Replay output riêng cho trận đó. Token chỉ vào
  environment của bot; báo cáo này không chứa token.

## Kiểm tra cuối đã thực hiện hôm nay, không rebuild

- `git diff -- src include`: empty; canonical source chưa bị nghiên cứu thay đổi.
- CTest theo đúng executable trong CMakeCache, chạy tuần tự một lượt:
  udonshield_tests PASS; strategy_bench_smoke PASS; master_oracle_smoke PASS.
  Tổng3/3,0fail,4.59seconds. Chỉ là regression/operational sanity, không dùng số
  đo local hoặc smoke scores làm promotion evidence.
- Test executable SHA256:
  `BF2E8B2F35D881EB69A1202BAC75AC7B43094EB62E938424E09B9349B2307F1B`.
- CTest LastTest.log hash tại thời điểm kiểm:
  `5AC95AD79494CB8EC0D3951BCF760B632D529596B347046BC565012F1F704667`.
- Exact current binary `replay-check --response-ms5000` trên replay có sẵn
  `artifacts/btc/m-13627.jsonl`: exit0;8/8days simulator-valid và validator-agrees;
 7/7transitions reconciled;resume8accepted;score8/64/279 khớp standings đã lưu.
- Replay SHA256:
  `DB267C6E51684483D65F2B4C8438CB5CC7395E9285DDFDA3E2C1C5D8A1F189E1`.
- Replay ghi8 ACK HTTP200 valid; response_ms do server ghi cao nhất6544ms trong
  window10000ms; main tối đa3377ms. Đây là telemetry **trận cũ**, không phải test
  BTC vừa chạy hôm nay hoặc xác nhận cấu hình/mạng vòng loại. Rank không dùng làm
  bằng chứng solver mạnh hơn. Không có trận mới được tạo bởi quyết định này.

## Tiếp tục sau vòng loại

Đợi người dùng cung cấp replay/setup/standings vòng loại. Trước hết xác minh
deadline/ACK/transition và vùng tham số thực tế; sau đó chọn đúng gap có evidence.
Giữ candidate362 để xem xét lại theo một quyết định prospective có thẩm quyền,
không bỏ nó khỏi lịch sử hoặc kết luận hết giá trị. Hai-xe vẫn chưa được nghiên
cứu trên parent mới và không được mô tả là đã hoàn thành. Không tự chạy nghiên
cứu trong lúc người dùng đang thi đấu; không tạo monitor chỉ để chờ replay.

## Nguồn quyết định

- [Protected366-P2 closure](C:/Users/LMC/Desktop/4Fun/research/evidence/SCORE-CAUSAL-RESOURCE-QUALIFICATION-366-protected-causal-closure.md).
- [Forensic368 closure](C:/Users/LMC/Desktop/4Fun/research/evidence/ATTR-ACTIVE-PROTECTED-DIVERGENCE-368-closure.md).
- [Causal amendment đã đóng băng](C:/Users/LMC/Desktop/4Fun/research/evidence/SCORE-CAUSAL-RESOURCE-QUALIFICATION-366-causal-decision-amendment.md).
- Protected summary `B401C7698036FFFF557B0520055D016AB328D3C7C5F83B57BE4CBD8233D97CB4`;
  completion `B3939DC611F695D86168BD1D6EA274592DF851A4BE9537DA66FB4E98B08A6C85`;
  forensic explanation `46652AD1F9EEBA99A777609ACB2E80C6B4030B79C6ACD2E274BF77609526D939`.

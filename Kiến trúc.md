# Ý TƯỞNG 4 - UDON-SHIELD: TỐI ƯU TỪ ĐIỂN VỚI VIABILITY FRONTIER, DANH MỤC LỘ TRÌNH ĐỒNG BỘ VÀ GIAO THÔNG CÓ RECOURSE

## 0. Kết luận ngắn

Ba ý tưởng trước đều có mảnh ghép tốt, đặc biệt là mô phỏng, tìm đường đa tiêu chí, rolling horizon, rendezvous và ALNS. Tuy nhiên, chưa ý tưởng nào khóa chặt toàn bộ chiến lược vào **đúng luật xếp hạng từ điển** của HEXUDON, cũng chưa xử lý đúng tính bất định của giao thông và thứ tự xử lý trong từng step.

Ý tưởng 4 đề xuất một kiến trúc có tên **UDON-SHIELD**:

1. Dùng **mô phỏng sự kiện chính xác theo từng step** làm nguồn sự thật duy nhất. Mọi phương án đều phải qua bộ mô phỏng trước khi được gửi.
2. Tối ưu bốn tiêu chí theo **thứ tự từ điển thật**, không dùng tổng trọng số tùy ý.
3. Trước mỗi quyết định, dựng một **viability frontier có comparator công khai**: prune bằng dominance đã chứng nhận, áp confidence gate luôn không rỗng và lưu witness cho cả tier 2 thay vì ép mọi route giữ một \(K^*\) worst-case duy nhất.
4. Không dự đoán giao thông bằng một con số duy nhất. Contribution của ta được mô phỏng deterministic; phần đối thủ là biến ngoại sinh trong tập likely/adversarial scenario. Kế hoạch hôm nay là quyết định chung trước quan sát, kế hoạch ngày sau được phép recourse khi server công bố traffic thật.
5. Sinh nhiều **cột lộ trình** cho từng xe bằng tìm đường đa tài nguyên; sau đó dùng CP-SAT/set-packing chọn tổ hợp xe tuần tra - xe tiếp nhiên liệu đồng bộ theo thời gian. ALNS chỉ là máy sinh và cải tiến lộ trình, không được phép phá thứ tự mục tiêu hay tính hợp lệ.
6. Khai thác hai hiệu ứng mạnh nhưng phải trả đúng chi phí của luật step:
   - **Explicit overnight harvest:** kết thúc ngày trên một spot, rồi đầu ngày sau phát `WAIT(1)` để hoàn thành một action tại spot và nhận udon trước khi rời đi. Không có pickup tự động ở đầu ngày và chi phí là đúng một step.
   - **End-day docking:** refuel dùng được trong cùng ngày vẫn cần đồng vị trí liên tục qua hai snapshot liên tiếp. Riêng transition sang ngày kế tiếp, BTC đã xác nhận patrol chỉ cần cùng terminal cell với tanker ở snapshot cuối; fuel đầy này không được dùng hồi tố cho action của ngày vừa kết thúc.

Điểm hơn cốt lõi không phải là “thuật toán phức tạp hơn”, mà là: **không tối ưu sai mục tiêu, không hi sinh loại udon hiếm một cách vô thức, luôn có phương án ngày hiện tại hợp lệ qua mô phỏng, và phân biệt rõ future bound với future certificate**.

---

## 1. Những sự thật chiến lược phải khóa trước khi tối ưu

### 1.1. Hàm điểm là một vector từ điển, không phải tổng điểm có trọng số

Gọi:

- \(B_d\): tập loại udon thu được trong ngày \(d\);
- \(Q_d\): tổng số phần udon thu được trong ngày \(d\);
- \(R_d\): thời gian đến lần gửi câu trả lời hợp lệ cuối cùng của ngày \(d\).

Vector kết quả cuối trận là:

\[
Z = \left(
\left|\bigcup_{d=1}^{D} B_d\right|,
\sum_{d=1}^{D}|B_d|,
\sum_{d=1}^{D}Q_d,
-\sum_{d=1}^{D}R_d
\right)
\]

và phải được cực đại hóa theo thứ tự từ điển.

Hệ quả:

- Không tồn tại bộ trọng số cố định \(w_1,w_2,w_3\) an toàn cho mọi trận nếu không dùng các hằng số Big-M được chứng minh từ cận trên. Một phần udon hay một loại udon lặp trong ngày không bao giờ được đánh đổi lấy một loại udon mới của toàn trận.
- “Mỗi ngày lấy đủ mọi brand” là mục tiêu rất tốt **chỉ sau khi** đã bảo đảm tối đa hóa số brand của toàn trận. Có tình huống phải bỏ vài brand dễ trong hôm nay để tiếp cận một brand hiếm chỉ còn cơ hội lấy ở ngày hiện tại.
- Thời gian phản hồi chỉ quan trọng khi ba thành phần đầu bằng nhau. Không được làm giảm chất lượng lộ trình chỉ để tiết kiệm vài trăm mili giây.

### 1.2. Các đội không tranh kho udon và xe không va chạm nhau

- Kho của spot độc lập theo từng đội. Đối thủ thu udon không làm giảm kho của ta.
- Nhiều agent được phép ở cùng một ô. Đây còn là điều kiện bắt buộc để tiếp nhiên liệu.

Vì vậy:

- Không cần CBS để tránh xung đột ô như bài toán MAPF cổ điển.
- Không cần “đua” với đối thủ đến spot để giành kho.
- Tương tác chiến lược trực tiếp duy nhất giữa các đội trong dữ liệu đã công bố là **giao thông dùng chung**; phần còn lại chủ yếu là cuộc đua tối ưu score riêng.

### 1.3. Thời gian và nhiên liệu không tỉ lệ với nhau

Chi phí của một lệnh rời ô \(u\) phụ thuộc vào **địa hình ô nguồn**:

| Ô nguồn | Step | Fuel của patrol |
|---|---:|---:|
| Đồng bằng | 2 | 1 |
| Núi | 3 | 2 |
| Road thông | 1 | 2 |
| Road đông | 2 | 2 |
| Road tắc | 4 | 2 |

Do đó:

- Đường nhanh nhất chưa chắc tiết kiệm nhiên liệu nhất.
- Đường ít nhiên liệu nhất chưa chắc kịp ngân sách step.
- Cạnh \(u\rightarrow v\) và \(v\rightarrow u\) có thể có chi phí khác nhau vì địa hình ô nguồn khác nhau. Đồ thị phải được coi là **đồ thị có hướng theo chi phí**, dù quan hệ kề ô là hai chiều.
- Một khoảng cách vô hướng “effective step-fuel distance” sẽ làm mất các nghiệm Pareto quan trọng. Cần giữ nhiều nhãn không trội \((time,fuel,road\ footprint)\).

### 1.4. Giao thông là tải lưu trú thực, không chỉ là lệnh Wait

Traffic đếm ô mà agent đang ở **sau pha phản ánh di chuyển** của từng step; step 0 không đếm, step cuối có đếm. Khi một lệnh di chuyển mất nhiều step, agent vẫn lưu trú ở ô nguồn trong các step trung gian và có thể làm tăng traffic tại road đó.

Vì vậy câu “chỉ cần không Wait trên road thì traffic bằng 0” là sai. Kết thúc ngày ở ô thường là tốt để tránh các step đệm trên road, nhưng mọi phương án vẫn phải mô phỏng chính xác dấu chân giao thông của cả quá trình di chuyển.

### 1.5. Thứ tự trong một step tạo ra ba chiến thuật quan trọng

Chuỗi xử lý cốt lõi là: tiêu thụ fuel của movement hoàn thành, phản ánh action hoàn thành, thu udon tại vị trí sau action, kiểm tra tiếp nhiên liệu liên tục, rồi cập nhật traffic. Các conformance replay với server BTC xác nhận:

- Udon chỉ được xét khi một action hoàn thành. Đứng trên spot ở step 0 không tự thu; phát movement ngay cũng không thu spot nguồn vì pickup được xét sau khi movement đã phản ánh vị trí đích.
- Muốn thu spot đang đứng ở đầu ngày, patrol phải hoàn thành `WAIT(1)` tại step 1; sau đó mới phát movement rời spot.
- Patrol chỉ được nạp khi có tanker cùng ô ở snapshot hiện tại **và** snapshot ngay trước đó. Cùng vừa đến một ô trong một snapshot chưa đủ.
- Fuel vừa được nạp chỉ dùng cho movement được nhận sau thời điểm nạp; movement đã được nhận vẫn phải đủ fuel tại lúc nhận lệnh.
- Nếu patrol và tanker xuất phát cùng ô và đi cùng timeline, co-location được duy trì nên patrol có thể được nạp sau movement. Đây là **escort thật sự**, không phải chỉ là rendezvous rời rạc.
- Rendezvous mới hình thành đúng step cuối không cấp fuel cho action nào trong ngày hiện tại, nhưng server BTC cấp fuel đầy trong state đầu ngày kế tiếp. Conformance tự nhiên `m-1258` và probe cô lập `m-1261` xác nhận boundary rule này.
- Nếu nhiều patrol cùng lấy spot khi kho không đủ, thứ tự agent trong cấu hình quyết định xe nào lấy trước. Đây không phải suy đoán: Q&A phần 2, câu 26 quy định agent đứng trước trong danh sách map lấy trước; phụ lục Q6 còn minh họa hai patrol đến cùng step, stock bằng 1 và agent có thứ tự thấp hơn nhận udon. Bộ mô phỏng và master solver phải bảo toàn thứ tự này. Adapter Việt Nam vẫn cần một conformance test với server tập luyện để phát hiện nếu BTC địa phương thay format hoặc semantics.

### 1.6. Mọi action plan phải dùng đúng toàn bộ số step

Mỗi agent phải có tổng thời lượng hành động đúng bằng \(S_d\). Chỉ một agent sai, toàn bộ câu trả lời có thể bị reject. Phần step dư phải được lấp bằng Wait hợp lệ; vị trí Wait ảnh hưởng traffic và trạng thái cuối ngày.

Đây là lý do tính hợp lệ không thể là một penalty mềm trong fitness. Nó phải là ràng buộc cứng và được xác minh lại bằng simulator.

---

## 2. Đánh giá ba ý tưởng trước

### 2.1. Ý tưởng 1

**Điểm tốt**

- Nhìn đúng vai trò ưu tiên của đa dạng brand.
- Đề xuất simulator, Monte Carlo và sensitivity analysis.
- Nhận ra phải xét time, fuel, traffic và trạng thái nhiều ngày.
- Mở ra hướng kết hợp tìm kiếm với chính sách học.

**Điểm chưa đủ để làm chiến lược thi đấu chính**

- Đồng nhất “giảm bước di chuyển” với “giảm nhiên liệu” trong khi bảng chi phí cho thấy hai đại lượng không tỉ lệ.
- Đề xuất CBS tránh va chạm, nhưng luật cho phép nhiều agent cùng ô; dùng CBS ở đây vừa tốn tính toán vừa có thể loại bỏ nghiệm tiếp nhiên liệu tốt.
- MARL quá khó để bảo đảm không sinh action invalid, khó chứng minh thứ tự mục tiêu, và không có đủ phân phối map chính thức để tin rằng policy sẽ tổng quát hóa.
- Chưa có cơ chế bảo vệ brand hiếm của toàn trận trước lợi ích trong một ngày.
- Chưa mô tả chính xác stock nội bộ, thứ tự agent và timing của refuel.

**Kết luận:** phù hợp làm bản khảo sát hướng đi và môi trường benchmark, không nên là bộ ra quyết định cuối cùng.

### 2.2. Ý tưởng 2

**Điểm tốt**

- Chuyển bài toán thành TOP có ràng buộc fuel là hướng mô hình hóa hợp lý.
- Đặt vấn đề phân vai, cache đường đi, rendezvous, traffic và fallback.
- Nhận ra tanker tận dụng road tốt vì không tốn fuel.

**Điểm yếu quyết định**

- Dùng hàm fitness có trọng số có thể đảo sai thứ tự thắng thua.
- Kết luận “mỗi ngày phải 100% brand” bỏ qua trường hợp brand hiếm của toàn trận cần được lấy trước.
- Quy tắc cố định \(N-1\) patrol + 1 tanker không xét vị trí xuất phát riêng của từng agent. Với tối đa 8 agent, hoàn toàn có thể đánh giá mọi bitmask phân vai thay vì đoán bằng heuristic.
- “Gây ùn tắc đối thủ” bị xem là chiến lược mặc định dù traffic là chung cho tất cả, đóng góp của ta còn bị chia cho số đội, và kho udon không tranh chấp.
- Gửi fallback sớm rồi gửi đè không làm thời gian phản hồi nhỏ: tie-break dùng **lần gửi hợp lệ cuối cùng**.
- Floyd-Warshall trên một trọng số duy nhất không giải quyết đúng Pareto time-fuel và dấu chân traffic.

**Kết luận:** là baseline triển khai tốt, nhưng cần thay weighted fitness bằng tối ưu từ điển và thay heuristic vai trò bằng đánh giá toàn bộ assignment.

### 2.3. Ý tưởng 3

**Điểm tốt**

- Rolling horizon và terminal state là bước tiến đúng.
- Nhận ra rendezvous phải có tọa độ và thời điểm.
- ALNS chuyên biệt phù hợp hơn GA/MARL thuần túy cho routing có ràng buộc.
- Kiến trúc anytime và precomputation là hướng thực dụng.

**Điểm còn thiếu hoặc có giả định nguy hiểm**

- Time-expanded state của từng patrol chưa tự giải quyết stock chung, thứ tự agent và đồng bộ tanker ở cấp đội.
- Voronoi “không chồng lộ trình” tuyệt đối có thể làm mất nghiệm tốt: nhiều patrol cần ghé cùng spot để rút stock lớn; nhiều xe dùng chung corridor hoặc escort cũng có thể tối ưu.
- MCMF thông thường không đủ biểu diễn rendezvous đồng bộ theo thời gian, escort nhiều step và thứ tự phục vụ có phụ thuộc fuel.
- Traffic volume thật không được server công bố, chỉ có trạng thái road. Lập kế hoạch như thể biết chính xác tải hai ngày là quá tự tin.
- Bẫy traffic có thể hại chính mình và không chắc đủ lực vượt threshold sau khi chia cho số đội.
- Không có khoảng nghỉ giữa hai ngày; ngày sau bắt đầu ngay. Precompute phải diễn ra sau khi đã gửi phương án hiện tại hoặc dựa trên các nhánh traffic dự đoán, không dựa vào “giờ nghỉ”.
- Gửi fallback rồi gửi đè chỉ là bảo hiểm hợp lệ, không phải “zero-delay” cho tie-break.
- Quan trọng nhất: ALNS/CP-SAT vẫn chưa được buộc phải tối ưu chính xác theo bốn tầng score và chưa có chứng chỉ rằng terminal state còn lấy được brand hiếm.

**Kết luận:** đây là nền gần nhất với một solver mạnh, nhưng cần một tầng điều phối cao hơn. UDON-SHIELD giữ rolling horizon và ALNS, đồng thời thêm score từ điển, uncertainty, route portfolio, exact simulator và viability shield.

---

## 3. Hạt nhân Ý tưởng 4: tối ưu từ điển với viability frontier có kiểm soát rủi ro

### 3.1. Trạng thái đầy đủ

Ở đầu ngày \(d\), trạng thái ra quyết định gồm:

\[
x_d = (p_i, f_i, kind_i, B^{life}, C^{daily}, Q, T_d, \mathcal{L}_d, O_d)
\]

trong đó:

- \(p_i,f_i,kind_i\): vị trí, fuel và loại của từng agent;
- \(B^{life}\): tập brand đã từng lấy trong toàn trận;
- \(C^{daily}\): tổng số brand theo ngày đã cố định;
- \(Q\): tổng phần udon đã lấy;
- \(T_d\): trạng thái road hiện tại;
- \(\mathcal{L}_d\): belief/tập khoảng cho tải road ẩn của hai ngày gần nhất;
- \(O_d\): vị trí, fuel và loại của agent đối thủ nếu API cung cấp.

### 3.2. Mục tiêu còn lại của trận và hồ sơ rủi ro

Với một chính sách \(\pi\) và một kịch bản traffic/opponent \(\omega\), score còn lại là:

\[
Z_d(\pi,\omega)=
\left(
|B^{life}\cup\bigcup_{t=d}^{D}B_t|,
\sum_{t=d}^{D}|B_t|,
\sum_{t=d}^{D}Q_t
\right)
\]

Thời gian phản hồi được xử lý ở tầng gửi đáp án, không trộn vào routing khi ba mục tiêu trên chưa khóa.

Một điểm phải thừa nhận thẳng: luật xếp hạng là deterministic trên kết quả **đã xảy ra**, còn lúc ra quyết định outcome vẫn bất định. Không tồn tại lựa chọn duy nhất “suy ra từ luật” giữa một route liều và một route an toàn. Mọi bot đều phải có một **risk policy chủ quan** ở đâu đó. Mục tiêu không phải giả vờ loại bỏ chủ quan, mà là:

- không đặt trọng số tùy ý giữa ba tier chính thức khi outcome đã biết;
- cô lập chủ quan ở tầng risk policy;
- công khai tham số, calibrate offline và khóa chúng trước trận thay vì đổi bằng cảm giác.

Mỗi candidate giữ:

- \(K_{cap}\): coverage ceiling - coverage lớn nhất trong một kịch bản còn hợp lệ;
- \(K_{safe}\): coverage floor trong public-adversarial stress scenarios;
- \(P_{\ge k}=\Pr(K\ge k)\): survival profile coverage;
- \(C_{\rho}(a)=\max\{k:\Pr(K_a\ge k)\ge\rho\}\): coverage đạt được với độ tin cậy ít nhất \(\rho\);
- các cận/witness tier 2 và tier 3 có điều kiện theo coverage.

Xác suất scenario chỉ được dùng nếu generator/weight đã calibrate trên replay. Nếu chưa có calibration, dùng số lượng cố định cho từng **scenario class** và trọng số class cố định; khi đó \(\Pr\) là một discrete policy measure chính xác theo manifest, không phải tuyên bố tần suất thật của đối thủ. Không để việc lấy mẫu nhiều đường cùng loại vô tình làm tăng probability của loại đó. \(K_{safe}\) là stress metric, không tự động được gán xác suất lớn.

Không đặt yêu cầu máy móc “vài trăm scenario cho mỗi candidate”. Nhiều sample tương quan không tạo thêm thông tin và có thể phá budget. Nếu dùng probability model học từ replay, manifest phải ghi effective sample size/calibration error và chọn \(\varepsilon\) không nhỏ hơn độ phân giải thống kê đã validate. Nếu ensemble ngày hiện tại không đạt minimum effective coverage của learned manifest, fail closed sang frozen class-weight fallback manifest và recompute toàn epoch. Nếu ngay cả fallback thiếu một scenario class bắt buộc, gán phần trọng số thiếu cho certified pessimistic outcome tương thích với bounds và vô hiệu hóa \(G\) cho cả epoch, tức đặt cùng một empty signature cho mọi candidate; total order vẫn được hoàn tất bởi các key sau. Không bỏ im lặng class khó và không dùng chữ số xác suất giả chính xác để phân hạng.

### 3.3. Comparator toàn phần: từ frontier đến đúng một action

Phiên bản đầu dùng policy tĩnh. Candidate mặc định đưa vào calibration trước trận là:

- confidence \(\rho=0.80\);
- safety slack \(\delta=1\) brand;
- probability resolution \(\varepsilon=0.01\).

Đây là **giá trị khởi đầu để calibrate**, không phải hằng số suy ra từ luật. Manifest production chỉ được chốt sau held-out benchmark và sau đó bất biến suốt trận; không tự động thay \(\rho,\delta\) theo thế trận.

Quy trình chọn:

1. Loại candidate invalid; giữ incumbent simulator-confirmed.
2. Prune candidate chỉ bằng certified stochastic dominance như mục 3.4, không dùng \(K_{cap}\) làm hard filter.
3. Tính mức confidence tốt nhất \(C_{\rho}^{best}=\max_a C_{\rho}(a)\). Tập đủ an toàn là:

\[
\mathcal{A}_{\rho,\delta}
=\{a:C_{\rho}(a)\ge C_{\rho}^{best}-\delta\}
\]

Tập này luôn không rỗng, nên không có nghịch lý “route mơ ước bị risk gate chặn còn route an toàn đã bị hard guard xóa”.

4. Đóng băng **scenario manifest** \(\Omega_d\) trước khi bắt đầu search ngày \(d\); mọi candidate được đánh giá trên đúng manifest đó và không được thêm scenario cho riêng một candidate. Candidate pool được phép tăng trong anytime loop. Với mỗi candidate mới \(a\) và \(\omega\in\Omega_d\), lấy full score tương lai đã có witness:

\[
y_{a,\omega}=(K_{a,\omega},H_{2,a,\omega},H_{3,a,\omega})
\]

Nếu chưa sửa được future witness hợp lệ cho một scenario, comparator dùng certified lower outcome của scenario đó; không được lấp chỗ trống bằng terminal UB. Estimated outcome chỉ được dùng để ưu tiên search.

Không gian toán học chung là tập hữu hạn các score nguyên hợp lệ của cấu hình trận:

\[
\mathcal Y_{off}\subseteq
\{0,\ldots,M\}\times
\{0,\ldots,D M\}\times
\{0,\ldots,D\sum_s stock_s\}
\]

được sắp duy nhất bằng \(<_{lex}\). Quantile trả trực tiếp một phần tử của không gian chung này, nên mỗi candidate có thể giữ sorted sparse histogram riêng; không cần rebuild profile cũ khi chỉ thêm candidate mới.

Với mỗi confidence level \(q\), định nghĩa quantile trên total order chung này; kết quả vẫn là một score vector thật, không phải utility nhân trọng số:

\[
Q_q(a)=\max_{lex}\{y\in\mathcal Y_{off}:\Pr(y_{a,\omega}\ge_{lex}y)\ge q\}
\]

Policy v1 dùng ladder cố định \(\mathcal Q=(0.95,0.80,0.50,0.20,0.05)\): bảo vệ downside trước, rồi mới xét median và upside. Candidate được bucket bằng five-quantile signature. Chỉ với một bucket \(T\) có từ hai candidate trở lên, evaluator tạo grid chung cục bộ \(\mathcal Y_{eval}(T)=\{y^1<_{lex}\cdots<_{lex}y^L\}\) là hợp support của tie-group \(T\), rồi tạo survival signature từ outcome xấu lên tốt:

\[
G(a)=\left(
\left\lfloor\frac{\Pr(y_a\ge_{lex}y^2)}{\varepsilon}\right\rfloor,
\ldots,
\left\lfloor\frac{\Pr(y_a\ge_{lex}y^L)}{\varepsilon}\right\rfloor
\right)
\]

Hai lớp có vai trò khác nhau, không phải tính trùng. Ladder là **risk semantics**: nó khóa ưu tiên tại năm mức confidence được công bố. \(G\) chỉ chạy khi toàn bộ năm quantile bằng nhau và làm **completion tie-break** trong lớp tương đương đó. Nếu bỏ ladder và dùng \(G\) ngay từ đầu, policy sẽ trở thành lexicographic tail-aversion cực đoan, nơi một thay đổi rất nhỏ ở ngưỡng xấu nhất có thể lấn mọi quantile còn lại. Inner loop chỉ đánh dấu tie-group `dirty`; không tính lại \(G\). Hai checkpoint bắt buộc mới finalize dirty groups: ngay trước lập W1 shortlist và sau W1 ngay trước quyết định gửi.

So \(G\) từ ngưỡng thấp lên cao có nghĩa trước hết giảm xác suất rơi vào tail xấu; lượng tử hóa theo \(\varepsilon\) ngăn nhiễu xác suất rất nhỏ đảo quyết định. Mọi candidate trong ngày phải dùng cùng trọng số scenario. Không được cập nhật thêm sample trong anytime loop. Nếu manifest bắt buộc phải đổi trước search vì fallback/gate, recompute tất cả đúng một lần rồi freeze lại.

5. Trong \(\mathcal{A}_{\rho,\delta}\), chọn lexicographically theo total key:

\[
Key(a)=
(Q_{0.95}(a),Q_{0.80}(a),Q_{0.50}(a),Q_{0.20}(a),Q_{0.05}(a),G(a),
LB^{cert}(a),scoreToday(a),terminalSlack(a),trafficSafety(a),-stableId(a))
\]

Mỗi \(Q_q\), \(LB^{cert}\) và `scoreToday` tự nó được so theo ba tier chính thức. `trafficSafety` là exact counterfactual key tính từ full own footprint: trước hết giảm số road mà contribution của ta làm vượt busy/jammed threshold trong scenario manifest, sau đó giảm số road bị đẩy vào safety band sát threshold, cuối cùng giảm tổng stay-step không cần thiết. Nó chỉ được xét sau score và terminal slack; không thưởng traffic poisoning. `stableId` là canonical byte serialization của action sequence và chỉ phân xử khi mọi quality key đã bằng nhau. Có thể dùng hash để index nhanh, nhưng nếu hash trùng phải fallback về so byte, nên total order không dựa vào giả định “hash không collision”. `stableId` không cần certificate riêng vì không đưa ra tuyên bố về chất lượng; nó chỉ chọn đại diện trong một equivalence class. \(UB\) lạc quan chỉ ưu tiên candidate cho search, không được dùng để đánh bại certified key khi chọn/gửi.

Comparator này không phải “chân lý không tham số”; nó là một risk policy executable, versioned và có thể benchmark. Nó không cộng brand với servings hay giây phản hồi; tham số \(\rho,\delta,\varepsilon\) chỉ quyết định thái độ trước uncertainty.

Comparator cũng không được mã hóa thành một khối biến khổng lồ trong CP-SAT. Master giữ tối đa 32 candidate simulator-valid. Với candidate mới, evaluator sort/histogram outcome trong \(O(|\Omega_d|\log|\Omega_d|)\), tính năm quantile và chỉ bucket candidate đó. Ở checkpoint, một tie-group kích thước \(g\) dựng \(G\) với chi phí \(O(g|\Omega_d|\log(g|\Omega_d|))\); tổng member-count của các tie-group không vượt pool size. Không còn thao tác \(O(|pool|\,|\Omega|\,L)\) mỗi iteration: full scenario rebuild trong search bằng 0, provisional-\(G^0\) finalize một lần trước W1, final \(G\) chỉ refresh cho shortlist/tie-groups bị W1 làm đổi trước send. Mọi profile work vẫn bị chặn bởi search/certificate absolute deadlines.

**Calibration protocol trước trận:** chia map/replay theo seed và topology thành train/selection/held-out để không rò các biến thể cùng map. Train chỉ fit scenario/opponent model. Thay vì grid-search liên tục một không gian lớn, pre-register một catalog nhỏ các manifest \((\rho,\delta,\varepsilon,\mathcal Q)\); dùng selection split để chọn và áp one-standard-error rule, tức ưu tiên policy đơn giản hơn nếu chưa có bằng chứng policy phức tạp tốt hơn. \(\tau_d\) và deadline profile được tune trong một latency split/nested pass riêng sau khi risk catalog đã thu hẹp, tránh meta-overfit giữa thái độ rủi ro và tốc độ máy.

Metric ngoài để chọn policy là **score vector thực sự xảy ra cuối replay theo luật BTC**, gồm response time khi ba tier đầu hòa, hoặc win/loss head-to-head suy ra từ vector đó. Metric này độc lập với internal `Key(a)` và các tham số đang tune; tuyệt đối không tự chấm policy bằng chính key của nó. Held-out chỉ mở một lần để promotion: lower confidence bound không kém static baseline, tail coverage không vi phạm gate và invalid rate bằng 0. Nếu fail, giữ baseline thay vì tune lại trên held-out. Manifest gồm version, scenario-class weights, tham số, seed và hash evaluator được đóng băng trước trận. Không có online tuning bí mật.

### 3.4. Viability frontier và quy tắc prune không-trội

Với mỗi phương án ngày hiện tại \(a_d\), ta thu được trạng thái cuối ngày \(x_{d+1}\) và viability envelope:

\[
\mathcal{E}(x_{d+1})=
\{(K,H_2,H_3,risk,witness,LB,UB)\}_{nondominated}
\]

Không còn hard \(K_{cap}\) guard. Một route an toàn \((K_{cap}=M-1,K_{safe}=M-1)\) không bị xóa chỉ vì có route khác \((M,M-2)\).

Với grid outcome ở mục 3.3, đặt \(S_j(a)=\Pr(y_a\ge_{lex}y^j)\). Candidate B chỉ được **xóa chắc chắn** nếu tồn tại candidate A có profile cận dưới đã chứng nhận không kém cận trên hợp lệ của B ở mọi ngưỡng full-score và tốt hơn nghiêm ngặt ít nhất một ngưỡng; nói gọn:

\[
S^{LB}_j(A)\ge S^{UB}_j(B)\ \forall j,
\quad \exists j:S^{LB}_j(A)>S^{UB}_j(B)
\]

`Certified` ở đây chỉ có nghĩa đúng với frozen scenario/weight manifest và các outcome bounds đã qua simulator; nó không tuyên bố đã biết phân phối thật của đối thủ. Nếu một scenario class mới nằm ngoài manifest xuất hiện, không dùng dominance certificate cũ để xóa candidate: mở lại frontier và hạ certificate về bounds-only.

Nếu chỉ profile ước lượng trội, B có thể bị hạ ưu tiên trong beam nhưng không bị xóa khỏi incumbent set. \(K_{cap}\) dùng để sinh route khám phá và báo upside, không một mình quyết định admissibility.

### 3.5. Ba cấp độ kiểm tra viability và ngân sách cứng

Viability là một bài toán covering đa agent có tài nguyên chung, nói chung NP-hard. Vì vậy tuyệt đối không đặt full proof trên critical path mỗi ngày.

**Cấp F - Fast necessary bounds, chạy mỗi ngày**

1. Với mỗi brand chưa lấy, tính spot đại diện và tập agent-day có thể tiếp cận dưới optimistic/pessimistic travel bounds.
2. Tính ngày cuối an toàn và slack.
3. Dùng matching/flow relaxation bỏ bớt đồng bộ tanker và stock timing để tìm cận trên nhanh.
4. Nếu ngay relaxation cũng không đạt một mức \(K\), mức đó chắc chắn bất khả thi; chiều ngược lại không phải certificate.

**Cấp W - Witness repair, ưu tiên dùng mỗi ngày**

1. Lấy witness plan đã lưu từ ngày trước hoặc từ incumbent.
2. Sửa cục bộ theo traffic mới, endpoint mới và route columns hiện có.
3. Chỉ khi toàn bộ route/refuel/stock đã qua simulator mới gắn nhãn `certified LB`.

**Cấp P - Full/strong proof, chỉ khi có slack**

- Chạy CP-SAT master nhiều ngày hoặc branch-and-bound để siết UB, chứng minh optimality hay infeasibility.
- Phù hợp pre-match, sau khi đã gửi đáp án ngày hiện tại, hoặc khi fast solver dừng sớm; không được phép trì hoãn việc tạo incumbent hợp lệ.

`Witness repair` có hai call-site khác mục đích và không được hạch toán trùng:

- **W0 - cache/reservation repair đầu ngày:** nằm trong bucket Fast viability tối đa 10%. Nó chỉ sửa witness cũ đủ để cập nhật bounds, slack và nhãn reservation; không cố certify cả candidate pool.
- **W1 - candidate certification:** nằm trong bucket Certificate repair + double validation 10% ở cuối. Không dùng final `Key` để chọn ai được certify, vì `LB^{cert}` trong key đó phụ thuộc lịch sử repair và sẽ tạo order bias. Mỗi candidate khi vào pool được chạy một **F0 provisional evaluator** tính vào search budget, với cùng fixed operation cap và chỉ được đọc candidate-independent reservations/bounds từ W0; F0 không đọc cached witness/certificate của candidate. Trước W1, mọi candidate được xếp bằng **provisional key đối xứng** \(Key^0\) từ F0, bỏ \(LB^{cert}\) và mọi lợi thế do lịch sử repair. W1 lấy ba phần tử phân biệt nếu có:
  1. `floor leader` = \(\arg\max Key^0\);
  2. `upside challenger` = candidate có valid UB cao nhất còn có thể tạo certified upgrade ở tier sớm nhất; nếu trùng floor leader thì lấy candidate kế tiếp;
  3. incumbent/lần gửi trước.

\[
Key^0(a)=(Q^0_{0.95},Q^0_{0.80},Q^0_{0.50},Q^0_{0.20},Q^0_{0.05},G^0,
scoreToday,terminalSlack,trafficSafety,-stableId)
\]

Trong đó \(Q^0,G^0\) chỉ dùng F0 pessimistic profile với cùng operation cap; không đọc `certified/not-certified`, thời điểm candidate được sinh hay số CPU candidate đã nhận ngoài cap chuẩn.

Sau W1 mới recompute final `Key` bằng certificate mới và chọn gửi. Cách này vừa khai thác floor, vừa cho bounds-only candidate tốt một đường vào certification, nhưng không phải EVoC và không đổi score tier lấy thời gian. W1 không lấy giờ từ W0. Trong bucket này giữ một validation floor tuyệt đối, phần còn lại mới được dùng sửa future witness.

Khi W0 hết ngân sách, solver dùng envelope/witness cache gần nhất và tiếp tục search; nó không chờ proof. Khi W1 hết ngân sách, candidate còn `bounds-only` không được gửi đè. Một incumbent hợp lệ phải được sinh **trước** mọi kiểm tra viability đắt. Full proof không thuộc hai bucket này và chỉ chạy ngoài critical path như mô tả ở cấp P.

Brand chỉ còn một khe agent-day khả thi trở thành **mandatory reservation**, nhưng reservation cũng mang nhãn nguồn: `proven`, `witness-backed` hay `relaxation-only`. ALNS không được phá reservation proven; reservation yếu có thể bị thách thức bởi một witness tốt hơn.

### 3.6. Frontier phải bảo vệ cả tier 2, không chỉ lifetime coverage

Hai terminal state có thể cùng giữ được \(K\) brand lifetime nhưng khác xa tổng daily distinct. Vì vậy mỗi phần tử của \(\mathcal{E}\) lưu cả lịch phân bổ brand theo ngày:

\[
witness = \{B_{d+1},B_{d+2},\ldots,B_D;\ routes;\ refuel\ events\}
\]

Với mỗi mức \(K\), lưu:

- \(LB_2(K)\): daily distinct đã có witness khả thi;
- \(UB_2(K)\): cận trên từ relaxation;
- tương tự \(LB_3(K),UB_3(K)\) sau khi khóa tier 1 và tier 2.

Master không chỉ hỏi “còn đạt \(K\) không?”. Witness tạo full joint outcome \((K,H_2,H_3)\) cho comparator mục 3.3; \(LB_2/UB_2\) và \(LB_3/UB_3\) vẫn được lưu riêng để bound/prune và chẩn đoán gap.

Điều này loại bỏ “ảo tưởng viability”: còn đường lấy đủ brand lifetime nhưng mọi đường như vậy buộc phải dồn brand vào ít ngày và thua tier 2.

Lá chắn dạng frontier vẫn giải quyết lỗi phổ biến của rolling-horizon tham lam - route hôm nay đẹp nhưng đẩy patrol ra xa, cạn fuel hoặc bỏ lỡ brand hiếm - mà không biến worst-case thành mục tiêu tuyệt đối.

---

## 4. Chọn loại agent bằng liệt kê toàn bộ, không dùng “quy tắc vàng”

Loại agent được khóa cả trận, số agent tối đa chỉ là 8. Vì vị trí xuất phát khác nhau, không chỉ số lượng tanker mà **agent nào làm tanker** cũng quan trọng.

### 4.1. Không gian lựa chọn đủ nhỏ

Có tối đa \(2^8=256\) bitmask loại agent. Con số này đủ nhỏ để **quét cận rẻ**, không có nghĩa là được chạy rollout sâu 256 lần.

Quy trình:

1. Tạo ngay cận dưới cho vài seed chắc chắn: all-patrol và các assignment một tanker ở những vị trí xuất phát có centrality cao.
2. Với mọi assignment \(m\), tính cận trên cực rẻ: bỏ traffic xấu, bỏ đồng bộ tanker, dùng reachability/coverage relaxation. Mục tiêu là quét 256 mask trong thời gian gần tuyến tính theo số agent/spot, không gọi multi-day CP-SAT.
3. Loại assignment có cận trên từ điển không vượt cận dưới tốt nhất hiện tại; gom các mask đối xứng/tương đương nếu có.
4. Giữ một beam nhỏ, mặc định 2-3 assignment đầu. Chỉ beam này nhận greedy route portfolio và scenario rollout sâu.
5. Nếu còn thời gian, mở rộng beam; nếu hết thời gian, chọn mask có kế hoạch khả thi đã simulator xác nhận tốt nhất, không chờ đánh giá hết.

Role selection có ngân sách cứng riêng tính từ deadline chọn loại. Một cấu hình mặc định hợp lý là: tối đa 15% quét/prune mask, 70% refine beam và 15% validate/serialize/send; tỷ lệ phải điều chỉnh theo thời gian BTC thật. Không được giả định luôn có một phút hay luôn chỉ có vài giây.

### 4.2. Khi nào tanker thật sự đáng giá?

Tanker chỉ đáng lấy mất một patrol nếu phần khả năng di chuyển/refuel mà nó mở khóa giúp tăng một thành phần score cao hơn phần năng lực thu thập bị mất.

Các mode cần được solver thử song song:

- **Không tanker:** phù hợp khi fuel đủ, spot gần, hoặc số brand lớn cần nhiều collector độc lập.
- **Escort:** tanker và một/nhóm patrol đi cùng corridor; patrol gần như luôn đầy fuel sau mỗi lần di chuyển.
- **Hub tanker:** tanker đứng ở nút giao có nhiều tour quay lại.
- **Relay rendezvous:** tanker nối nhiều cuộc hẹn theo thứ tự thời gian.
- **End-day docking:** tanker chủ yếu tạo trạng thái đầu ngày sau tốt.

Không mode nào luôn tối ưu. Assignment enumeration để dữ liệu trận quyết định.

---

## 5. Bộ máy sinh lộ trình nhiều tầng

### 5.1. Tầng 0 - Simulator sự kiện chính xác

Simulator là “trọng tài cục bộ”, không phải công cụ phụ. Với mỗi step, nó phải tái hiện:

1. Kiểm tra action plan có đúng tổng \(S_d\) cho mọi agent.
2. Tiêu thụ fuel theo ô nguồn khi movement hoàn thành.
3. Phản ánh movement.
4. Thu udon theo spot, visited-per-agent-per-day, stock và thứ tự agent.
5. Nạp fuel khi patrol và tanker cùng ô ở pha nạp.
6. Cộng stay-step vào road footprint.
7. Xuất vị trí/fuel cuối ngày, score ngày và traffic contribution chính xác.

Mọi route column và tổ hợp master đều phải được simulator xác nhận. Không dùng “penalty lớn” cho invalid; invalid bị loại ngay.

### 5.2. Tầng 1 - Đường đi Pareto giữa các điểm quan trọng

Điểm quan trọng gồm:

- vị trí agent hiện tại;
- spot;
- hub/rendezvous candidate;
- ô thường cạnh road bottleneck;
- terminal staging point.

Với mỗi cặp, chạy multi-label Dijkstra/A* trên đồ thị có hướng theo ô nguồn. Một nhãn chứa tối thiểu:

\[
\ell=(cell,time,fuel,criticalRoadFootprint,path)
\]

Giữ top-K đường không trội thay vì một đường ngắn nhất duy nhất. Hai đường cùng time/fuel vẫn có thể khác giá trị vì một đường làm tăng road bottleneck ngày sau.

Đối với tanker, bỏ resource fuel nhưng vẫn giữ time và footprint. Đối với patrol, dùng dominance theo time, fuel và tập road quan trọng.

Footprint chỉ được nén ở **heuristic dominance của label generator**, tuyệt đối không được nén trong route column hay final evaluation. Mỗi path hoàn chỉnh giữ sparse vector đầy đủ:

\[
o^{path}=\{(road\_id,staySteps)\mid staySteps>0\}
\]

cho mọi road đã đi qua. Sau khi ghép route, simulator tái tạo position theo từng step và kiểm tra lại vector này. Nếu một road chưa nằm trong critical set nhưng tổng tải dự báo tiến gần threshold, road đó được “promote” vào critical set và các path liên quan được sinh lại. Nhờ vậy nén nhãn giúp giảm số trạng thái, nhưng không thể che mất một road phụ do chính ta làm tắc.

### 5.3. Tầng 2 - Cột lộ trình của từng agent

Một route column không chỉ là danh sách spot. Nó chứa:

- action sequence đúng \(S_d\);
- position và fuel theo từng step;
- spot/brand/serving thực sự thu được;
- các sự kiện refuel cần thiết \((cell,step)\);
- escort segment nếu có;
- sparse road footprint đầy đủ cho mọi road, kèm trace theo step để kiểm chứng;
- vị trí/fuel cuối ngày;
- giá trị terminal: ở spot nào, gần brand nào, có được dock cuối ngày hay không.

Nguồn sinh column:

- greedy franchise-first có viability;
- DP/label-setting trên tập spot nhỏ;
- insertion/removal local search;
- các operator ALNS chuyên biệt;
- route từ witness plan của ngày trước;
- route contingency đã precompute cho ba trạng thái road.

### 5.4. Tầng 3 - Master problem chọn tổ hợp toàn đội

Dùng CP-SAT hoặc set-packing chọn đúng một column cho mỗi agent. Không nhét toàn bộ thứ tự stock theo từng step vào base model nếu điều đó làm nổ số biến.

Ràng buộc chính:

- mỗi agent chọn một route;
- chỉ patrol mới thu udon;
- base master dùng capacity/score relaxation cho stock, chưa mô hình đầy đủ agent-order ở mọi event;
- nếu patrol column yêu cầu refuel event \(e\), ít nhất một tanker column phải cover đúng cell và step;
- escort segment của patrol phải tương thích với segment của tanker;
- terminal state phải khớp state dùng trong viability witness;
- không có ràng buộc tránh cùng ô, vì luật cho phép co-location;
- master cộng full sparse footprint của các column; critical-road summary chỉ dùng để xếp hạng/sinh column, không dùng làm score giao thông cuối;
- tổ hợp cuối cùng được simulator đội xác minh lại để xử lý chính xác stock tie theo agent ID.

Dùng vòng **lazy conflict cut**:

1. Base master chọn tổ hợp column với refuel/escort compatibility và stock relaxation.
2. Exact simulator chạy toàn đội theo step và trả score thật.
3. Mỗi patrol column \(c\) của agent \(i\) phải khai báo `firstVisit` \(v_{icst}\in\{0,1\}\) và `claimedServing` \(q_{icst}\in\{0,1\}\) cho spot \(s\), step \(t\); tanker column có \(v=q=0\). Một patrol chỉ có tối đa một first-visit event cho mỗi spot/ngày. \(q=1\) nghĩa score/witness coefficient của column đang đòi serving đó, không chỉ đi qua spot.
4. Nếu simulator thấy tổng claimed servings tại \(s\) vượt stock, luôn thêm cut mạnh hợp lệ, không cần “suy ra nếu có thể”:

\[
\sum_{i,c,t} q_{icst}x_{ic}\le stock_s
\tag{CAP(s)}
\]

Tổ hợp vừa mô phỏng vẫn được cache với **exact simulated score** như một incumbent hợp lệ; cut chỉ ngăn master tiếp tục định giá nó bằng score overcount.

5. Nếu một witness cần đúng serving \(e=(i,c,s,t)\), sắp event theo \((step,agentOrder)\) và đặt \(Pred(e)\) là mọi first-visit column-event ở \(s\) đứng trước hoặc bằng \(e\). Chỉ tại hotspot, tạo credit variable \(u_e\le x_{ic}\) và prefix implication:

\[
\sum_{(j,c',t')\in Pred(e)}v_{jc'st'}x_{jc'}
\le stock_s+N(1-u_e)
\tag{PREFIX(e)}
\]

Score/witness chỉ được dùng \(u_e\), không dùng optimistic constant của column. Khi \(u_e=1\), bất đẳng thức chứng minh event đó còn stock theo đúng thứ tự xử lý; khi \(u_e=0\), route vẫn có thể được chọn nhưng không được credit serving. Sau khi spot được promote, tắt các \(q x\) score coefficients tại spot đó và thay CAP bằng \(\sum_e u_e\le stock_s\); nếu không, route không-được-serving sẽ bị loại oan dù action vẫn hợp lệ.

6. `repeatCount(s)` tăng khi simulator tìm một **prefix violation mới chưa có cut** tại spot đó. Candidate manifest dùng mặc định \(h_{promote}=2\): sau hai violation khác nhau, promote toàn bộ first-visit/credit events của spot \(s\) trong ngày hiện tại sang mô hình PREFIX cục bộ. Counter reset mỗi ngày; không đếm lại cùng một violated prefix.
7. Mỗi master solve có hard cap \(R_{cut}=\min(8,2N)\) resolve rounds ngoài wall-clock deadline. Combination no-good chỉ dùng cho mismatch đa tài nguyên mà CAP/PREFIX không biểu diễn soundly và phải log reason code; stock overcount thuần túy không được fallback ngay về no-good. Hết cap thì trả simulator-valid incumbent.

Ngay từ column generation, pricing hạ ưu tiên các arrival đồng thời tại spot stock thấp, hoặc thử biến thể lệch step, nhưng không cấm chúng nếu có lợi tier cao. CAP là aggregate cut rẻ; PREFIX chỉ mở ở event cần certificate/hotspot lặp, nên đây không phải exact b-matching toàn master. Cut count, repeated-prefix count, hotspot promotions và thời gian resolve phải được log để benchmark; \(h_{promote},R_{cut}\) được calibrate offline rồi freeze trong manifest.

### 5.5. Tối ưu từ điển trong master

Không dùng một weighted sum tùy ý. **Bên trong mỗi scenario/witness deterministic**, giải tuần tự:

1. Maximize số brand lifetime đạt được trong horizon; ghi optimum \(z_1^*\).
2. Thêm ràng buộc \(z_1=z_1^*\), maximize tổng daily distinct; ghi \(z_2^*\).
3. Thêm \(z_2=z_2^*\), maximize servings.
4. Khi ba mục tiêu bằng nhau, maximize terminal slack, giảm road risk và giảm thời gian solver.

**Giữa các scenario**, master xuất viability envelope; comparator mục 3.3 áp confidence gate rồi downside-first joint-outcome quantile key. Terminal slack không được dùng để vượt qua một candidate có certified score/risk profile tốt hơn.

Nếu dùng Big-M để tăng tốc, M phải được suy ra từ cận chắc chắn:

- tổng daily distinct không quá \(D\cdot M_{brand}\);
- tổng servings không quá \(D\sum_s stock_s\).

Tuy vậy, solve tuần tự dễ kiểm chứng và tránh lỗi overflow/scale hơn.

---

## 6. Rolling horizon đúng nghĩa dưới bất định giao thông

### 6.1. Tải road là biến ẩn

Server công bố status 0/1/2, không công bố traffic volume chính xác. Status ngày \(d\) chỉ cho một ràng buộc khoảng lên tổng stay-step hai ngày trước sau khi chia số đội.

Với road \(r\):

\[
v_{r,d} = \frac{1}{P}\sum_{team}\left(o_{r,d-1}^{team}+o_{r,d-2}^{team}\right)
\]

Quan sát status cho biết:

- smooth: \(0\le v<b\);
- busy: \(b\le v<j\);
- jammed: \(v\ge j\).

Ta biết chính xác contribution của mình, nhưng không biết đường đi đầy đủ của đối thủ. Vì vậy phải duy trì interval/posterior cho phần còn lại.

### 6.2. Sinh kịch bản đối thủ

Nếu API cho vị trí/fuel các agent khác ở đầu mỗi ngày:

1. Sinh các đường khả thi nối endpoint ngày trước - ngày sau.
2. Loại đường mâu thuẫn với loại agent, budget step và fuel quan sát được; lưu ý refuel khiến fuel chênh lệch chỉ là bằng chứng yếu.
3. Ưu tiên các đường ngắn/Pareto hợp lý, nhưng luôn giữ kịch bản biên có tải cao ở bottleneck.
4. Lọc tập đường bằng status road vừa quan sát.
5. Lấy mẫu vài tổ hợp đại diện cho smooth-edge, busy-edge và jam-edge.

Ngoài hành vi route hợp lý, sinh **bounded-rational adversarial scenarios**, không giả định đối thủ toàn năng:

1. Trước tiên xác định `public-obvious roads` chỉ từ thông tin mọi đội đều có và tính rất rẻ: articulation/cut road, betweenness cao giữa start-spots, cửa ngõ duy nhất tới một cụm brand, hoặc corridor nhiều đội đã lặp lại ở endpoint history.
2. Chỉ với các road công khai-hiển nhiên này, tính agent đối thủ nào đến được và stay-step tối đa trong budget. Không giả định họ giải ngược master/route riêng của ta.
3. Cộng contribution tối đa sau khi chia số đội; chỉ giữ scenario nếu nó có thể đổi status hoặc làm mất slack đáng kể.
4. Với trận 2-3 đội, giữ nhiều scenario hơn; với nhiều đội, prune trường hợp contribution một đội không đủ dịch threshold.

`Public-obvious` có cả thành phần động: một corridor được nâng `exposed` nếu endpoint công khai của ta trong ít nhất hai ngày liên tiếp chỉ tương thích với một nhóm đường hẹp, hoặc lịch sử status công khai liên tục làm posterior của corridor đó tăng. Đây chính là khả năng đối thủ học hành vi qua nhiều ngày. Việc promote phải tái tạo được chỉ từ public observation và một inference budget nhỏ; không được đọc route thật/private dependency của solver để giả làm kiến thức đối thủ.

Road mà chỉ pipeline nội bộ của ta mới biết là “phụ thuộc mạnh” được đưa vào **sensitivity test exogenous**, không gắn nhãn “đối thủ cố ý đoán đúng” và không dùng để đặt xác suất thắng. Omniscient-opponent scenario chỉ dùng offline để stress kiến trúc, không đi vào production risk gate.

Không cần đoán chính xác “đối thủ sẽ làm gì”. Cần hành vi hợp lý để ước lượng probability, public-adversarial scenario để đo \(K_{safe}\), và tách riêng private-route sensitivity để tránh tự dựng một đối thủ mạnh phi thực tế.

### 6.3. Bài toán hai tầng có recourse - phạm vi đúng của non-anticipativity

Uncertainty là **một phía**:

- Với một candidate action \(a_d\), contribution giao thông của chính ta là deterministic và được simulator biết chính xác.
- Contribution của đối thủ trong ngày \(d\) là biến ngoại sinh chưa quan sát khi ta nộp action.
- Status ngày \(d+1\) là kết quả của hai phần trên cùng memory ngày \(d-1\).

Khi tối ưu chung nhiều kịch bản, \(a_d\) vẫn là biến quyết định và phải dùng cùng một giá trị trước khi biết phần đối thủ. Đó chính là ràng buộc non-anticipativity; nó không có nghĩa contribution của ta là ngẫu nhiên. Sau khi chọn một candidate, full own footprint được cộng **giống hệt** vào mọi nhánh. Từ ngày \(d+1\), policy recourse được phép khác nhau sau khi server công bố status thật.

Vì vậy mô hình đúng hơn là **two-stage robust/stochastic optimization with recourse**, không phải cây bất định đối xứng cho cả ta và đối thủ. Nếu không có probability model đáng tin, dùng interval/boundary scenarios; nếu có dữ liệu replay, thêm trọng số xác suất để xếp hạng mà vẫn giữ adversarial stress test.

Độ sâu thực dụng:

- mô phỏng chi tiết ngày hiện tại;
- tập recourse scenario chi tiết thêm 1-2 ngày, đúng bằng memory traffic hai ngày;
- phần còn lại dùng terminal value và viability matching.

Cách này rẻ hơn giải joint time-expanded graph cho toàn bộ 10 ngày, nhưng đúng bản chất thông tin hơn một dự báo duy nhất.

### 6.4. Traffic manipulation chỉ là tùy chọn có kiểm chứng

Chủ động làm tăng traffic chỉ được phép khi một kiểm tra counterfactual cho thấy:

1. Wait đó là step đệm bắt buộc hoặc không làm giảm score hiện tại;
2. ta có đường thay thế chắc chắn trong hai ngày ảnh hưởng;
3. đối thủ có xác suất cao phụ thuộc road đó;
4. contribution thêm đủ khả năng đẩy qua threshold sau khi chia cho số đội;
5. worst-case score của ta không giảm.

Nếu thiếu một điều kiện, Wait ở ô thường/hub/spot có terminal value cao. Traffic poisoning không phải chiến lược mặc định.

---

## 7. Các operator chiến thuật dành riêng cho HEXUDON

ALNS vẫn hữu ích, nhưng operator phải tôn trọng score từ điển và viability shield.

### 7.1. Rare-brand rescue

Tìm brand có slack nhỏ nhất hoặc ngày cuối an toàn sớm nhất; xóa một đoạn route ít quan trọng và chèn spot đại diện của brand đó. Chỉ so sánh servings sau khi lifetime coverage được giữ.

### 7.2. Overnight-harvest insertion

Biến terminal cell thành spot có giá trị đầu ngày sau, nhưng future witness bắt buộc phải dành `WAIT(1)` trước khi rời spot. Công thức nhanh sau chỉ là **cận trên heuristic**, không phải score được phép cộng trực tiếp vào master:

\[
UB_{overnight}(s,k)=
\Delta brand_{next}+\min(k,stock_s) + V_{position}(s)
\]

Score thật của ngày sau phải lấy từ witness plan và simulator:

- Mỗi patrol chỉ nhận tối đa một serving tại spot \(s\) trong cả ngày, dù đứng thêm, rời rồi quay lại hay đi qua nhiều lần.
- Diversity của brand là một biến OR theo toàn đội/ngày, chỉ được đếm một lần dù overnight patrol và route khác cùng lấy.
- Serving tại step 1 chỉ xuất hiện khi overnight patrol hoàn thành `WAIT(1)` tại spot. Nó phụ thuộc stock và thứ tự agent nếu patrol khác cũng hoàn thành action tại spot đúng step đó; simulator xử lý sự kiện đồng thời, không mặc định overnight group luôn lấy đủ \(\min(k,stock_s)\).
- Không cộng \(\Delta brand_{next}\) chỉ vì terminal cell là spot. Chỉ ghi nhận diversity khi một future witness cụ thể xác nhận brand đó chưa được route khác tính trùng. Nếu chưa có witness, overnight chỉ tạo feature/cận trên, không tạo certified score.

Trong master nhiều ngày, mô hình trực tiếp biến \(visit_{i,s,d+1}\), \(served_{i,s,d+1}\) và \(brandSeen_{b,d+1}\). Trong master một ngày, **không cộng bất kỳ next-day brand hay serving nào vào official-score key**. Overnight chỉ là terminal feature dùng sau khi score/profile hiện tại bằng nhau. Chỉ khi witness repair dựng được lịch ngày sau và simulator xác nhận, phần overnight mới được promote thành \(LB_2/LB_3\) certified. Nhờ vậy một UB overnight sai không thể làm master chốt terminal state như thể đã có diversity thật.

### 7.3. End-step dock

Dịch timeline của patrol và tanker để cùng terminal cell khi ngày kết thúc. Co-location liên tục từ step áp chót vẫn mạnh hơn vì có thể nạp fuel ngay trong ngày; tuy nhiên BTC xác nhận cùng đến ở đúng step cuối cũng làm patrol bắt đầu ngày sau đầy fuel. Exact simulator phải áp dụng refill boundary sau khi đã xác nhận mọi movement và score ngày hiện tại, để fuel đó không thể cứu một action vốn thiếu nhiên liệu.

### 7.4. Escort merge/split

Ghép patrol với tanker trên đoạn road nhanh hoặc đoạn fuel đắt, sau đó tách tại hub. Operator phải kiểm tra patrol đủ fuel trả cho lần rời ô trước khi được nạp lại.

### 7.5. Stock-aware multi-visit

Khác Voronoi cứng, operator này chủ động đưa nhiều patrol đến cùng spot nếu stock > 1 và servings tier đang được tối ưu. Khi stock nhỏ, tránh đến đồng thời vô ích hoặc phân quyền cho agent ID phù hợp. Event simulator lưu `visited[i][spot][day]`; quay lại cùng spot trong ngày không được tạo serving mới. Khi nhiều event thu xảy ra cùng step, xử lý theo thứ tự agent trong map đúng Q&A Q26.

### 7.6. Critical-road bypass

Thay đoạn qua road có xác suất đổi trạng thái cao bằng một đường Pareto khác. Không tự động tránh mọi road tắc: road tắc vẫn chỉ tốn 2 fuel và có thể hữu ích cho tanker hoặc khi đường vòng qua núi quá dài.

### 7.7. Terminal-hub shift

Khi hai route bằng score hôm nay, chọn endpoint có minimax distance tốt hơn đến các brand chưa chắc chắn của ngày sau, hoặc có nhiều contingency route hơn.

### 7.8. Viability repair

Nếu một mutation làm mất witness của brand bắt buộc, operator tự động chèn một staging/refuel/end-position mới hoặc rollback. Không cho simulated annealing “chấp nhận tạm” một nghiệm phá hỏng tầng score cao hơn.

---

## 8. Quy trình ra quyết định mỗi ngày

### 8.1. Trước trận

1. Parse map, spot, stock, brand, daySteps, fuel limit, thresholds, số đội.
2. Xây even-r adjacency và test biên map.
3. Tính spot representatives, bottleneck road, hub và khoảng cách Pareto cơ sở.
4. Liệt kê assignment loại agent; dùng bounds để chọn assignment tốt nhất.
5. Precompute route skeleton và backward reachability cho brand.
6. Khởi tạo simulator và chạy toàn bộ test vàng.

### 8.2. Đầu ngày \(d\)

1. Đọc state, đối chiếu vị trí/fuel với dự đoán simulator ngày trước.
2. Cập nhật tập brand đã lấy, daily sum và stock reset.
3. Cập nhật belief traffic bằng status mới và endpoint đối thủ.
4. Sinh ngay một incumbent greedy hợp lệ, simulator xác nhận, trước khi chạy viability đắt.
5. Trong hard budget riêng, cập nhật fast viability bounds, \(K_{cap}/K_{safe}/C_\rho\), joint-outcome quantiles, brand slack và mandatory reservations; sửa witness cache nếu kịp.
6. Sinh/cải tiến route columns song song; giải master theo viability envelope và score từ điển có điều kiện.
7. Tính \(Key^0\) không dùng certificate history; đưa provisional floor leader, distinct earliest-tier/max-valid-UB challenger và incumbent/lần gửi trước vào W1 queue. Candidate-specific witness repair và simulator độc lập cuối cùng chỉ tiêu bucket Certificate/validation, không quay lại bucket W0.
8. Theo dõi certified bounds và absolute deadline; tốc độ cải thiện gần đây chỉ phân bổ lại CPU, không tự quyết định dừng.

### 8.3. Chính sách gửi đáp án

Phải phân biệt **bảo hiểm timeout**, **giá trị của việc tính tiếp** và **tie-break response time**. Luật dùng tổng lũy kế:

\[
R_{total}=\sum_{d=1}^{D}R_d
\]

nên thời gian đã tiêu ở ngày trước là chi phí vĩnh viễn nếu cuối trận hòa ba tier đầu. Tuy nhiên, không được đổi một cải thiện chắc chắn ở tier 1-3 lấy response time, vì luật vẫn là từ điển.

Không dùng EVoC trong production path. EVoC đòi hỏi quy đổi một xác suất cải thiện tier cao với giây response, trong khi luật không cho một tỉ giá như vậy và hệ thống chưa có mô hình cải thiện theo thời gian đã calibrate.

Thay vào đó dùng **lex-consistent search schedule**. Trước trận, benchmark xác định \(\tau_d\) cho từng lớp độ khó map/ngày; \(\tau_d\) là thời điểm search mềm, luôn nhỏ hơn hard network deadline. Trong ngày, tiếp tục search tới \(\tau_d\), trừ khi xảy ra một trong các điều kiện chứng minh:

1. LB và UB full-horizon của ba tier đầu đã khít.
2. Không còn active column/branch nào có certified UB trội incumbent.

Tốc độ cải thiện gần 0 chỉ dùng để chuyển CPU giữa column generator/CP-SAT/witness repair, không tự nó là lý do đánh đổi một cơ hội tier cao lấy tier 4. Khi tới \(\tau_d\), dừng theo policy đã calibrate offline và gửi incumbent; đây là tham số cạnh tranh được thừa nhận công khai, không phải “giá trị kỳ vọng” giả chính xác.

Trước hard safety margin, gửi incumbent hợp lệ nếu chưa gửi gì. Nếu ba tier full-horizon đã khóa và phần còn lại chỉ cải thiện terminal heuristic chưa chứng nhận, gửi ngay để giữ \(R_d\).

Sau mỗi lần gửi, lưu envelope \([LB_{sent},UB_{sent}]\), witness và exact score ngày hiện tại. Production bot là **certified-only resend**:

- gửi đè nếu \(LB_{new}\) trội từ điển hơn \(UB_{sent}\) tại tier đầu tiên khác nhau; hoặc exact score ngày hiện tại tốt hơn và future envelope được chứng minh không kém ở mọi tier cao hơn;
- một terminal heuristic lạc quan, \(K_{cap}\) cao hơn hay UB tốt hơn không phải certificate;
- nếu không đủ thời gian sửa/kiểm chứng witness, giữ đáp án cũ.

`Risk-mode resend` **không có trong competition build**. Research build chỉ được phép thử nó khi đồng thời có: telemetry score đối thủ đủ dùng, head-to-head replay độc lập, probability calibration đạt ngưỡng đã định trước, và lower confidence bound của win-rate improvement vượt ngưỡng triển khai. Nếu sau này qua đủ gate, nó phải trở thành một policy version mới được review/freeze trước trận, không phải runtime switch. Thiếu một điều kiện thì không có ngoại lệ. Polling vẫn tuân thủ mức tối đa công bố.

Sau khi đã commit đáp án hiện tại, dùng phần thời gian còn lại trước khi ngày kế tiếp xuất hiện để precompute contingency cho status road ngày sau. Không giả định có khoảng nghỉ riêng giữa hai ngày.

---

## 9. Pseudocode cấp cao

```text
PREMATCH(config):
    graph      <- build_even_r_directed_cost_graph(config)
    simulator  <- exact_step_simulator(config)
    assignments <- all_role_bitmasks(N) excluding no-patrol

    best <- feasible_seed(all_patrol, best_single_tanker_candidates)
    for m in assignments:
        UB[m] <- cheap_role_coverage_relaxation(m)     # hard budget: stage A
        if UB[m] lex<= feasible_lower_bound(best): continue
        beam.push(m, UB[m])

    for m in beam.top(B = 2..3) while before role_soft_deadline:
        plan[m] <- fast_multi_day_scenario_rollout(m)  # only shortlist
        if simulator.valid(plan[m]) and plan[m] lex> best: best <- plan[m]
    submit_roles(best.assignment)

SOLVE_DAY(state, endsAt):
    assert state_matches_previous_simulation_or_reconcile()
    budget_profile <- frozen_deadline_class(endsAt - now)
    if budget_profile == emergency:
        fallback <- WAIT_at_current_cell_for_exactly_Sd_each_agent(state)
        return validate_serialize_and_submit(fallback)

    belief <- update_hidden_traffic_belief(state.traffics, opponents)
    scenarios <- freeze_common_scenario_manifest(
        build_likely_and_reachable_adversarial_scenarios(belief)
    )

    incumbent <- valid_greedy_plan(state)              # create first
    assert simulator.valid(incumbent)

    envelope, reservations <- W0_fast_bounds_and_cache_repair(
        state, scenarios,
        budget = budget_profile.W0,
        cached_witnesses
    )

    columns <- seed_route_portfolio(incumbent, cached_contingencies)
    incumbent.profile0 <- F0_fixed_cap_pessimistic_profile(incumbent, scenarios)
    pool <- {incumbent}
    while now < soft_deadline:
        columns <- pareto_label_generation(columns)
        columns <- shielded_ALNS(columns, reservations)
        candidate <- risk_aware_lexicographic_master(columns, scenarios, envelope)

        if simulator.valid(candidate)
           and candidate not dominated by viability frontier:
               profile0 <- F0_fixed_cap_pessimistic_profile(candidate, scenarios)
               pool.add(candidate with exact_today_score, bounds_only, profile0)
               bucket_by_five_quantiles_and_mark_tie_group_dirty(candidate, pool)

        if all_active_full_score_bounds_tight(pool)
           or no_active_certified_UB_can_enter_or_beat_coarse_frontier(pool)
           or now >= calibrated_search_deadline(tau_d):
               break

    finalize_G0_for_all_dirty_quantile_tie_groups(pool)
    floor_leader <- argmax(provisional_Key0_without_certificate_history, pool)
    upside <- optional_argmax(earliest_upgrade_tier_then_valid_UB, pool excluding floor_leader)
    shortlist <- {floor_leader, upside, last_sent_or_incumbent}
    W1_repair_future_witnesses(shortlist, budget_profile.W1_minus_validation_floor)
    independently_validate_send_candidates(budget_profile.validation_floor)
    update_final_profiles_and_affected_G_tie_groups(shortlist, pool, scenarios)
    send_candidate <- select_send_safe_candidate(shortlist, last_sent, certified_only_resend)
    submit_policy(send_candidate, submission_certificate(send_candidate), response_ledger, endsAt)
    cache_next_day_contingencies(send_candidate, scenarios)
```

---

## 10. Ví dụ chiến thuật minh họa

Giả sử còn 3 ngày, có 4 patrol và 1 tanker. Brand X chỉ có một spot xa ở phía đông; các brand A/B/C có nhiều spot gần trung tâm. Hôm nay traffic phía đông còn thông, nhưng tập recourse scenario cho thấy road có thể tắc từ ngày sau.

Một solver single-day có thể cho 4 patrol vét A/B/C, thu nhiều servings và kết thúc ở trung tâm. UDON-SHIELD làm khác:

1. Backward reachability phát hiện X chỉ còn khả thi chắc chắn hôm nay; X trở thành mandatory reservation.
2. Một patrol đi X, tanker escort trên corridor fuel đắt.
3. Hai patrol lấy A/B/C; patrol còn lại vét spot stock cao.
4. Patrol đi X và tanker rendezvous ở spot X trước step cuối, giữ nguyên co-location trọn bước cuối để patrol được nạp đầy.
5. Sang ngày sau, patrol đang ở spot X phát `WAIT(1)`, nhận serving khi wait hoàn thành ở step 1 rồi mới rời đi; nếu X đã tính daily diversity ở route khác thì serving vẫn có giá trị tầng 3.
6. Các patrol trung tâm kết thúc ở hai spot khác brand để mở hai cơ hội thu đầu ngày sau với chi phí một step mỗi patrol, không coi đó là score miễn phí.

Hôm nay có thể ít servings hơn baseline, nhưng số brand lifetime tối đa được bảo đảm. Sau khi tier 1 được khóa, solver mới chọn cách kết thúc ở spot và dock tanker để phục hồi tier 2/tier 3.

Nếu backward reachability chỉ nói “đi X hôm nay có xác suất cao nhất” chứ không chứng minh đây là cửa sổ cuối, X không trở thành hard mandatory. Frontier giữ cả route rủi ro đi X và route an toàn staging gần X; so \(K_{cap}\), survival profile và witness tier 2. Đây là khác biệt thực tế giữa `proven reservation` và `relaxation-only urgency`.

---

## 11. Ngân sách tính toán và tính khả thi triển khai

Không gian joint action toàn trận là khổng lồ, nhưng giới hạn thực tế rất thuận lợi cho decomposition:

- map tối đa 1024 ô;
- agent tối đa 8;
- ngày tối đa 10;
- step/ngày bị chặn theo kích thước map;
- vai trò chỉ có tối đa 256 assignment;
- spot/brand là tập điểm quan trọng nhỏ hơn nhiều so với toàn map.

Phải tách hai đồng hồ độc lập:

- \(T_{role}\): cửa sổ chọn loại agent trước trận, chỉ chạy một lần; dùng ngân sách 15/70/15 ở mục 4.1.
- \(T_d\): cửa sổ action của ngày \(d\), chạy lại mỗi ngày; dùng bảng dưới đây.

Phân bổ tính toán trong trận:

1. **Offline/pre-match:** graph geometry, route skeleton, reachability, bottleneck analysis và strong viability proof nếu có thời gian.
2. **Đầu ngày:** cập nhật trọng số road, belief và Pareto paths bị ảnh hưởng.
3. **Anytime search:** luôn giữ một incumbent đã simulator xác minh.
4. **Master nhỏ:** chỉ chọn trên portfolio cột tốt, không bung toàn bộ \(cell\times step\times fuel\times agent\times day\) thành một mô hình khổng lồ.
5. **Song song hợp lý:** mỗi worker sinh route columns/scenario rollout độc lập; master và simulator quyết định cuối cùng theo một chuẩn duy nhất.

Gọi \(t^{recv}_d\) là lúc nhận state và \(T_d=endsAt-t^{recv}_d\) là toàn bộ cửa sổ wall-clock của ngày. Network reserve nằm ngay trong bảng, không bị trừ hai lần. Profile `normal-v1` chốt đúng 100% như sau:

| Hạng mục | Ngân sách gợi ý | Điều kiện bắt buộc |
|---|---:|---|
| Parse + reconcile + greedy incumbent | 10% | Phải hoàn tất trước mọi proof đắt |
| Fast viability bounds + W0 cache/reservation repair | tối đa 10% | Hết giờ thì dùng cache, không chờ |
| Column generation + master + ALNS | 60% | Chạy tới \(\tau_d\) hoặc khi bound đã khít |
| W1 provisional-shortlist certification + double validation | 10% | Giữ validation floor; không gửi route chưa validate |
| Serialize/network safety margin | 10% | Hard reserve, không cho solver chiếm |

Tỷ lệ thuần phần trăm không đủ cho deadline cực ngắn. Trên đúng hardware/runtime thi đấu, đo p99 của ba floor không thể cắt: \(t_{seed}^{min}\) cho parse + instantiate safe incumbent từ state hiện tại, \(t_{val}^{min}\) cho independent validation, và \(t_{net}^{min}\) cho serialize/send. Manifest có các deadline class `emergency/short/normal/long`, chọn **một lần ngay khi nhận \(T_d\)** chỉ từ deadline/config công khai; mỗi profile tự cộng đúng 100% và bất biến trong ngày.

Runtime HTTP phải kiểm tra thêm freshness của ACK: nếu state wire là `d`, ACK hợp lệ phải khai báo ngày `d+1`. ACK của ngày lớn hơn chứng minh submission đã vượt biên ngày và có thể bị áp lên state khác, nên không được cập nhật ledger hay tiếp tục planning. Trong giai đoạn chưa đủ mẫu p99, implementation dùng manifest bảo thủ `btc-http-observed-safe-v1` với network floor `1500 ms` và cấm gửi khi cửa sổ còn dưới `1000 ms`; đây là safety floor tạm thời cần tiếp tục đo trên BTC, không phải tham số tối ưu hóa score.

- Nếu \(T_d\) đủ cho `normal-v1`, dùng bảng trên.
- Nếu \(T_d\) chỉ đủ các floor, vào `emergency`: parse state, tạo đúng một core `WAIT(S_d)` tại current cell cho mỗi agent, một validation pass, serialize/send; W0, column search và W1 challenger certification nhận 0. Không tái dùng mù một route của ngày trước.
- Nếu \(T_d<t_{seed}^{min}+t_{val}^{min}+t_{net}^{min}\) ở p99, build chưa đạt điều kiện thời gian tối thiểu và không được coi là competition-ready; không chữa bằng cách bỏ validation.

Không để implementer tự định nghĩa “legal filler”. Core emergency contract duy nhất là:

```text
EMERGENCY_FALLBACK(dayState, S_d):
    require dayState contains a current cell for every configured agent
    for agent i in official agent-list order:
        plan[i] <- WAIT(cell = dayState.position[i], duration = S_d)
        assert sum(action.duration for action in plan[i]) == S_d
        assert every action in plan[i] is WAIT at dayState.position[i]
    return plan
```

Luật chính thức cho phép wait tại ô hiện tại trong một số step chỉ định; wait không phải movement nên không đi vào pond, không tiêu fuel và không cần đủ movement fuel. Mỗi ngày server cung cấp position đầu ngày bằng endpoint ngày trước; movement thiếu step ở cuối ngày là invalid và phải đổi thành wait, nên core state không có “lệnh di chuyển dở dang” xuyên ngày. Nếu wire format có duration, serializer phát một `WAIT(S_d)` mỗi agent, chi phí logic \(O(N)\); nếu format yêu cầu per-step token/cell, adapter mới expand thành \(S_d\) phần tử, chi phí output \(O(NS_d)\). Cả hai phải round-trip về cùng core plan và qua exact simulator + independent validator. Không có adapter test chứng minh WAIT encoding thì emergency profile bị disable và build fail competition-readiness từ startup, không đoán format lúc runtime.

Mọi mốc được chuyển thành absolute deadlines ngay khi nhận state. Phần thời gian một tầng dùng thiếu có thể trả về search pool theo profile, nhưng không được lấn validation floor hay network reserve. Các profile và ngưỡng class được benchmark offline rồi freeze; không đổi online chỉ vì solver đang chậm. Full proof chỉ chạy sau khi đã có incumbent/send-safe plan.

CP-SAT phù hợp cho master số nguyên và đồng bộ sự kiện; label-setting phù hợp cho đường đi đa tài nguyên; ALNS phù hợp để mở rộng portfolio. Không công cụ nào phải gánh toàn bài toán.

---

## 12. Kiểm thử bắt buộc

### 12.1. Golden tests từ tài liệu chính thức

- even-r: hàng chẵn lệch phải, map không torus;
- chi phí theo ô nguồn;
- action plan phải đúng tổng step;
- bắt đầu ngày trên spot không tự thu; `WAIT(1)` thu ở step 1 còn movement ngay không thu spot nguồn;
- nhiều patrol đến spot cùng lúc, stock phân theo agent order;
- gặp nhau tức thời không refuel; đồng vị trí trọn một step mới nạp;
- tanker và patrol lockstep từ trạng thái đã đồng vị trí vẫn refuel;
- tanker đi trước một ô không refuel patrol phía sau;
- mới gặp ở step cuối không được nạp; cùng ô từ step áp chót qua step cuối mới được nạp;
- traffic đếm ô sau movement, có step cuối, không có step 0;
- invalid của một agent làm reject toàn plan.

### 12.2. Property tests

- fuel không bao giờ âm;
- mỗi serving làm stock giảm đúng 1 và một patrol không lấy cùng spot hai lần/ngày;
- stock reset giữa ngày nhưng visited-per-day cũng reset;
- tổng duration mỗi agent đúng \(S_d\);
- serialize/deserialize không đổi action semantics;
- simulator chạy lại cùng state/action cho kết quả deterministic;
- mọi route do solver gửi đều qua một validator độc lập thứ hai.

### 12.3. Tests cho các sửa đổi sau phản biện

- fast viability bị timeout vẫn phải trả incumbent hợp lệ và không vượt hard cap;
- một candidate có \(K_{safe}=M-1\) nhưng xác suất cao đạt \(K_{cap}=M\) không bị hard shield loại chỉ vì worst-case;
- hai state cùng coverage nhưng witness tier 2 khác nhau phải được xếp đúng theo \(LB_2(K)\);
- overnight diversity không được đếm trùng khi route khác lấy cùng brand ngày sau;
- patrol overnight, patrol đến cùng step và stock thiếu phải phân serving theo agent order;
- full sparse footprint từ route column phải bằng footprint simulator cho mọi road;
- road phụ bị tải gần threshold phải được promote vào critical set;
- adversarial opponent dwell scenario chỉ được tạo khi đối thủ thật sự đến được road và có đủ step để ảnh hưởng status;
- với cùng public state, đổi route candidate bí mật của ta không được làm đổi tập bounded-rational opponent scenarios; private-route sensitivity phải nằm ở channel riêng;
- non-anticipativity: own action/footprint giống nhau giữa các nhánh, future recourse được khác sau khi quan sát status;
- confidence gate luôn không rỗng; nếu mọi candidate đều dưới mức tuyệt đối mong muốn thì candidate tốt nhất theo \(C_\rho\) vẫn được giữ;
- profile cắt nhau theo fixture \(A:95\%\,(M-1),5\%\,M\) và \(B:80\%\,M,20\%\,(M-2)\) phải được phân giải đúng bằng downside-first quantile ladder đã khai báo, không bằng thứ tự input;
- \(\Omega_d\) phải bất biến suốt anytime search; thêm candidate chỉ tính local quantiles, bucket/mark dirty, không rebuild \(G\) trong inner loop; hai checkpoint finalize phải cho kết quả bit-for-bit bằng full-rebuild oracle;
- với pool 32 và scenario manifest lớn nhất đã benchmark, candidate insertion phải nằm dưới hard latency counter; số full-manifest rebuild trong search bằng 0, provisional-\(G^0\) checkpoint bằng 1 và final affected-group checkpoint bằng 1;
- hai profile bằng nhau tại năm quantile nhưng khác distribution phải chỉ được phân xử bằng \(G\) sau khi ladder hòa; test riêng chứng minh bỏ ladder và dùng \(G\) trực tiếp tạo một policy khác;
- comparator phải cho đúng một kết quả ổn định với mọi cặp profile cắt nhau, kể cả khi input order hoặc số worker thay đổi;
- nhân đôi số sample trong cùng một scenario class không được làm đổi class probability hay quyết định; perturbation nhỏ hơn \(\varepsilon\) không được lật thứ tự;
- learned scenario manifest thiếu effective coverage phải fail closed sang class-weight fallback; thiếu class bắt buộc phải nhận pessimistic mass và tắt \(G\), không được bỏ class;
- hai route hòa mọi score/slack nhưng khác self-induced threshold crossing phải được phân xử bằng `trafficSafety` trước `stableId`;
- ép fixture hash collision phải vẫn phân xử ổn định bằng canonical action bytes;
- candidate an toàn \(K_{safe}=M-1\) không bị xóa chỉ vì candidate khác có \(K_{cap}=M\); chỉ certified lower-profile dominance mới được phép prune;
- các tham số rủi ro \(\rho,\delta,\varepsilon\) không tự đổi trong trận và cùng một replay phải cho cùng quyết định;
- calibration phải xếp policy bằng realized official score vector độc lập với internal `Key`; thay `Key` mà giữ action/outcome replay không được làm metric ngoài đổi;
- W0 và W1 phải bị charge đúng hai bucket riêng; W1 shortlist phải gồm \(Key^0\) floor leader, distinct earliest-tier/max-valid-UB challenger và incumbent/lần gửi trước, giữ validation floor và không được mượn ngược giờ W0;
- shuffle candidate generation/certification history nhưng giữ F0 profiles phải cho cùng W1 shortlist; cached \(LB^{cert}\) không được làm đổi \(Key^0\);
- master stock overcount phải sinh đúng \(CAP(s)\); required-serving order conflict phải sinh \(PREFIX(e)\) có predecessor set theo \((step,agentOrder)\);
- violation prefix mới thứ hai tại cùng spot phải promote local exact hotspot; cùng prefix lặp lại không tăng counter; resolve rounds không vượt \(R_{cut}=\min(8,2N)\);
- sau hotspot promotion, route được chọn với \(u_e=0\) phải vẫn hợp lệ nhưng không nhận serving; \(q x\) optimistic coefficient cũ phải bị tắt và \(\sum u_e\le stock_s\);
- stress map nhiều patrol cùng tranh spot stock thấp phải log bounded cut rounds; overcount-only combination vẫn được giữ với exact score thay vì bị gắn invalid;
- cùng public history phải sinh cùng exposure scenarios dù private route cache khác; corridor lặp qua endpoint history được promote, private-only dependency thì không;
- one-day master không được cộng brand/serving tương lai vào score chính thức; overnight chỉ là terminal feature cho tới khi có future witness;
- resend policy production không coi UB, terminal heuristic hay estimated win probability là certified improvement;
- competition build/config schema không expose risk-mode switch; replay cố bật option lạ phải fail closed;
- response ledger bằng đúng tổng thời gian của lần gửi hợp lệ cuối mỗi ngày;
- mọi normal/short/long profile phải cộng đúng 100%, không lẫn với pre-match role budget và không xâm lấn validation/network floor;
- emergency deadline phải bỏ W0/search/W1 challenger và sinh đúng \(WAIT(S_d)\) ở current cell cho mọi agent; test fuel 0, current cell là pond-adjacent/road/spot, \(S_d=1\), serializer duration và serializer per-step đều round-trip cùng semantics;
- emergency fixture phải chứng minh position/fuel không bị đọc thành pending movement, tổng duration từng agent đúng \(S_d\), exact simulator + independent validator đều pass; nếu p99 ba floor vượt deadline tối thiểu thì gate competition-readiness fail.

### 12.4. Benchmark chiến lược

So sánh ít nhất bốn cấu hình:

1. greedy một ngày;
2. Ý tưởng 2: TOP-FC weighted + rendezvous;
3. Ý tưởng 3: rolling ALNS không shield;
4. UDON-SHIELD đầy đủ.

Không gộp kết quả thành một “điểm trung bình” duy nhất. Báo cáo:

- tỷ lệ thắng theo so sánh từ điển;
- lifetime brand coverage;
- daily diversity với lifetime coverage bằng nhau;
- servings với hai tier trước bằng nhau;
- invalid rate, bắt buộc bằng 0;
- response-time distribution khi ba tier score bằng nhau;
- ablation: single robust \(K^*\) so với viability frontier, bỏ tier-2 witness, bỏ adversarial scenarios, nén footprint cả ở final evaluation, dùng weighted objective, dùng heuristic tanker cố định.

Risk-policy benchmark phải báo riêng static policy v1, các policy candidate và oracle hindsight; tuyệt đối không dùng oracle trong production. Train/validation/held-out tách theo map family, báo paired bootstrap confidence interval của win-rate và tỷ lệ rơi thấp hơn baseline ở tier 1. Nếu hai policy không khác biệt có ý nghĩa trên held-out, chọn policy tĩnh đơn giản hơn. Một adaptive policy chỉ được benchmark như ứng viên tương lai sau khi opponent model qua probability-calibration test; nó không được âm thầm thay static v1.

Kích thước held-out được pre-register theo mục tiêu độ rộng confidence interval/power, không chốt tùy ý “vài map” hay “vài trăm map”. Báo thêm effective scenario size, calibration error, decision-flip rate khi bootstrap/resample scenario, tỷ lệ ngày phải dùng fallback manifest, p99 candidate-profile/tie-group update, W1 shortlist stability dưới shuffled generation order, số candidate W1 certify được trong budget, CAP/PREFIX cut rounds, hotspot promotions và p99 latency của từng deadline class. Chỉ tăng số scenario khi các metric này chứng minh có thêm ổn định trên mỗi đơn vị thời gian.

Map test cần có cả ngẫu nhiên và adversarial hand-crafted: brand hiếm ở ngõ cụt, road sắp vượt threshold, fuel vừa sát, stock cao cần nhiều patrol, và trường hợp overnight harvest làm đổi nghiệm tối ưu.

---

## 13. Lộ trình triển khai

### Giai đoạn A - Đúng luật trước

- Core even-r và đồ thị chi phí theo ô nguồn.
- Exact simulator theo step.
- Validator độc lập và golden tests từ Q&A.
- Parser/serializer API có versioning; không hard-code khác biệt giữa format Nhật và format Việt Nam.

### Giai đoạn B - Baseline mạnh, deterministic

- Pareto path time-fuel.
- Greedy franchise-first đúng từ điển.
- Stock/agent-order awareness.
- Cheap-bound scan toàn bộ role assignment + beam refine 2-3 mask.
- Explicit `WAIT(1)` overnight harvest và sustained end-day dock.

### Giai đoạn C - Phối hợp đội

- Route columns.
- CP-SAT relaxed-stock master + lazy cuts + exact local hotspot promotion.
- Rendezvous/escort synchronization.
- Sequential lexicographic solve và bounds.

### Giai đoạn D - Đa ngày và bất định

- Backward reachability, brand deadlines và ba cấp Fast/Witness/Proof.
- Viability envelope \((K_{cap},K_{safe},C_\rho,Q_{\mathcal Q},LB_2/UB_2,LB_3/UB_3)\).
- Frozen common scenario manifest, traffic interval/belief, likely scenarios và public-exposure adversarial scenarios.
- Two-stage recourse 1-2 ngày với deterministic own footprint.

### Giai đoạn E - Anytime và thi đấu

- Shielded ALNS operators.
- Frozen deadline-class profiles, W0/W1 accounting, calibrated \(\tau_d\), emergency fallback và response ledger.
- Submit-once hoặc certified-only upgrade có lưu quantile/survival certificate lần gửi trước; competition build không chứa risk-mode switch.
- Contingency precompute sau khi gửi.
- Replay logging: state, candidate, bound, reason reject, serialized answer và simulator trace.

---

## 14. Vì sao Ý tưởng 4 tốt hơn ba ý tưởng còn lại

| Vấn đề | Ý tưởng 1 | Ý tưởng 2 | Ý tưởng 3 | UDON-SHIELD |
|---|---|---|---|---|
| Đúng score từ điển | Nêu ưu tiên nhưng chưa khóa solver | Weighted fitness | Chưa mô tả khóa tuần tự | Solve tuần tự + bounds |
| Bảo vệ brand hiếm tương lai | Chung chung | Không | Rolling horizon nhưng không certificate | Viability frontier + calibrated risk policy + tier-2 witness |
| Phân vai | Heuristic nhóm | Quy tắc 1-2 tanker | FDI/RCI threshold | Cheap scan 256 mask, refine beam 2-3 mask |
| Time-fuel | Có nhắc nhưng lẫn bước/fuel | Khoảng cách/cache | Effective distance | Pareto labels trên cạnh theo ô nguồn |
| Co-location/refuel | Có hộ tống | Rendezvous | Mobile pipeline | Event sync + escort + final-step dock |
| Stock nội bộ/agent order | Mờ | Mờ | Voronoi dễ bỏ multi-visit | Exact simulator + stock-aware master |
| Traffic ẩn | Monte Carlo chung | Giả định thao túng | Dự đoán tương đối chắc | Belief + likely/adversarial reachable scenarios |
| Traffic footprint | Chủ yếu tránh Wait | Tránh kết thúc road | Self-shielding quá mạnh | Full sparse vector + simulator, adaptive critical set |
| Tính hợp lệ | Chưa là lõi | Penalty/fallback | Fallback | Hard constraints + double validation |
| Response tie-break | Tối ưu tốc độ chung | Gửi sớm rồi ghi đè | Gửi nhiều tầng | Cumulative ledger + fixed calibrated \(\tau_d\) + certified-only upgrade |
| Khả năng triển khai | Rộng, nhiều hướng | Baseline khá rõ | Mạnh nhưng nhiều tuyên bố khó chứng minh | Decomposition có milestone và fallback |

Ý tưởng 4 chấp nhận những gì đúng ở ba ý tưởng trước, nhưng đặt chúng dưới một thứ tự quyền lực rõ ràng:

```text
Luật và simulator
    > score từ điển
        > viability của toàn trận
            > tính thích nghi trước traffic
                > tối ưu route ngày hiện tại
                    > servings
                        > thời gian phản hồi
```

Đó là cấu trúc khiến một heuristic nhanh không thể vô tình đánh bại mục tiêu thật, một ALNS không thể chấp nhận nghiệm “đẹp nhưng mất brand”, và một dự báo traffic sai không thể làm cả kế hoạch sụp đổ.

---

## 15. Những điều không nên làm

- Không dùng CBS/avoid-collision vì agent được phép cùng ô.
- Không dùng một weighted reward chưa chứng minh cận Big-M.
- Không mặc định 1 tanker cho mọi map.
- Không chia Voronoi cứng khiến patrol không được dùng chung spot/corridor.
- Không xem traffic là số đã biết khi server chỉ cho status.
- Không tin rằng không Wait trên road đồng nghĩa không tạo traffic.
- Không hy sinh score để cố gây tắc đối thủ.
- Không gửi nhiều đáp án cùng score; lần hợp lệ cuối làm xấu tie-break.
- Không để mô hình học quyết định tính hợp lệ. ML, nếu dùng, chỉ nên xếp hạng column, dự báo kịch bản đối thủ hoặc chọn ALNS operator; simulator và score vẫn deterministic.

---

## 16. Kết quả kiểm tra các phản biện

| Phản biện | Kết luận | Cách xử lý trong bản sửa |
|---|---|---|
| \(K^*\) toàn cục vừa NP-hard vừa nằm trên critical path | Đúng | Bỏ single \(K^*\); tách Fast/Witness/Proof, fast cap 10%, incumbent sinh trước proof |
| Worst-case \(K^*\) làm bot quá bảo thủ | Đúng | Dùng viability frontier và downside-first joint-outcome profile; không còn hard \(K_{cap}\) guard |
| Envelope đa chiều chưa tạo ra một thứ tự thi hành | Đúng | Thêm confidence gate tương đối luôn không rỗng, rồi total comparator bằng downside-first joint-outcome quantile ladder, survival signature, certified bound, score hiện tại, slack, traffic và stable ID |
| Hai survival profile cắt nhau thì risk budget vẫn chủ quan | Đúng và không thể loại bỏ bằng toán nếu thiếu utility/xác suất thật | Công khai candidate v1 \((\rho=0.80,\delta=1,\varepsilon=0.01)\), calibrate offline, version và freeze manifest trong trận; không giả vờ đây là hệ quả của luật |
| Hard recoverability guard có thể loại phương án \(M-1\) an toàn trong khi phương án \(M\) rủi ro sau đó cũng bị loại | Đúng, là lỗi logic | Bỏ guard hoàn toàn; một candidate chỉ bị xóa khi lower profile đã chứng nhận của candidate khác thống trị upper profile hợp lệ của nó ở mọi threshold |
| Shield chỉ giữ tier 1, có thể làm mất tier 2 | Đúng | Viability envelope lưu \(LB_2/UB_2\), phân bổ brand theo ngày và witness cụ thể |
| EVoC chưa được định nghĩa và xung đột với score từ điển | Đúng | Loại EVoC khỏi production; dùng \(\tau_d\) cố định đã calibrate offline, chỉ dừng sớm khi bound khít hoặc mọi certified UB còn lại không thể thắng incumbent |
| Response time là tổng toàn trận | Đúng về dữ kiện; kết luận “luôn dùng hết deadline” không luôn đúng | Tách cumulative ledger khỏi search schedule; tối ưu tới \(\tau_d\), nhưng giữ hard certificate/network deadlines và không đánh đổi tier 1-3 lấy thời gian |
| Bảng ngân sách ngày vượt 100% và lẫn đồng hồ pre-match | Đúng | Tách \(T_{role}\) khỏi \(T_d\); normal-v1 là 10% + 10% + 60% + 10% + 10%, mọi deadline-class profile đều chuyển thành absolute deadlines và cộng đúng 100% |
| Non-anticipativity áp dụng sai vì own traffic đã biết | Không đúng về toán, nhưng cách diễn đạt cũ dễ hiểu sai | Viết lại thành two-stage recourse: own footprint deterministic, chỉ opponent là exogenous; action hiện tại vẫn phải chung giữa các nhánh |
| Agent-order stock tie chưa được xác nhận | Không đúng với tài liệu chính thức | Q&A2 câu 26 và phụ lục Q6 xác nhận thứ tự danh sách agent; vẫn thêm conformance test cho server Việt Nam |
| Overnight một ngày vẫn có thể đếm trước diversity tương lai | Đúng | One-day master không credit brand/serving tương lai trong official key; overnight chỉ là terminal feature, chỉ nâng thành \(LB_2/LB_3\) khi future witness qua simulator |
| Adversarial dwell giả định đối thủ biết dependency route bí mật | Đúng với cách dựng cũ | Production chỉ sinh bounded-rational scenario từ tín hiệu công khai: cut/articulation road, start-spot betweenness, brand gateway, repeated endpoint corridor; private dependency chỉ là sensitivity test, omniscient case chỉ dùng offline |
| Quét 256 role mask có thể quá chậm | Đúng nếu rollout sâu cả 256; bản cũ đã prune nhưng chưa khóa budget | Cheap-bound scan toàn bộ, beam 2-3 mask, hard role budget và send reserve |
| Nén road footprint có thể bỏ sót road phụ | Đúng nếu nén ở final evaluation | Chỉ nén dominance khi sinh nhãn; column lưu full sparse vector, simulator kiểm tra và promote road mới |
| Exact stock/agent-order trong master có thể làm CP-SAT nổ kích thước | Đúng | Base master dùng stock relaxation; simulator giữ exact incumbent, thêm \(CAP(s)\)/\(PREFIX(e)\), rồi chỉ promote exact constraints tại hotspot lặp |
| Gửi đè dựa trên ước lượng lạc quan | Đúng | Competition build chỉ có certified-only resend; không chứa risk-mode switch hay đường vòng bằng UB/estimated win probability |
| Tự thích nghi risk parameter khi chưa có opponent model đã calibrate | Đúng | Production v1 dùng static versioned parameters; thay đổi chỉ qua benchmark/replay độc lập trước trận |
| Quantile lex chưa nói rõ outcome universe chung | Hữu ích nhưng không phải lỗi của quantile trên total order; lỗi thật là mô tả/grid của \(G\) còn mơ hồ | Định nghĩa \(\mathcal Y_{off}\) chung; quantile dùng candidate-local histogram, còn \(\mathcal Y_{eval}(T)\) chỉ là hợp support của quantile tie-group |
| Quantile ladder thừa vì đã có full survival signature | Kết luận “thừa” không đúng | Ladder mã hóa risk semantics; \(G\) chỉ complete total order sau khi năm quantile hòa. Dùng \(G\) trước sẽ tạo policy tail-averse khác |
| W0/W1 witness repair chưa rõ ăn bucket nào | Đúng | W0 thuộc Fast viability 10%; W1 provisional-shortlist certification thuộc Certificate/validation 10% và phải giữ validation floor |
| `stableId` có thể bỏ qua khác biệt traffic tương lai | Đúng một phần; tie-break không có lỗi certification nhưng scalar traffic key cũ còn thô | Thêm exact `trafficSafety` counterfactual trước `stableId`; stable ID dùng canonical bytes với collision fallback và chỉ chọn giữa route đã tương đương theo mọi quality key |
| Calibration có vẻ tự chấm bằng internal `Key` | Đúng về sự mơ hồ của câu cũ | Metric ngoài là realized official end-of-match vector/win-loss, độc lập hoàn toàn với \(\rho,\delta,\varepsilon,\mathcal Q\) |
| Bắt buộc vài trăm scenario để \(\varepsilon=0.01\) có nghĩa | Nguyên tắc chống false precision đúng; con số cứng không cần thiết và có thể hại budget | Phân biệt discrete policy measure với learned probability; kiểm tra effective coverage/calibration error, fail closed sang fallback manifest và chỉ dùng \(G\) khi đạt gate |
| Tune đồng thời nhiều risk/latency parameter dễ meta-overfit | Đúng | Pre-register catalog nhỏ, selection theo one-standard-error, tune latency ở nested pass riêng và chỉ mở held-out một lần |
| Nhiều scenario làm W1 certificate repair thành bottleneck | Đúng như rủi ro hiệu năng | W1 chỉ certify provisional-\(Key^0\) floor leader, distinct max-valid-UB challenger và incumbent/lần gửi trước, có hard budget/validation floor |
| Đối thủ có thể học dependency qua lịch sử nhiều ngày | Đúng một phần và đã được endpoint history bao phủ một phần | Thêm dynamic public exposure từ endpoint/status history; vẫn cấm dùng private route dependency làm kiến thức đối thủ |
| 10-10-60-10-10 không phù hợp deadline cực ngắn | Đúng | Giữ normal-v1, thêm frozen deadline-class profiles và emergency `WAIT(S_d)` với absolute p99 seed/validation/network floors |
| Lazy cuts có thể lặp nhiều ở low-stock hotspot | Đúng như integration risk | Conflict-aware pricing, \(CAP(s)\), \(PREFIX(e)\), promote sau \(h_{promote}=2\) prefix violations và chặn \(R_{cut}=\min(8,2N)\) rounds |
| Emergency “legal filler action” chưa được định nghĩa | Đúng; giả thuyết pending movement xuyên ngày không phù hợp progression chính thức nhưng không làm giảm lỗi đặc tả | Core fallback duy nhất là mỗi agent `WAIT(currentCell,S_d)`; duration sum đúng \(S_d\), không movement/fuel requirement, adapter round-trip + hai validator bắt buộc |
| Event-capacity cut “khi suy ra được” không có thuật toán | Đúng | Column khai báo first-visit/claimed-serving; aggregate overcount luôn sinh \(CAP(s)\), serving bắt buộc sinh predecessor-order \(PREFIX(e)\); no-good không còn là fallback mặc định cho stock |
| Rebuild toàn pool mỗi candidate làm anytime comparator thành bottleneck | Đúng về hiệu năng, không phải mâu thuẫn semantics | Freeze \(\Omega_d\); candidate mới chỉ tính local histogram/quantiles và mark dirty. \(G^0\) finalize trước W1, final \(G\) refresh sau W1; không rebuild trong inner loop |
| W1 dùng final Key chứa \(LB^{cert}\) để chọn người được certify | Đúng, tạo order-dependent selection bias | Tạo \(Key^0\) đối xứng từ F0 fixed-cap pessimistic profile trong search budget, bỏ mọi certificate history; slot hai dùng earliest-tier/valid-UB và phải khác floor leader |

Các phản biện làm thay đổi đáng kể thiết kế: UDON-SHIELD không còn là “robust single-target shield”, mà là **risk-aware viability frontier có một total decision policy công khai, hard compute budgets, conditional tier witnesses và certified-only submission**.

---

## 17. Nguồn và căn cứ

### Luật và format chính thức

- [Đề bài HEXUDON tiếng Anh - NAPROCK](https://www.naprock.jp/uploads/media/BblyWRbwVAA)
- [Trang tài liệu chính thức KOSEN Procon 2026, gồm Q&A và format](https://www.procon.gr.jp/?p=73510)
- [Q&A phần 1](https://www.procon.gr.jp/uploads/download/BcAnkyvgVAA)
- [Phụ lục chi tiết thứ tự hành động theo step](https://www.procon.gr.jp/uploads/download/BcAnkz3QVAA)
- [Q&A phần 2](https://www.procon.gr.jp/uploads/download/BcgLrQEgVAA)
- [Format JSON công bố tháng 5/2026](https://www.procon.gr.jp/uploads/download/BcgL66tAVAA)

Format API cuối cùng của Procon Việt Nam có thể khác tên field hoặc protocol bọc ngoài. Core luật nên tách khỏi adapter, sau đó kiểm chứng lại bằng tài liệu/máy chủ tập luyện của BTC Việt Nam trước khi thi.

### Thuật toán tham khảo

- Chao, Golden, Wasil, [The Team Orienteering Problem](https://doi.org/10.1016/0377-2217(94)00289-4), EJOR 1996.
- Ropke, Pisinger, [An Adaptive Large Neighborhood Search Heuristic for the Pickup and Delivery Problem with Time Windows](https://doi.org/10.1287/trsc.1050.0135), Transportation Science 2006.
- Irnich, Desaulniers, [Shortest Path Problems with Resource Constraints](https://doi.org/10.1007/0-387-25486-2_2), 2005.
- Irnich, Villeneuve, [The Shortest-Path Problem with Resource Constraints and k-Cycle Elimination](https://doi.org/10.1287/ijoc.1040.0117), INFORMS Journal on Computing 2006.
- [Google OR-Tools CP-SAT documentation](https://developers.google.com/optimization/cp/cp_solver).

Các nguồn trên chỉ cung cấp công cụ nền. Phần đặc thù tạo lợi thế của Ý tưởng 4 là cách ghép chúng theo luật HEXUDON: **risk-aware lexicographic viability frontier + synchronized route portfolio + hidden-traffic recourse + overnight harvest/end-step docking + certificate-aware submission**.

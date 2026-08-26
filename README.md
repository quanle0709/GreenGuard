# 🌿 GreenGuard

GreenGuard là mái che cây tự động chạy trên **NodeMCU 1.0 (ESP-12E Module / ESP8266)**. Khi tín hiệu mưa ổn định, bộ điều khiển có thể che cây; khi trời khô đủ lâu, nó mới thu mái che. Toàn bộ phần phản ứng với mưa nằm ngay trên board nên mất điện thoại, trình duyệt hay Wi‑Fi thì logic bảo vệ vẫn chạy.

Tụi mình làm GreenGuard vì chuyện chạy ra kéo mái che mỗi khi mưa nghe đơn giản, nhưng nếu đang ở trường hoặc mưa tới quá nhanh thì khá bất tiện. Bản này tập trung vào một MVP dễ hiểu, dễ kiểm tra và không giả vờ biết những gì phần cứng chưa đo được.

> Controller đã được chủ dự án xác nhận. GPIO, dây BTS7960, điện áp DO, nguồn, chiều motor, công tắc hành trình và thời gian chạy thật **chưa được xác nhận vật lý**.

## Bản hiện tại làm được gì?

- Lọc nhiễu DO không chặn chương trình: mặc định xác nhận ướt 3 giây, xác nhận khô 120 giây.
- AUTO tự che cây khi mưa; chỉ tự thu khi khô ổn định và đang có ước lượng vị trí.
- MANUAL có lệnh rõ nghĩa: **Che cây**, **Thu mái che**, **Dừng khẩn cấp**, **Đặt lại lỗi**.
- Ngắt cả hai PWM trước khi đổi chiều và chờ dead-time 300 ms.
- Giới hạn thời gian motor, khóa lỗi, ghi trạng thái LittleFS và chuyển về `UNKNOWN` nếu khởi động lại giữa lúc chạy.
- Dashboard tiếng Việt chạy trực tiếp trên ESP8266, responsive cho điện thoại, có request ID và trạng thái nhận/bắt đầu/hoàn tất riêng.
- Có token điều khiển LAN tùy chọn; tự động cảnh báo nếu chưa đặt token.
- `ACTUATOR_DRY_RUN=true` là mặc định bắt buộc: state machine chạy nhưng RPWM, LPWM và enable do firmware điều khiển vẫn LOW.

Nói ngắn gọn: tụi mình muốn code đủ “cứng đầu” để một cú double-click hay Wi‑Fi chập chờn không biến thành lệnh motor thứ hai :)))

## Nó hoạt động như thế nào?

```text
DO cảm biến mưa
       │
       v
xác nhận WET/DRY ──> state machine trên ESP8266 ──> interlock + dry-run ──> BTS7960 ──> motor
                              │
                              ├──> trạng thái/cấu hình LittleFS
                              └──> REST API + dashboard trong cùng mạng LAN
```

Browser chỉ quan sát và gửi yêu cầu. ESP8266 mới là nơi quyết định lệnh có hợp lệ hay không; dashboard không được phép coi “HTTP đã nhận” là “mái che đã chạy xong”.

## Phần cứng

| Thành phần | Trạng thái hiện tại |
| --- | --- |
| NodeMCU 1.0 (ESP-12E Module), ESP8266 | **Đã xác nhận**; PlatformIO `nodemcuv2` |
| BTS7960 / Driver HW-039 | ** Đã xác nhận** |
| Motor DC giảm tốc, có thể là 12 V DC |  |
| Cảm biến mưa RainDrop dùng DO | Chưa xác nhận model, VCC, cực tính hay điện áp DO |
| Nguồn tổ ong 12V 10A DC |  |
| Module hạ áp XL4005|

Pin trong code là **profile ứng viên từ lịch sử repo**, không phải sơ đồ as-built:

| NodeMCU | GPIO | Kết nối ứng viên | Ghi chú |
| --- | ---: | --- | --- |
| D1 | 5 | Rain DO | Đo điện áp/cực tính trước; GPIO không được nhận quá 3.3 V |
| D5 | 14 | RPWM | Chưa xác nhận đầu dây và chiều |
| D6 | 12 | LPWM | Không bao giờ hoạt động cùng RPWM |
| D2 | 4 |R_EN VÀ L_EN tùy chọn | Mặc định không dùng; tuyệt đối không nối khi EN vẫn ở 5 V |
|Vin|   | 5V OUT+ XL4005 |
|VCC|   | GND OUT- XL4005|

D3/GPIO0, D4/GPIO2 và D8/GPIO15 là chân boot-strapping nên profile mặc định tránh dùng. Xem [wiring worksheet](WIRING.md) và [hardware audit](docs/HARDWARE_AUDIT.md) trước khi cắm bất cứ dây nào.

## Cảnh báo nguồn và điện áp

**Ngắt nguồn motor khi đấu dây. Không đưa 5 V hoặc 12 V vào bất kỳ GPIO ESP8266 nào. Không cấp motor từ NodeMCU.**

Đo DO ở trạng thái khô và ướt trước khi nối D1. Xác nhận nguồn logic/motor, GND chung, cực tính, dòng stall, khả năng nguồn, fuse, tiết diện dây và công tắc ngắt khẩn cấp. R_EN/L_EN đang là điểm chưa rõ: nếu chúng thật sự được kéo lên 5 V thì D2 phải để rời; nếu muốn firmware điều khiển enable thì phải tháo hoàn toàn dây 5 V trước.

## AUTO, MANUAL và các nút

| Điều khiển | Nghĩa chính xác |
| --- | --- |
| Tự động | Xác nhận mưa rồi che; xác nhận khô lâu hơn rồi thu |
| Thủ công | Dừng chuyển động hiện tại và chờ lệnh rõ ràng |
| Che cây | Đưa mái che tới trạng thái `DEPLOYED`; được phép khi mưa |
| Thu mái che | Đưa mái che tới `RETRACTED`; bị chặn nếu chưa xác nhận khô |
| Dừng khẩn cấp | Tắt drive ngay, chuyển MANUAL và giữ STOP latch |
| Đặt lại lỗi | Sau khi kiểm tra cơ khí/điện, xóa lỗi nhưng vẫn dừng ở MANUAL |

Nếu mưa quay lại trong lúc đang thu thủ công, GreenGuard dừng, chờ dead-time rồi che cây. Nếu vị trí đang `UNKNOWN`, AUTO được phép chạy full-time theo hướng che để bảo vệ, nhưng không tự thu mù quáng.

## Vị trí là ước lượng, không phải phép đo

Khi chưa có limit switch, 0% và 100% chỉ là kết quả của thời gian chạy. Điện áp, tải, ma sát, trượt, vật cản và mất điện đều làm nó lệch. Dashboard ghi `ESTIMATED`; STOP giữa đường ghi `STOPPED_PARTIAL`; reboot giữa chuyển động ghi `UNKNOWN`. Nếu bạn nhìn trực tiếp một endpoint rồi đánh dấu, trạng thái là `USER_CALIBRATED`. Chỉ switch đã bật và đang tác động mới là `LIMIT_CONFIRMED`.

Đây là chỗ tụi mình từng loay hoay khá lâu :3 — biến “motor chạy 30 giây” thành “mái che chắc chắn đã tới nơi” là một kết luận sai nếu không có feedback vật lý.

## Cấu trúc repo

```text
include/                   cấu hình phần cứng, persistence, secrets example
src/                       tích hợp ESP8266, Wi-Fi, LittleFS, REST
lib/GreenGuardCore/src/    state machine C++ không phụ thuộc phần cứng
data/                      dashboard được nạp vào LittleFS
test/test_core/            mô phỏng controller native
test/web.test.mjs          protocol, DOM contract, auth và integration mock
scripts/                   project check và server preview
docs/                      audit, thiết kế, protocol, test, checklist vật lý
```

## Chuẩn bị môi trường

1. Cài [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html) hoặc PlatformIO IDE.
2. Dùng Node.js 20+ để chạy test web; repo không cần `npm install` vì test chỉ dùng module có sẵn của Node.
3. Native test trên Windows cần `g++`/MinGW trong `PATH`.
4. Sao chép `include/secrets.example.h` thành `include/secrets.h`:

```cpp
#define WIFI_SSID "ten-wifi"
#define WIFI_PASSWORD "mat-khau"
#define CONTROL_TOKEN "mot-token-ngau-nhien-dai"
```

`include/secrets.h` đã nằm trong `.gitignore`. Đừng paste token hoặc mật khẩu vào issue, ảnh Serial hay commit.

## Build và test

```powershell
pio run -e nodemcuv2
pio run -e nodemcuv2 -t buildfs
pio test -e native
npm test
npm run check
```

Build đã chạy ngày 2026-08-26 cho đúng môi trường:

```ini
[env:nodemcuv2]
board = nodemcuv2
```

Kết quả hiện tại: firmware PASS, LittleFS PASS, 45/45 scenario controller (132 assertions) và 11/11 test web/protocol/integration (52 assertions). Tổng cộng 56 test case/scenario và 184 assertions. RAM 31,720/81,920 byte (38.7%); flash 382,923/1,044,464 byte (36.7%). Xem lệnh và giới hạn cụ thể trong [test results](docs/TEST_RESULTS.md).

## Nạp firmware và dashboard

Chỉ chọn đúng cổng sau khi đã nhận diện board. Lần nạp đầu vẫn phải để motor tháo rời và dry-run bật:

```powershell
pio run -e nodemcuv2 -t upload
pio run -e nodemcuv2 -t uploadfs
pio device monitor -b 115200
```

Firmware và filesystem là hai image riêng: đổi code thì upload firmware; đổi file trong `data/` thì upload filesystem. `uploadfs` ghi lại phân vùng LittleFS nên có thể làm mất state/config đang lưu; tháo motor và chuẩn bị hiệu chuẩn lại trước khi nạp. Khi Wi‑Fi kết nối, mở `http://greenguard.local`; nếu mDNS không chạy trên máy của bạn, dùng địa chỉ IP in ở Serial Monitor.

## Test an toàn lần đầu

1. Tháo motor và ngắt nguồn công suất.
2. Giữ `ACTUATOR_DRY_RUN=true`; đo RPWM/LPWM đều LOW lúc boot và khi bấm lệnh.
3. Đo DO, kiểm tra khô/ướt và cực tính riêng.
4. Nạp LittleFS, mở dashboard, thử token/AUTO/MANUAL/STOP riêng.
5. Hoàn thành [hardware test checklist](docs/HARDWARE_TEST_CHECKLIST.md).
6. Chỉ sau review phần điện mới thử một xung ngắn không tải, với emergency disconnect trong tầm tay.

Nhớ test từng phần trước nha 🔧. Đừng nối cảm biến, driver, motor và nguồn công suất cùng một lúc rồi mới bắt đầu debug.

## Dashboard và protocol

ESP8266 phục vụ HTML/CSS/JS từ LittleFS. Mỗi POST có request ID; status tách `ACCEPTED`, `STARTED`, `COMPLETED`, `STOPPED`, `REJECTED`, `FAULT`. UI bỏ qua acknowledgement sai ID, khóa double-click và không hiển thị thành công nếu 90 giây vẫn chưa có phase kết thúc. Chi tiết ở [protocol v2](docs/PROTOCOL.md).

Một mock server cho phép xem UI mà không có board:

```powershell
npm run preview
# mở http://127.0.0.1:4173 và dùng token test-token-1234
```

Browser backend của phiên rebuild không khả dụng, nên repo không thêm screenshot và không tuyên bố đã visual QA bằng browser. Responsive/DOM/accessibility contracts và flow tương tác đã được test tự động; vẫn nên mở mock preview trên desktop lẫn điện thoại trước demo.

## Đã test vật lý gì?

Không có board/serial nào được xác nhận hoặc flash trong đợt rebuild này. Không cấp điện motor, không đo cảm biến, không xác nhận chiều, endpoint, nguồn hay full-travel. Controller model được xác nhận từ thông tin chính thức của chủ dự án, không phải vì Codex nhìn thấy board.

## Giới hạn và nâng cấp nên làm

- Thêm hai limit switch để endpoint thành dữ liệu thật.
- Thêm current sensor/stall detection, fuse đúng dòng và emergency disconnect vật lý.
- Đo/level-shift rain DO; cải thiện nguồn, lọc nhiễu, dây và enclosure chống mưa.
- Token HTTP chỉ phù hợp LAN tin cậy, không phải bảo mật Internet.
- Cảm biến DO một bit không tự phát hiện được mọi kiểu sensor hỏng.

## Tài liệu

- [Hardware audit](docs/HARDWARE_AUDIT.md)
- [System design](docs/SYSTEM_DESIGN.md)
- [Protocol](docs/PROTOCOL.md)
- [Hardware test checklist](docs/HARDWARE_TEST_CHECKLIST.md)
- [Automated test results](docs/TEST_RESULTS.md)
- [Wiring worksheet](WIRING.md)

## Team

Lịch sử Git hiện có commit thật từ:

- [`quanle0709`](https://github.com/quanle0709) — owner của repository; lịch sử có initial implementation và cập nhật hardware wiring.
- [`minhkhoi092211`](https://github.com/minhkhoi092211) — lịch sử có commit thêm archive web/IoT.
- [`nhiennguyenquoc`](https://github.com/nhiennguyenquoc) — lịch sử có các commit README, gồm commit `18a98b3` ngay trước rebuild.

Repo không đủ bằng chứng để tụi mình tự gán vai trò chi tiết hơn. GitHub chỉ liên kết contributor khi commit dùng email đã gắn/xác minh với đúng tài khoản (hoặc một PR/commit thật từ tài khoản đó). Không cần và không nên tạo commit rỗng hay ghi co-author giả để “làm đẹp” bảng Contributors.

## Một lời nhắn nhỏ

Phiên bản đầu gần như chắc chắn sẽ có dây cắm nhầm, DO bị đảo hoặc thời gian motor chưa đúng — chuyện đó không có nghĩa ý tưởng thất bại. Tách nhỏ ra: test sensor, web, rồi driver/motor không tải. Ghi lại từng phép đo, sửa một thứ mỗi lần và luôn giữ cách ngắt điện trong tầm tay. Nếu bạn cũng muốn thử, cứ học từ repo này rồi làm nó chắc hơn nhé; bọn mình rất muốn thấy GreenGuard có limit switch thật ở phiên bản tiếp theo :)))

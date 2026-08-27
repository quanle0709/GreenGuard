# 🌿 GreenGuard

GreenGuard là mái che cây tự động chạy trên **NodeMCU 1.0 (ESP-12E Module / ESP8266)**, đọc cảm biến mưa và điều khiển motor để che hoặc thu mái che. Đây là sản phẩm cuối khóa do các bạn học viên trực tiếp lắp ráp, tích hợp, debug và kiểm thử sau ba tháng làm quen với embedded systems.

## GreenGuard bắt đầu từ đâu?

GreenGuard bắt đầu từ một khóa học nhập môn embedded systems kéo dài ba tháng. Minh Quân khởi xướng và trực tiếp hướng dẫn khóa học cho một nhóm nhỏ bạn bè cũng là học viên của mình. Mục tiêu không phải tạo một startup hay một sản phẩm thương mại, mà là giúp các bạn hiểu nền tảng hệ thống nhúng bằng trải nghiệm thật trước khi chọn ngành học đại học.

Tụi mình học theo cách **learning by building — học bằng cách tự tay làm**. Minh Quân thiết kế và dẫn dắt quá trình học, giải thích nguyên lý, đưa ra định hướng kỹ thuật, hỗ trợ giải quyết vấn đề và giám sát lúc tích hợp, kiểm thử. Các học viên không chỉ đứng xem: sau ba tháng học và khám phá, các bạn trực tiếp biến từng phần đã học thành prototype GreenGuard hoàn chỉnh dưới sự hướng dẫn của Minh Quân.

GreenGuard vì vậy vừa là một hệ thống thật, vừa là bài tổng kết để cả nhóm trả lời câu hỏi: “Một cảm biến, một vi điều khiển, một driver và một motor phối hợp với nhau ngoài đời như thế nào?” Câu trả lời có nhiều dây hơn tụi mình tưởng :)))

## Tụi mình muốn học được gì?

Qua quá trình làm GreenGuard, các học viên được tiếp xúc thực tế với:

- Vai trò của microcontroller trong một hệ thống nhúng.
- Cách đọc tín hiệu số từ cảm biến và nối đầu vào đó với hành vi của cơ cấu chấp hành.
- Cách điều khiển motor DC thông qua driver BTS7960/HW-039.
- GPIO, mức điện áp, nguồn riêng cho motor và khái niệm GND chung.
- Firmware phản ứng với tín hiệu ngoài đời mà không khóa vòng lặp.
- Sự khác nhau giữa điều khiển AUTO và MANUAL.
- Tích hợp firmware với dashboard web chạy trong mạng nội bộ.
- Debug phần cứng và phần mềm như một hệ thống duy nhất.
- Đo điện áp, dòng điện, timing và nhiệt độ thay vì đoán hệ thống “có vẻ ổn”.
- Một góc nhìn thực tế về việc học embedded systems và các ngành kỹ thuật liên quan.

## Sản phẩm cuối khóa làm được gì?

- Lọc nhiễu DO không chặn chương trình: mặc định xác nhận ướt 3 giây và xác nhận khô 120 giây.
- AUTO tự che cây khi mưa; chỉ tự thu khi khô ổn định và đang có ước lượng vị trí.
- MANUAL có lệnh rõ nghĩa: **Che cây**, **Thu mái che**, **Dừng khẩn cấp**, **Đặt lại lỗi**.
- Ngắt cả hai PWM trước khi đổi chiều và chờ dead-time cấu hình 300 ms.
- Giới hạn thời gian motor, khóa lỗi, lưu trạng thái bằng LittleFS và chuyển về `UNKNOWN` nếu khởi động lại giữa lúc chạy.
- Dashboard tiếng Việt chạy trực tiếp trên ESP8266, responsive cho điện thoại, có request ID và trạng thái nhận/bắt đầu/hoàn tất riêng.
- Có token điều khiển LAN tùy chọn và vẫn chạy logic AUTO khi Wi‑Fi hoặc trình duyệt mất kết nối.
- Chống gửi lặp do double-click hoặc acknowledgement đến trễ.

Repository công khai luôn giữ `ACTUATOR_DRY_RUN=true`: state machine vẫn chạy nhưng RPWM, LPWM và enable do firmware điều khiển luôn LOW. Prototype thật được kiểm thử bằng một cấu hình local đã bật actuator; cấu hình đó không được commit và không chứa thông tin cần công khai.

## Luồng hệ thống

```text
RainDrop DO
      │
      v
xác nhận WET/DRY ──> state machine ESP8266 ──> interlock + timeout ──> BTS7960 ──> motor
                              │
                              ├──> trạng thái/cấu hình LittleFS
                              └──> REST API + dashboard trong cùng mạng LAN
```

Browser chỉ quan sát và gửi yêu cầu. ESP8266 mới là nơi quyết định lệnh có hợp lệ hay không; dashboard không được phép coi “HTTP đã nhận” là “mái che đã chạy xong”.

## Prototype thật và kết quả đo

Firmware dựa trên commit [`441f91e`](https://github.com/quanle0709/GreenGuard/commit/441f91ea89253d85c3314e3da47faa3d03afb850) đã được nạp vào NodeMCU 1.0 ESP-12E và kiểm thử trên prototype hoàn chỉnh ngày **2026-08-26**. Các giá trị dưới đây là kết quả đo do chủ dự án cung cấp, không phải ước lượng:

| Hạng mục | Kết quả |
| --- | ---: |
| Kiểm thử che–thu liên tục | 10 chu kỳ với tải cơ khí thật |
| Điện áp nguồn motor trước khi chạy | 12.18 V |
| Điện áp nguồn motor khi đang chạy | 11.72 V |
| Độ sụt áp nguồn | 3.8% |
| Mức thấp nhất của rail 3V3 khi motor khởi động | 3.17 V |
| NodeMCU reset trong quá trình kiểm thử | Không có |
| Rain sensor DO khi khô | 3.27 V |
| Rain sensor DO khi ướt | 0.08 V |
| Logic cảm biến mưa | Active-LOW |
| Dòng motor khi chuyển động có tải bình thường | 2.63 A |
| Dòng khởi động lớn nhất đo được | 7.20 A |
| Dead-time đảo chiều đo được | 307 ms |
| Nhiệt độ motor cao nhất sau 10 chu kỳ | 51°C |
| Nhiệt độ dây và connector sau 10 chu kỳ | 34°C |
| Đánh giá tổng thể | **PASS có điều kiện trong phạm vi kiểm thử này** |

Prototype đã vượt qua quan sát chức năng và điện trong 10 chu kỳ che–thu liên tiếp dưới tải thật. Rail 3V3 duy trì đủ ổn định trong các lần khởi động được quan sát và NodeMCU không reset. DO của RainDrop được xác nhận active-LOW, với cả mức khô lẫn ướt nằm trong dải điện áp GPIO ESP8266 trong lần test này. Dead-time 307 ms cũng bám sát mục tiêu cấu hình 300 ms.

“PASS có điều kiện” rất quan trọng: 10 chu kỳ chưa chứng minh độ bền dài hạn, khả năng chống mưa ngoài trời, ingress protection, mọi tình huống vật cản hay độ tin cậy trong toàn bộ vòng đời. Tụi mình không biến một buổi test tốt thành lời hứa “an toàn tuyệt đối”.

## Phần cứng đã dùng

| Thành phần | Trạng thái |
| --- | --- |
| NodeMCU 1.0 (ESP-12E Module), ESP8266 | Đã xác nhận; PlatformIO `nodemcuv2` |
| BTS7960 / driver HW-039 | Đã xác nhận trên prototype |
| Motor DC giảm tốc 12 V | Đã xác nhận trên prototype |
| Cảm biến mưa RainDrop dùng DO active-LOW | Đã xác nhận; đo 3.27 V khô và 0.08 V ướt |
| Nguồn tổ ong 12 V, 10 A | Đã xác nhận trên prototype |
| Module hạ áp XL4005 | Đã xác nhận trên prototype |
| Công tắc hành trình | Chưa có thông tin xác nhận; firmware để tắt |

### Wiring vẫn cần được ghi lại chính xác

Các phép đo xác nhận prototype hoạt động, nhưng chưa cung cấp một bảng as-built đầy đủ cho từng dây. Pin trong code vẫn là profile ứng viên từ lịch sử repository:

| NodeMCU | GPIO | Kết nối ứng viên | Trạng thái tài liệu |
| --- | ---: | --- | --- |
| D1 | 5 | RainDrop DO | Điện áp và active-LOW đã đo; chân thực tế chưa được ghi lại bằng bảng as-built |
| D5 | 14 | BTS7960 RPWM | Mapping trong firmware; terminal thực tế cần được ghi lại |
| D6 | 12 | BTS7960 LPWM | Mapping trong firmware; terminal thực tế cần được ghi lại |
| D2 | 4 | Enable R_EN/L_EN tùy chọn | Mặc định firmware không điều khiển; wiring EN thực tế chưa được cung cấp |
| D7 | 13 | Limit thu tùy chọn | Tắt; chưa xác nhận có switch |
| D0 | 16 | Limit che tùy chọn | Tắt; chưa xác nhận có switch |

D3/GPIO0, D4/GPIO2 và D8/GPIO15 là các chân boot-strapping nên profile mặc định tránh dùng. Xem [wiring worksheet](WIRING.md) và [hardware audit](docs/HARDWARE_AUDIT.md) trước khi sửa dây hoặc thay đổi cấu hình output.

## Cảnh báo nguồn và điện áp

**Ngắt nguồn motor khi đấu dây. Không đưa 5 V hoặc 12 V vào bất kỳ GPIO ESP8266 nào. Không cấp motor từ NodeMCU.**

Các giá trị đã đo chỉ mô tả prototype trong 10 chu kỳ thử nghiệm ngày 2026-08-26. Dòng 7.20 A là dòng khởi động lớn nhất quan sát được, không phải kết quả stall test. Chưa có kết quả được cung cấp cho stall, vật cản, fuse rating, tiết diện dây, chống nước hoặc chạy dài hạn.

R_EN/L_EN vẫn cần một sơ đồ as-built rõ ràng. Nếu chúng đang được kéo lên 5 V thì D2 phải để rời; nếu muốn firmware điều khiển enable, phải tháo hoàn toàn dây 5 V trước. Không bao giờ nối cùng một node enable vừa với 5 V vừa với GPIO ESP8266.

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

Khi chưa có limit switch, 0% và 100% trong firmware chỉ là kết quả của thời gian chạy. Điện áp, tải, ma sát, trượt, vật cản và mất điện đều làm nó lệch. Dashboard ghi `ESTIMATED`; STOP giữa đường ghi `STOPPED_PARTIAL`; reboot giữa chuyển động ghi `UNKNOWN`. Nếu người vận hành nhìn trực tiếp một endpoint rồi đánh dấu, trạng thái là `USER_CALIBRATED`. Chỉ switch đã bật và đang tác động mới là `LIMIT_CONFIRMED`.

Đây là chỗ tụi mình từng loay hoay khá lâu :3 — prototype đã chạy đủ 10 chu kỳ không có nghĩa “motor chạy N giây” tự động trở thành phép đo endpoint chính xác.

## Firmware, dashboard và protocol

State machine không dùng `delay()` để chờ mưa, chờ khô hay chờ chuyển động. Hai hướng PWM loại trừ lẫn nhau; khi đổi chiều, firmware đưa cả hai về LOW rồi chờ dead-time. Runtime timeout, fault lockout, STOP latch và trạng thái persistence đều nằm ở controller, không phụ thuộc browser.

ESP8266 phục vụ HTML/CSS/JS từ LittleFS. Mỗi POST có request ID; status tách `ACCEPTED`, `STARTED`, `COMPLETED`, `STOPPED`, `REJECTED`, `FAULT`. UI bỏ qua acknowledgement sai ID, khóa double-click và không báo thành công nếu 90 giây vẫn chưa có phase kết thúc. Chi tiết ở [protocol v2](docs/PROTOCOL.md).

## Cấu trúc repository

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
2. Dùng Node.js 20+ để chạy test web; repository không cần `npm install` vì test chỉ dùng module có sẵn của Node.
3. Native test trên Windows cần `g++`/MinGW trong `PATH`.
4. Sao chép `include/secrets.example.h` thành `include/secrets.h`:

```cpp
#define WIFI_SSID "ten-wifi"
#define WIFI_PASSWORD "mat-khau"
#define CONTROL_TOKEN "mot-token-ngau-nhien-dai"
```

`include/secrets.h` nằm trong `.gitignore`. Không đưa token hoặc mật khẩu vào issue, ảnh Serial hay commit.

## Build và test

```powershell
pio run -e nodemcuv2
pio run -e nodemcuv2 -t buildfs
pio test -e native
npm test
npm run check
```

Build phần mềm đã chạy ngày 2026-08-26 cho đúng môi trường:

```ini
[env:nodemcuv2]
board = nodemcuv2
```

Kết quả: firmware PASS, LittleFS PASS, 45/45 scenario controller (132 assertions) và 11/11 test web/protocol/integration (52 assertions). Tổng cộng 56 test case/scenario và 184 assertions. RAM 31,720/81,920 byte (38.7%); flash 382,923/1,044,464 byte (36.7%). Xem [test results](docs/TEST_RESULTS.md).

## Nạp firmware và dashboard

Chỉ chọn đúng cổng sau khi nhận diện board. Lần kiểm thử mới vẫn nên bắt đầu với motor tháo rời và dry-run bật:

```powershell
pio run -e nodemcuv2 -t upload
pio run -e nodemcuv2 -t uploadfs
pio device monitor -b 115200
```

Firmware và filesystem là hai image riêng. `uploadfs` ghi lại phân vùng LittleFS nên có thể làm mất state/config đang lưu; ngắt motor và chuẩn bị hiệu chuẩn lại trước khi nạp. Khi Wi‑Fi kết nối, mở `http://greenguard.local`; nếu mDNS không chạy, dùng địa chỉ IP in ở Serial Monitor.

### Kiểm thử an toàn cho một lần nạp mới

1. Tháo motor và ngắt nguồn công suất.
2. Giữ `ACTUATOR_DRY_RUN=true`; kiểm tra RPWM/LPWM đều LOW lúc boot và khi gửi lệnh.
3. Xác nhận đúng serial port, nạp firmware và LittleFS, rồi thử rain DO, token, AUTO, MANUAL và STOP ở dry-run.
4. So sánh wiring thực tế với [hardware test checklist](docs/HARDWARE_TEST_CHECKLIST.md) trước khi dùng cấu hình actuator-enabled.
5. Chỉ thử motor khi có cách ngắt nguồn vật lý trong tầm tay và người vận hành quan sát trực tiếp.

Một mock server cho phép kiểm tra UI mà không có board:

```powershell
npm run preview
# mở http://127.0.0.1:4173 và dùng token test-token-1234
```

## Phạm vi đã xác nhận và phần còn lại

Đã xác nhận trong test 2026-08-26: firmware được nạp lên NodeMCU thật; cảm biến active-LOW; 10 chu kỳ che–thu có tải; điện áp nguồn motor, rail 3V3, dòng vận hành/khởi động, dead-time và nhiệt độ sau chu kỳ; không có reset NodeMCU trong lần test.

Vẫn cần chủ dự án bổ sung hoặc tiếp tục kiểm tra:

- Bảng wiring as-built cho từng GPIO, terminal BTS7960 và R_EN/L_EN.
- Có hay không có limit switch, cách xác định endpoint và full-travel time chính xác.
- Motor stall current và phản ứng trong các tình huống vật cản khác nhau.
- Fuse rating, tiết diện dây, emergency disconnect và bảo vệ cơ khí.
- Chạy dài hạn, số chu kỳ lớn hơn, chống mưa, thoát nước, ăn mòn và ingress protection.
- Bảo mật ngoài LAN; token HTTP hiện tại không phải TLS và không nên đưa trực tiếp ra Internet.

Các hạng mục chưa test không được suy ra từ kết quả 10 chu kỳ. Checklist tiếp theo nằm tại [hardware test checklist](docs/HARDWARE_TEST_CHECKLIST.md).

## Tài liệu kỹ thuật

- [Hardware audit](docs/HARDWARE_AUDIT.md)
- [System design](docs/SYSTEM_DESIGN.md)
- [Protocol](docs/PROTOCOL.md)
- [Hardware test checklist](docs/HARDWARE_TEST_CHECKLIST.md)
- [Automated and physical test results](docs/TEST_RESULTS.md)
- [Wiring worksheet](WIRING.md)

## Người hướng dẫn, học viên và contributor

- **Minh Quân** ([`quanle0709`](https://github.com/quanle0709) + [`nhiennguyenquoc`](https://github.com/nhiennguyenquoc)): người khởi xướng khóa học; gia sư và người hướng dẫn embedded systems; instructor, project mentor và technical guide. Minh Quân thiết kế quá trình học, giải thích nền tảng, định hướng kỹ thuật, hỗ trợ giải quyết vấn đề và giám sát tích hợp/kiểm thử.
- **[`minhkhoi092211`](https://github.com/minhkhoi092211)**: học viên, người trực tiếp xây dựng prototype và contributor có commit thật trong repository.
- Các bạn học viên trong nhóm là những người trực tiếp xây dựng, tích hợp, debug và kiểm thử GreenGuard sau ba tháng học; vai trò đó khác với vai trò dạy và mentor của Minh Quân, nhưng không phải vai trò quan sát thụ động.

GitHub attribution vẫn dựa trên các commit thật. Repository không viết lại lịch sử, tạo commit rỗng hay thêm `Co-authored-by` giả để thay đổi bảng Contributors.

## Một lời nhắn nhỏ

GreenGuard không phải một sản phẩm “hoàn hảo”; nó là bằng chứng rằng một nhóm bạn có thể bắt đầu từ GPIO và cảm biến số rồi cùng nhau làm ra một hệ thống chạy thật. Nếu bạn cũng đang học embedded, hãy đo từng thứ, debug từng lớp và đừng ngại những lúc code đúng mà dây vẫn sai 🔧. Bọn mình hy vọng hành trình này giúp bạn hình dung rõ hơn ngành embedded trông như thế nào — rồi tự tay làm một project còn hay hơn nữa nhé 🌱

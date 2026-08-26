# GreenGuard

GreenGuard là bộ rèm bảo vệ cây tự động dùng NodeMCU ESP-12E, ngõ số DO của cảm biến mưa, cầu H HW-039/BTS7960, dashboard nội bộ và ThingSpeak. Điều khiển motor, xác nhận mưa/khô, timeout và AUTO đều chạy tại chỗ; mất Internet hoặc ThingSpeak không làm mất chức năng bảo vệ.

## Tính năng

- Đọc DO định kỳ và xác nhận trạng thái mưa/khô theo thời gian, không chặn vòng lặp.
- Đóng khi WET ổn định; mở khi DRY ổn định; AUTO bị khóa nếu vị trí chưa biết hoặc có lỗi.
- Ước lượng vị trí 0% (đóng)–100% (mở), chạy phần thời gian còn lại, dừng và đảo chiều an toàn 500 ms.
- Lưu cấu hình, vị trí, trạng thái chuyển động và số sự kiện mưa bằng LittleFS theo kiểu file tạm rồi đổi tên.
- Khôi phục an toàn sau mất điện: chuyển động dang dở làm vị trí thành UNKNOWN và không tự chạy.
- Dashboard tiếng Việt responsive, REST JSON, Wi-Fi/mDNS và ThingSpeak 20 giây/lần.
- `DEMO_MODE` trong `include/config.h` vô hiệu hóa đầu ra motor thật và cho phép API mô phỏng mưa.

## Phần cứng và phần mềm

NodeMCU 1.0 ESP-12E, module cảm biến mưa có DO, motor DC 12 V, HW-039/BTS7960, nguồn 12 V phù hợp, nguồn logic 5 V, cầu chì và dây điện đúng dòng. Cài VS Code, PlatformIO IDE hoặc PlatformIO Core. Xem [WIRING.md](WIRING.md) trước khi cấp nguồn.

| Kết nối | Chức năng |
|---|---|
| D1 / GPIO5 | Rain sensor DO |
| D5 / GPIO14 | BTS7960 RPWM |
| D6 / GPIO12 | BTS7960 LPWM |
| R_EN | 5 V logic |
| L_EN | 5 V logic |
| D7 | Không sử dụng |
| A0 | Không sử dụng |

Hai chân Enable luôn được bật bằng 5 V. Firmware không điều khiển Enable và dừng motor bằng cách đưa cả RPWM lẫn LPWM về 0. Không nối R_EN/L_EN đồng thời vào 5 V và ESP8266. Cấu hình phần cứng này đã được kiểm tra bằng code test và motor đã chạy thành công.

## Cấu trúc

```text
platformio.ini, README.md, WIRING.md
include/  config.h, secrets*.h, types.h, PersistentStorage.h,
          RainSensor.h, MotorController.h, GreenGuardController.h
src/      main.cpp và ba lớp triển khai
data/     index.html, style.css, app.js
```

## Thiết lập và nạp

Sao chép/điền `include/secrets.h` (file này bị Git bỏ qua), không chia sẻ nội dung:

```cpp
#define WIFI_SSID "ten-wifi"
#define WIFI_PASSWORD "mat-khau"
#define THINGSPEAK_CHANNEL_ID 123456
#define THINGSPEAK_WRITE_API_KEY "write-key"
```

Trong ThingSpeak tạo channel với tám field: **Rain DO Level (0 LOW, 1 HIGH)**, **Rain Stable (0 dry, 1 wet)**, **Curtain Position %**, **Mode (0 auto, 1 manual)**, **Rain Event Count**, **Motor State**, **WiFi RSSI**, **Error Code**. Firmware dùng đúng một `writeFields()` mỗi chu kỳ tối thiểu 20 giây.

```powershell
pio run
pio run -t upload
pio run -t uploadfs
pio device monitor
```

Phải nạp cả firmware và LittleFS. Nếu `pio` không có trong terminal thường, dùng các nút Build, Upload, Upload Filesystem Image và Monitor trên thanh công cụ PlatformIO. Khi Wi-Fi kết nối, mở `http://greenguard.local`; nếu mDNS không hoạt động, dùng IP in trên Serial Monitor. Firmware không in mật khẩu/API key.

Khi thay đổi code firmware phải nạp lại firmware bằng `pio run -t upload`. Khi thay đổi giao diện web trong `data/` phải nạp lại LittleFS bằng `pio run -t uploadfs`.

## Hiệu chuẩn

Chỉ nối DO vào D1/GPIO5; AO để hở và A0 không dùng. Làm cảm biến khô rồi làm ướt ở mức cần phát hiện, đồng thời chỉnh biến trở trên module để DO đổi trạng thái chắc chắn. Dashboard hiển thị trực tiếp mức `HIGH/LOW` và trạng thái ngõ vào. Phần lớn module xuất LOW khi ướt nên `rainDigitalActiveLow` mặc định là `true`; nếu module của bạn xuất HIGH khi ướt, bỏ chọn mục **DO ở mức LOW khi có mưa**. Firmware vẫn yêu cầu trạng thái mưa liên tục 3 giây và khô liên tục 30 giây trước khi đổi trạng thái ổn định, giúp chống rung tín hiệu.

Đo vài lần thời gian rèm chạy hết hành trình rồi đặt `fullTravelTimeMs` (5–60 giây). Đưa rèm thật tới đầu hành trình và bấm **Đặt là Mở hoàn toàn** hoặc **Đặt là Đóng hoàn toàn**. Vị trí chỉ được suy ra từ thời gian: `thời gian = hành trình đầy đủ × khoảng cách % / 100`. Sai số tích lũy do điện áp, tải, ma sát, pin, PWM, trượt hoặc vật cản; hãy hiệu chuẩn lại khi vị trí hiển thị lệch thực tế. 30 giây chỉ là ban đầu, không tăng dư quá mức vì motor có thể tiếp tục ép cơ cấu ở cuối hành trình.

AUTO đóng ở WET ổn định và mở ở DRY ổn định. MANUAL gồm Mở, Đóng, Dừng; nút AUTO quay lại tự động. Khi vị trí UNKNOWN, mở/đóng cần xác nhận và chạy trọn hành trình; AUTO không chạy. Sau timeout: xóa lỗi, kiểm tra cơ khí, đưa rèm tới một đầu và hiệu chuẩn. Sau mất điện giữa chuyển động cũng phải hiệu chuẩn. Công tắc hành trình hoặc encoder chính xác hơn nhưng không thuộc phiên bản này; điều khiển thời gian không thể phát hiện kẹt.

## Danh sách kiểm thử bắt buộc

1. Khởi động khi tháo motor; đo RPWM và LPWM đều LOW trong khởi động. R_EN/L_EN nối cố định 5 V nên luôn HIGH; D7 không dùng.
2. Quan sát mức DO khi khô và ướt; chỉnh biến trở và `rainDigitalActiveLow`; kiểm tra xác nhận WET 3 s và DRY 30 s.
3. Đưa rèm mở hẳn, bấm **Đặt là Mở hoàn toàn**.
4. Close khoảng 2 s rồi Stop; motor phải dừng ngay và vị trí phải đổi.
5. Kiểm tra hai chiều; nếu sai bật `motorDirectionReversed`.
6. Close đầy đủ, dừng gần 30 s; hiệu chuẩn đóng; Open đầy đủ.
7. Open/Close, dừng gần 15 s: vị trí gần 50%; chạy tiếp chỉ dùng thời gian còn lại.
8. Đảo chiều và đo khoảng ngắt ít nhất 500 ms.
9. Khởi động lại khi file đánh dấu chuyển động đang chạy: vị trí UNKNOWN, AUTO không chạy; hiệu chuẩn rồi kiểm tra AUTO chạy lại.
10. Tạo mưa: tự đóng; làm khô: phải ổn định 30 s mới mở.
11. Ngắt Wi-Fi: logic AUTO vẫn hoạt động; nối lại: ThingSpeak gửi tiếp.
12. Tạo timeout: motor dừng, lỗi `MOTOR_TIMEOUT`, vị trí UNKNOWN; xóa lỗi không được tự chạy.

## Khắc phục sự cố.

- Không có dashboard: kiểm tra SSID, IP Serial, nạp `uploadfs`, cùng mạng LAN; thử IP thay mDNS.
- Motor không chạy: kiểm tra nguồn 12 V tại B+/B-, 5 V logic tại VCC và cả R_EN/L_EN, GND chung, cầu chì, hai tín hiệu PWM và lỗi dashboard.
- Chiều sai: đổi `motorDirectionReversed`, không cần đổi dây.
- Mưa đảo: đổi `rainDigitalActiveLow`; chỉnh biến trở module nếu DO dao động hoặc không chuyển mức.
- Vị trí trôi: đo lại thời gian, kiểm tra tải/nguồn và hiệu chuẩn đầu hành trình.
- ThingSpeak lỗi: kiểm tra channel ID/write key và Wi-Fi; điều khiển cục bộ vẫn tiếp tục.

Luôn thử không tải trước. Dùng dây đủ tiết diện, cầu chì và công tắc ngắt khẩn cấp. Không bao giờ nối 12 V vào NodeMCU hoặc VCC logic BTS7960.

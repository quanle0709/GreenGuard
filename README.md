# GreenGuard

GreenGuard là bộ rèm bảo vệ cây tự động dùng NodeMCU ESP-12E, cảm biến mưa analog, cầu H HW-039/BTS7960, dashboard nội bộ và ThingSpeak. Điều khiển motor, xác nhận mưa/khô, timeout và AUTO đều chạy tại chỗ; mất Internet hoặc ThingSpeak không làm mất chức năng bảo vệ.

## Tính năng

- Lọc trung bình 10 mẫu, hysteresis, xác nhận mưa/khô không chặn.
- Đóng khi WET ổn định; mở khi DRY ổn định; AUTO bị khóa nếu vị trí chưa biết hoặc có lỗi.
- Ước lượng vị trí 0% (đóng)–100% (mở), chạy phần thời gian còn lại, dừng và đảo chiều an toàn 500 ms.
- Lưu cấu hình, vị trí, trạng thái chuyển động và số sự kiện mưa bằng LittleFS theo kiểu file tạm rồi đổi tên.
- Khôi phục an toàn sau mất điện: chuyển động dang dở làm vị trí thành UNKNOWN và không tự chạy.
- Dashboard tiếng Việt responsive, REST JSON, Wi-Fi/mDNS và ThingSpeak 20 giây/lần.
- `DEMO_MODE` trong `include/config.h` vô hiệu hóa đầu ra motor thật và cho phép API mô phỏng mưa.

## Phần cứng và phần mềm

NodeMCU 1.0 ESP-12E, cảm biến mưa AO, motor DC 12 V, HW-039/BTS7960, nguồn 12 V phù hợp, nguồn logic 5 V, cầu chì và dây điện đúng dòng. Cài VS Code, PlatformIO IDE hoặc PlatformIO Core. Xem [WIRING.md](WIRING.md) trước khi cấp nguồn.

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

Trong ThingSpeak tạo channel với tám field: **Rain Filtered**, **Rain Stable (0 dry, 1 wet)**, **Curtain Position %**, **Mode (0 auto, 1 manual)**, **Rain Event Count**, **Motor State**, **WiFi RSSI**, **Error Code**. Firmware dùng đúng một `writeFields()` mỗi chu kỳ tối thiểu 20 giây.

```powershell
pio run
pio run -t upload
pio run -t uploadfs
pio device monitor
```

Phải nạp cả firmware và LittleFS. Nếu `pio` không có trong terminal thường, dùng các nút Build, Upload, Upload Filesystem Image và Monitor trên thanh công cụ PlatformIO. Khi Wi-Fi kết nối, mở `http://greenguard.local`; nếu mDNS không hoạt động, dùng IP in trên Serial Monitor. Firmware không in mật khẩu/API key.

## Hiệu chuẩn

Đọc giá trị thô/lọc khi cảm biến **khô hoàn toàn**, sau đó khi **ướt thực tế**. Chọn `rainValueIncreasesWhenWet` đúng chiều. Với cảm biến giảm khi ướt, `wetThreshold` phải nhỏ hơn `dryThreshold`; với cảm biến tăng khi ướt thì ngược lại. Khoảng giữa hai ngưỡng là hysteresis, ngăn trạng thái rung quanh một điểm. Mặc định 500/650 chỉ là giá trị khởi đầu và bắt buộc phải đo lại.

Đo vài lần thời gian rèm chạy hết hành trình rồi đặt `fullTravelTimeMs` (5–60 giây). Đưa rèm thật tới đầu hành trình và bấm **Đặt là Mở hoàn toàn** hoặc **Đặt là Đóng hoàn toàn**. Vị trí chỉ được suy ra từ thời gian: `thời gian = hành trình đầy đủ × khoảng cách % / 100`. Sai số tích lũy do điện áp, tải, ma sát, pin, PWM, trượt hoặc vật cản; hãy hiệu chuẩn lại khi vị trí hiển thị lệch thực tế. 30 giây chỉ là ban đầu, không tăng dư quá mức vì motor có thể tiếp tục ép cơ cấu ở cuối hành trình.

AUTO đóng ở WET ổn định và mở ở DRY ổn định. MANUAL gồm Mở, Đóng, Dừng; nút AUTO quay lại tự động. Khi vị trí UNKNOWN, mở/đóng cần xác nhận và chạy trọn hành trình; AUTO không chạy. Sau timeout: xóa lỗi, kiểm tra cơ khí, đưa rèm tới một đầu và hiệu chuẩn. Sau mất điện giữa chuyển động cũng phải hiệu chuẩn. Công tắc hành trình hoặc encoder chính xác hơn nhưng không thuộc phiên bản này; điều khiển thời gian không thể phát hiện kẹt.

## Danh sách kiểm thử bắt buộc

1. Khởi động khi tháo motor; đo RPWM, LPWM, ENABLE đều LOW trong khởi động.
2. Xem raw; ghi số khô và ướt; đặt hai ngưỡng/hướng; kiểm tra xác nhận WET 3 s và DRY 30 s.
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

## Khắc phục sự cố

- Không có dashboard: kiểm tra SSID, IP Serial, nạp `uploadfs`, cùng mạng LAN; thử IP thay mDNS.
- Motor không chạy: kiểm tra nguồn 12 V tại B+/B-, 5 V logic tại VCC, GND chung, cầu chì, ENABLE và lỗi dashboard.
- Chiều sai: đổi `motorDirectionReversed`, không cần đổi dây.
- Mưa đảo: đổi `rainValueIncreasesWhenWet`, sau đó đặt lại ngưỡng hợp lệ.
- Vị trí trôi: đo lại thời gian, kiểm tra tải/nguồn và hiệu chuẩn đầu hành trình.
- ThingSpeak lỗi: kiểm tra channel ID/write key và Wi-Fi; điều khiển cục bộ vẫn tiếp tục.

Luôn thử không tải trước. Dùng dây đủ tiết diện, cầu chì và công tắc ngắt khẩn cấp. Không bao giờ nối 12 V vào NodeMCU hoặc VCC logic BTS7960.

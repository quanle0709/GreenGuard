# Đấu nối GreenGuard

> **Ngắt nguồn khi đấu dây.** Động cơ dùng nguồn 12 V riêng; tuyệt đối không đưa 12 V vào GPIO, A0, 3V3 hay chân VCC logic của BTS7960. Tất cả GND phải nối chung.

| Chân NodeMCU / nguồn | GPIO | Thiết bị | Chân kết nối | Mục đích | Lưu ý điện |
|---|---:|---|---|---|---|
| D5 | 14 | BTS7960 | RPWM | PWM một chiều | Logic 3,3 V |
| D6 | 12 | BTS7960 | LPWM | PWM chiều ngược | Không bao giờ bật đồng thời D5 |
| D7 | 13 | BTS7960 | R_EN và L_EN | Cho phép cầu H | Nối chung hai EN |
| GND | — | BTS7960 | GND | Mass logic | Phải chung mass nguồn 12 V |
| 5 V | — | BTS7960 | VCC | Nguồn logic | Đây không phải đầu vào motor 12 V |
| A0 | ADC | Cảm biến mưa | AO | Tín hiệu analog | Không nối DO; không vượt dải A0 của board |
| 3V3 | — | Cảm biến mưa | VCC | Nguồn cảm biến | Giảm nguy cơ quá áp A0 |
| GND | — | Cảm biến mưa | GND | Mass cảm biến | Chung mass |
| Nguồn +12 V | — | BTS7960 | B+ | Nguồn công suất | Lắp cầu chì phù hợp |
| Nguồn 0 V | — | BTS7960 | B- | Nguồn công suất | Nối chung GND logic |
| Hai dây motor | — | BTS7960 | M+, M- | Đầu ra động cơ | Dây đủ lớn theo dòng motor |

Kiểm tra đúng nhãn in trên bo HW-039 vì bố trí chân có thể khác. Thử motor khi chưa gắn rèm. Nếu cấp NodeMCU từ cùng nguồn 12 V, dùng bộ DC-DC và chỉnh khoảng 5 V **trước** khi nối NodeMCU. Điều khiển theo thời gian không phát hiện được vật cản hoặc kẹt cơ khí; nên có cầu chì và công tắc ngắt nguồn khẩn cấp vật lý.


# Đấu nối GreenGuard

> **Ngắt nguồn khi đấu dây.** Động cơ dùng nguồn 12 V riêng; tuyệt đối không đưa 12 V vào GPIO, A0, 3V3 hay chân VCC logic của BTS7960. Tất cả GND phải nối chung.

| Chân NodeMCU / nguồn | GPIO | Thiết bị | Chân kết nối | Mục đích | Lưu ý điện |
|---|---:|---|---|---|---|
| D5 | 14 | BTS7960 | RPWM | PWM một chiều | Logic 3,3 V |
| D6 | 12 | BTS7960 | LPWM | PWM chiều ngược | Không bao giờ bật đồng thời D5 |
| GND | — | BTS7960 | GND | Mass logic | Phải chung mass nguồn 12 V |
| 5 V | — | BTS7960 | VCC | Nguồn logic | Đây không phải đầu vào motor 12 V |
| 5 V | — | BTS7960 | R_EN và L_EN | Luôn cho phép cầu H | Nối cả hai EN trực tiếp vào 5 V logic; không nối D7 |
| D1 | 5 | Cảm biến mưa | DO | Tín hiệu mưa số | Mặc định LOW khi có mưa; có thể đảo trong cấu hình |
| 3V3 | — | Cảm biến mưa | VCC | Nguồn cảm biến | Chỉ cấp 3,3 V |
| GND | — | Cảm biến mưa | GND | Mass cảm biến | Chung mass |
| Không nối | — | Cảm biến mưa | AO | Không sử dụng | Để hở |
| A0 | ADC | — | — | Không sử dụng | Để hở |
| D7 | 13 | — | — | Không sử dụng | Không nối vào R_EN/L_EN |
| Nguồn +12 V | — | BTS7960 | B+ | Nguồn công suất | Lắp cầu chì phù hợp |
| Nguồn 0 V | — | BTS7960 | B- | Nguồn công suất | Nối chung GND logic |
| Hai dây motor | — | BTS7960 | M+, M- | Đầu ra động cơ | Dây đủ lớn theo dòng motor |

R_EN và L_EN luôn ở mức HIGH do nối 5 V; firmware dừng motor bằng cách đưa **cả RPWM và LPWM về 0**. D7 không được dùng. Vì firmware không thể hạ EN, công tắc ngắt nguồn khẩn cấp vật lý càng quan trọng.

Không nối D7 vào R_EN hoặc L_EN. Không đồng thời nối R_EN/L_EN vào 5 V và GPIO ESP8266, và tuyệt đối không đưa 5 V vào GPIO ESP8266. R_EN và L_EN phải nối trực tiếp nguồn logic 5 V; BTS7960 VCC cũng là nguồn logic 5 V. B+ và B- là đầu nguồn motor 12 V, không phải nguồn logic.

Chỉ dùng ngõ số **DO** của cảm biến mưa: DO nối D1/GPIO5. Để AO hở, không nối A0. Chỉnh biến trở trên module để DO đổi trạng thái ổn định tại mức ướt mong muốn; quan sát đèn báo DO nếu module có trang bị.

Kiểm tra đúng nhãn in trên bo HW-039 vì bố trí chân có thể khác. Motor sử dụng nguồn 12 V riêng và tất cả GND phải nối chung. Thử motor khi chưa gắn rèm. Nếu cấp NodeMCU từ cùng nguồn 12 V, dùng bộ DC-DC và chỉnh khoảng 5 V **trước** khi nối NodeMCU. Không đưa 12 V vào NodeMCU. Nên có cầu chì trên đường nguồn 12 V và công tắc ngắt nguồn motor khẩn cấp. Điều khiển bằng thời gian không phát hiện được motor bị kẹt hoặc vật cản cơ khí.

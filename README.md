# 🤖 Autonomous Maze Solving Robot (Micromouse 2026)

[![PlatformIO](https://img.shields.io/badge/Build-PlatformIO-orange.svg)](https://platformio.org/)
[![Hardware](https://img.shields.io/badge/Hardware-Raspberry_Pi_Pico_W-red.svg)](https://www.raspberrypi.com/products/raspberry-pi-pico/)
[![Algorithm](https://img.shields.io/badge/Algorithm-LSRB_Path_Simplification-blue.svg)]()

Dự án Robot tự hành giải mê cung (Micromouse) tốc độ cao. Hệ thống được phát triển trên vi điều khiển lõi kép **Raspberry Pi Pico W**, kết hợp hệ thống đo lường Odometry từ Encoder và nhận thức không gian bằng cảm biến Laser ToF.

## ✨ Tính năng nổi bật (Core Features)
- **Thuật toán LSRB (Left-Right-Straight-Back):** Tự động khám phá mê cung bằng quy tắc bám tường trái, ghi nhớ hành trình và rút gọn ngõ cụt để tìm ra quỹ đạo tối ưu.
- **Odometry & Dead Reckoning:** Loại bỏ hoàn toàn hàm `delay()`. Mọi chuyển động (tiến 1 ô, xoay 90 độ) được đo lường chính xác tuyệt đối qua tín hiệu xung từ Encoder.
- **PID Wall-Following:** Thuật toán PID giữ robot luôn đi thẳng ở tâm đường bằng cách tính toán sai số từ 2 mắt Laser hai bên hông.
- **Finite State Machine (FSM):** Quản lý vòng đời robot thông minh qua 4 trạng thái: `WAITING` -> `EXPLORING` (Dò đường) -> `FINISHED` -> `SPEEDRUN` (Bứt tốc).

## 🛠️ Phần cứng (Hardware Stack)
- **MCU:** Raspberry Pi Pico W (Lập trình C/C++).
- **Sensors:** 4x Cảm biến khoảng cách Laser VL53L0X (I2C) với hệ thống cấp phát địa chỉ động (XSHUT).
- **Actuators:** 2x Động cơ giảm tốc GA25-370 12V (Tỉ số truyền 1:45) tích hợp Hall Encoder.
- **Motor Driver:** Module TB6612FNG (Điều khiển PWM băm xung độc lập).
- **Power:** Pin LiPo 3S 12V qua mạch hạ áp DC-DC LM2596 (Hạ xuống 5V nuôi Pico).

## 🚀 Cài đặt & Nạp Code (Installation & Build)
Dự án này được quản lý và xây dựng bằng **PlatformIO**. 

1. Clone repository về máy:
   ```bash
   git clone https://github.com/minhtenlado/robot_2026.git

Mở thư mục dự án bằng VS Code có cài sẵn extension PlatformIO IDE.

Nạp thư viện: PlatformIO sẽ tự động tải các thư viện cần thiết (như Adafruit_VL53L0X) dựa trên file platformio.ini.

Kết nối Pico W qua cổng USB và nhấn nút Upload (mũi tên chỉ sang phải ở thanh trạng thái dưới cùng).

⚙️ Cấu hình tinh chỉnh (Calibration)
Trước khi chạy trên sa bàn thực tế, vui lòng tinh chỉnh các hằng số sau trong code (dựa trên ma sát và pin thực tế):

TICKS_PER_90_DEG: Số xung Encoder để xe xoay tại chỗ đúng 90 độ.

TICKS_PER_CELL: Số xung Encoder để xe đi lọt 1 ô mê cung (ví dụ 180mm).


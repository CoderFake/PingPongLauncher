# Máy bắn bóng bàn: ESP32 + MQTT + React

```
pingpong-launcher/
├── firmware/PingPongLauncher/      Sketch Arduino (tên thư mục = tên .ino)
│   ├── PingPongLauncher.ino
│   └── src/
│       ├── config/AppConfig.h      Chân, PWM, giới hạn, broker, giá trị mặc định
│       ├── core/                   LauncherState, StateCodec (JSON + chống trùng), LauncherController
│       ├── hardware/               Flywheel (AOD4184), ServoDriver, Turret, Feeder
│       ├── net/                    WifiController (AP/STA), ConfigPortal, PortalAssets.h (sinh tự động), MqttLink
│       ├── ota/                    OtaUpdater (HTTPS + SHA-256), OtaGuard (rollback), OtaCoordinator, LocalOta
│       ├── storage/                CredentialStore (NVS)
│       └── input/                  ResetButton (giữ BOOT 3 giây)
├── firmware/portal/                index.html, style.css, app.js của trang cài WiFi
├── firmware/tools/build_portal.py  Nhúng portal/* vào PortalAssets.h
├── .github/workflows/              Build firmware và tạo Release khi đẩy tag v*
└── web/                            React + Vite + Tailwind v4 + mqtt.js
    └── src/
        ├── config/  types/  utils/  services/  hooks/
        └── components/{ui,layout,power,flywheel,turret,feed}
```

## Nối dây

| Thiết bị | GPIO | Nguồn |
|---|---|---|
| AOD4184 bánh trên (SB1) | 18 | 12V |
| AOD4184 bánh dưới (SB2) | 19 | 12V |
| MG996R nâng bóng | 25 | 6V (buck riêng, ≥5A) |
| MG996R xoay cổ nòng | 26 | 6V (buck riêng, ≥5A) |
| Nút reset WiFi | 0 (BOOT) | — |
| LED trạng thái | 2 | — |

Tất cả GND nối chung một điểm. Mỗi servo đặt thêm tụ 470–1000µF, mỗi ZWF-30 đặt tụ 1000µF và diode SS34.

## Firmware

1. Trong Arduino IDE, cài board **esp32 by Espressif**, bản 4.x (đã build thử trên 4.0.0). Bản 3.x vẫn chạy nhưng OTA sẽ không kiểm chứng chứng chỉ TLS.
2. Vào Library Manager, cài **PubSubClient** (Nick O'Leary) và **ArduinoJson** (7.x).
3. Mở `firmware/PingPongLauncher/PingPongLauncher.ino`, chọn board *ESP32 Dev Module*.
4. Bắt buộc chọn **Tools → Partition Scheme → Minimal SPIFFS (1.9MB APP with OTA)**. Với phân vùng mặc định, bản build chiếm tới 99% và không cập nhật OTA được. Lần nạp đầu tiên phải qua cáp USB để ghi bảng phân vùng này.
5. Các thông số cần cân chỉnh theo cơ khí thật nằm trong `AppConfig.h`: `FEED_DOWN_DEG`, `FEED_UP_DEG`, `TURRET_CENTER_DEG`, `TURRET_INVERT`.

### Sửa giao diện trang cài WiFi

Chỉ sửa các file trong `firmware/portal/`, sau đó chạy:

```bash
python3 firmware/tools/build_portal.py
```

Script này sinh lại `src/net/PortalAssets.h`. Không sửa trực tiếp file đó.

### Cài WiFi lần đầu

1. Máy phát WiFi tên `PingPong-XXXXXX`, mật khẩu `pingpong`.
2. Điện thoại kết nối vào là trang cài đặt tự bật lên. Nếu không tự bật, mở `http://192.168.4.1`.
3. Chọn mạng, nhập mật khẩu (mạng mở thì bấm Kết nối luôn). Máy tự lấy IP qua DHCP, hiện IP và **mã thiết bị**, sau khoảng 8 giây thì tắt điểm phát và chuyển hẳn sang chế độ STA.
4. Thông tin WiFi được lưu trong NVS. Muốn xóa, giữ nút BOOT 3 giây, máy sẽ khởi động lại vào trang cài đặt.
5. Nếu mạng đã lưu không kết nối được trong 15 giây, trang cài đặt tự mở lại. Thông tin cũ vẫn giữ, và khi router lên lại thì máy tự kết nối.

## Cập nhật firmware qua OTA

### OTA qua GitHub Release, bấm từ trang web

1. Đẩy một tag phiên bản lên GitHub:
   ```bash
   git tag v1.1.0 && git push origin v1.1.0
   ```
2. GitHub Actions sẽ build và tạo Release gồm 2 file:
   - `PingPongLauncher-1.1.0.bin`: dùng cho OTA.
   - `…-full-usb.merged.bin`: bản đầy đủ, nạp qua cáp USB.
3. Trên trang web, mục **Firmware** hiện bản đang chạy và bản mới nhất. Tắt nguồn máy rồi bấm **Cập nhật**.
4. ESP32 tự tải file qua HTTPS, kiểm tra SHA-256 (lấy từ `digest` của asset trên GitHub), ghi vào vùng OTA còn trống rồi khởi động lại.

Cơ chế an toàn:
- Firmware chỉ nhận link bắt đầu bằng `OTA_URL_PREFIX` (trỏ tới Releases của repo này).
- Phải khớp SHA-256 thì mới ghi vào bộ nhớ.
- Máy đang bật nguồn bắn thì từ chối cập nhật.
- Bản mới chạy ở chế độ thử. Nếu sau 3 lần khởi động mà vẫn chưa kết nối được MQTT, máy tự quay về bản cũ.

### OTA trong mạng LAN, dùng Arduino IDE

Khi máy tính và ESP32 ở chung một mạng WiFi, vào **Tools → Port** sẽ thấy cổng mạng `pingpong-XXXXXX`. Chọn cổng đó rồi bấm Upload như bình thường. Mật khẩu là `pingpong-ota`, đổi trong `AppConfig.h`.

## Web

```bash
cd web
npm install
npm run dev
```

Mở trang web, nhập mã thiết bị 6 ký tự. Broker mặc định là `wss://broker.emqx.io:8084/mqtt`. Muốn đổi thì sao chép `.env.example` thành `.env`.

## MQTT

Broker công cộng EMQX: ESP32 dùng `tcp://broker.emqx.io:1883`, web dùng WSS cổng 8084.

| Topic | Chiều | Nội dung |
|---|---|---|
| `ppl/<id>/cmd` | web → máy | `{"id":"…","sb1":75}`, QoS 1, không retain |
| `ppl/<id>/state` | máy → web | Toàn bộ trạng thái kèm `rev`, `boot`, retain |
| `ppl/<id>/status` | máy → web | `online` / `offline` (LWT), retain |

Các khóa lệnh: `power`, `sb1`, `sb2`, `sbAll`, `mode` (`sweep|random|fixed`), `range`, `angle`, `feed`, `get`, `ota` (`{url, sha256, version}`).

Topic `ppl/<id>/ota` (không retain) báo tiến trình cập nhật: `queued`, `downloading` (kèm %), `rebooting`, `error`.

### Chống trùng lặp và đồng bộ hai chiều

- Mỗi lệnh có `id` ngẫu nhiên. Firmware nhớ 24 id gần nhất và bỏ qua bản gửi lại (QoS 1 có thể giao trùng).
- Lệnh không làm thay đổi gì thì không tăng `rev` và không publish lại.
- Firmware gom các thay đổi, publish state tối đa khoảng 12 lần/giây.
- Thanh trượt gửi tối đa 1 lần mỗi 150ms, không gửi lại giá trị vừa gửi, và giữ giá trị đang kéo cho tới khi máy gửi xác nhận.
- Web bỏ qua state có `rev` cũ hơn trong cùng một `boot`. Khi máy khởi động lại, `boot` đổi nên web nhận lại từ đầu.
- Máy là nguồn sự thật duy nhất, nên mọi tab hoặc điện thoại đang mở đều thấy cùng một trạng thái.

Lưu ý: broker công cộng ai cũng đọc được. Chỉ biết mã thiết bị là gửi lệnh được, nên đừng dùng cho việc gì cần bảo mật.

# Máy bắn bóng bàn: ESP32 + MQTT + React

```
pingpong-launcher/
├── firmware/PingPongLauncher/      Sketch Arduino (tên thư mục = tên .ino)
│   ├── PingPongLauncher.ino
│   └── src/
│       ├── config/AppConfig.h      Chân, PWM, giới hạn, broker, giá trị mặc định
│       ├── core/                   LauncherState, StateCodec (JSON + chống trùng), LauncherController
│       ├── hardware/               Flywheel (AOD4184), ServoDriver, Turret, Feeder
│       ├── net/                    WifiController (AP/STA), ConfigPortal, PortalAssets.h (sinh tự động), MqttLink,
│       │                           LocalControl (HTTP API + web nhúng), WebAppAssets.h (sinh tự động)
│       ├── ota/                    OtaUpdater (HTTPS + SHA-256), OtaGuard (rollback), OtaCoordinator, LocalOta
│       ├── ui/                     Lcd1602 (PCF8574, tự dò), Buzzer (giai điệu không chặn), Feedback
│       ├── storage/                CredentialStore (NVS)
│       └── input/                  ResetButton (giữ BOOT 3 giây), Button, ControlPanel (4 nút + menu)
├── firmware/portal/                index.html, style.css, app.js của trang cài WiFi
├── firmware/tools/build_portal.py  Nhúng portal/* vào PortalAssets.h
├── firmware/tools/build_webapp.py  Nhúng web/dist-device vào WebAppAssets.h
├── .github/workflows/              Build firmware và tạo Release khi đẩy tag v*
└── web/                            React + Vite + Tailwind v4 + mqtt.js
    └── src/
        ├── config/  types/  utils/  services/  hooks/
        └── components/{ui,layout,power,flywheel,turret,feed}
```

## Nối dây

| Thiết bị | GPIO | Nguồn |
|---|---|---|
| AOD4184 bánh trên (SB1) | 18 | 12V từ boost 5→12V |
| AOD4184 bánh dưới (SB2) | 19 | 12V từ boost 5→12V |
| Dây đọc lại PWM bánh trên (từ chân PWM của AOD4184 SB1) | 16 | — |
| Dây đọc lại PWM bánh dưới (từ chân PWM của AOD4184 SB2) | 17 | — |
| MG996R nâng bóng (cam / đỏ / nâu) | 25 | 5V từ nguồn 5V 20A |
| MG996R xoay cổ nòng (cam / đỏ / nâu) | 26 | 5V từ nguồn 5V 20A |
| Nút POWER (bật/tắt bắn) | 32 | nối xuống GND |
| Nút MENU (chọn thông số) | 33 | nối xuống GND |
| Nút − | 13 | nối xuống GND |
| Nút + | 14 | nối xuống GND |
| Nút reset WiFi | 0 (BOOT) | — |
| LED trạng thái | 2 | — |
| Còi MH-FMD (kích mức thấp), chân I/O | 27 | **3.3V** |
| LCD 16x2 I2C (PCF8574), SDA / SCL | 21 / 22 | 5V (tùy chọn) |

**Nguồn:** nguồn 5V 20A cấp cho VIN của ESP32, 2 servo và LCD. Mạch boost 5→12V (≥5A) lấy từ nguồn 5V và cấp riêng cho VIN+ của 2 AOD4184. Không cấp servo từ chân 5V của ESP32 hay từ cổng USB: MG996R kéo 1–2.5A lúc khởi động, làm sụt áp và ESP32 tự reset.

**Khóa an toàn bánh đà:** piston chỉ đẩy khi cả 2 bánh đà được đặt tốc độ lớn hơn 0 và firmware đọc lại được xung PWM ở **ngay chân PWM trên module AOD4184**. Từ chân PWM của mỗi module, nối thêm 1 dây về GPIO16 (SB1) và GPIO17 (SB2). Nếu dây PWM từ ESP32 sang module bị rút hoặc lỏng, chân đọc lại không thấy xung, firmware không cho piston chạy, LCD báo `No PWM: top`, `No PWM: bottom` hoặc `No PWM: both`, và Serial in `[Launcher] Feeding blocked`. Không dùng tính năng này thì đặt `FLYWHEEL_SENSE_ENABLED = false`.

**Nút nhấn:** dùng nút nhấn nhả 2 chân, một chân vào GPIO, chân kia xuống GND. Firmware bật điện trở kéo lên bên trong và tự chống dội, không cần linh kiện ngoài.

**Còi:** cấp VCC từ chân **3.3V**, không dùng 5V. Module kích bằng transistor PNP: nếu cấp 5V, mức HIGH 3.3V của ESP32 không tắt được transistor nên còi kêu liên tục. Đây là còi thụ động, kêu to nhất ở 2–5kHz, nên mọi giai điệu được đặt trong dải 2–4.2kHz.

**LCD:** có thể cắm hoặc không cắm. Firmware dò địa chỉ 0x27 (mặc định của module này), sau đó thử 0x3F. Việc dò chạy lúc khởi động và dò lại mỗi 3 giây, nên cắm hoặc rút LCD khi máy đang chạy cũng không sao. Một số lưu ý:
- LCD cần nguồn 5V để đủ sáng.
- Board I2C phía sau LCD có điện trở kéo lên 5V ở SDA/SCL, mà chân ESP32 không chịu được 5V. Nên gỡ 2 điện trở kéo lên đó, hoặc dùng mạch chuyển mức 3.3V–5V.
- Nếu màn sáng mà không thấy chữ, vặn biến trở chỉnh độ tương phản ở sau lưng LCD.

Tất cả GND nối chung một điểm. Mỗi servo đặt thêm tụ 470–1000µF, mỗi ZWF-30 đặt tụ 1000µF và diode SS34.

## Firmware

1. Trong Arduino IDE, cài board **esp32 by Espressif**, bản 4.x (đã build thử trên 4.0.0). Bản 3.x vẫn chạy nhưng OTA sẽ không kiểm chứng chứng chỉ TLS.
2. Vào Library Manager, cài **PubSubClient** (Nick O'Leary) và **ArduinoJson** (7.x).
3. Mở `firmware/PingPongLauncher/PingPongLauncher.ino`, chọn board *ESP32 Dev Module*.
4. Bắt buộc chọn **Tools → Partition Scheme → Minimal SPIFFS (1.9MB APP with OTA)**. Với phân vùng mặc định, bản build chiếm tới 99% và không cập nhật OTA được. Lần nạp đầu tiên phải qua cáp USB để ghi bảng phân vùng này.
5. Các thông số cần cân chỉnh theo cơ khí thật nằm trong `AppConfig.h`: `FEED_DOWN_DEG`, `FEED_UP_DEG`, `TURRET_CENTER_DEG`, `TURRET_INVERT`.

### Cân chỉnh servo nâng bóng

Các thông số nằm trong `AppConfig.h`:

| Hằng số | Mặc định | Ý nghĩa |
|---|---|---|
| `FEED_DOWN_DEG` / `FEED_UP_DEG` | 0° / 180° | Góc nghỉ và góc đẩy |
| `SERVO_MS_PER_60DEG` | 200 | Thời gian servo quay 60°. MG996R ở 5V khoảng 190–200ms, ở 6V khoảng 150ms |
| `FEED_HOLD_UP_MS` | 120 | Thời gian giữ ở đỉnh sau khi đã quay xong |
| `FEED_SETTLE_DOWN_MS` | 250 | Thời gian chờ quả kế tiếp lăn vào sau khi hạ |
| `FEED_RATE_MAX` | 35 | Số quả/phút tối đa, để servo luôn quay hết hành trình |

Cơ cấu tay quay: tay servo 10cm, thanh nối 10cm. Servo đi từ góc nghỉ 0° tới góc đẩy 180° (hết hành trình); thời gian đi và về tính theo hành trình với `SERVO_MS_PER_60DEG = 300` (khoảng 0.9 giây mỗi chiều) để servo kịp tới đích khi có tải, nên tốc độ lên bóng thực tế tối đa khoảng 27 quả/phút. Lắp tay servo khi servo đang ở góc nghỉ 0°. Với hai thanh dài bằng nhau đặt thẳng hàng, khoảng cách trục servo đến chốt piston là 20 × cos(góc tay quay), thanh nối nghiêng đúng bằng góc tay quay. Vì vậy phải đặt servo sao cho piston và thân servo không nằm trên đường quét của nhau trong cả hành trình 170°.

Thời gian đi lên và đi xuống được tính từ góc hành trình và `SERVO_MS_PER_60DEG`, nên piston luôn quay hết góc trước khi đổi chiều. Muốn bắn nhanh hơn 35 quả/phút thì thu hẹp hành trình hoặc cấp 6V cho servo, sau đó tăng `FEED_RATE_MAX` (và sửa `feed.max` trong `web/src/config/limits.ts` cho khớp).

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

## Nút vật lý

| Nút | Thao tác |
|---|---|
| POWER | Bấm để bật hoặc tắt bắn, dùng được ở mọi màn hình |
| MENU | Bấm để chuyển thông số: SB1 → SB2 → Cả hai → Chế độ nòng → Góc quét/góc cố định → Tốc độ lên bóng → màn hình chính. Giữ 1 giây để về màn hình chính |
| − / + | Chỉnh thông số đang hiện. Giữ để chỉnh liên tục, giữ quá 1.5 giây thì mỗi bước là 5 |

Không bấm gì trong 6 giây thì LCD tự về màn hình chính. Thay đổi từ nút được đưa vào cùng trạng thái như lệnh từ web (tăng `rev`), nên web MQTT và web nội bộ cập nhật ngay.

Màn hình chỉnh thông số:

```
Top wheel    80%      Sweep range +-25°     Turret mode
████████████----      ████------------      - Random      +
```

## LCD và còi báo trạng thái

| Tình huống | LCD (tiếng Anh) | Còi |
|---|---|---|
| Khởi động | `PingPong Launch` / `v1.2.0   A1B2C3` | 2 nốt lên ngắn |
| Cần cài WiFi | Luân phiên `Setup: join WiFi` / `PingPong-A1B2C3` ↔ màn hình chính với `Web 192.168.4.1` | 2 tiếng bíp |
| Đang vào mạng | `Joining WiFi...` / tên mạng | — |
| Vào mạng thất bại | `WiFi failed` / `Wrong password`… | 1 tiếng trầm dài |
| Đã vào mạng | `WiFi connected` / IP | 3 nốt đi lên |
| Kết nối MQTT | — | 1 tiếng bíp cao |
| Mất MQTT | — | 2 nốt đi xuống |
| Máy tắt | `OFF 80/60 30/m` / `Sweep +-25` ↔ IP | — |
| Bật nguồn | `ON  80/60 30/m` / `Spinning up...` | 1 tiếng bíp |
| Bánh đà đủ tốc | `ON  80/60 30/m` / `Sweep +-25#   42` | 2 tiếng bíp cao |
| Tắt nguồn | — | 2 nốt đi xuống |
| Bấm nút chỉnh | Màn hình chỉnh thông số, có thanh mức | Tích ngắn; chạm giới hạn thì bíp trầm |
| Cập nhật OTA | `Updating fw...` / thanh tiến trình % | Bíp khi bắt đầu, 4 nốt đi lên khi xong |
| OTA lỗi | `Update failed` / mã lỗi | 2 tiếng trầm |
| Giữ nút BOOT | `Hold to reset` / `WiFi in 2s...` | Tích mỗi giây, bíp dài khi xóa WiFi |

Dòng 1 màn hình chính: nguồn, tốc độ bánh trên/bánh dưới (%), tốc độ lên bóng (quả/phút). Dòng 2: chế độ nòng và số bóng đã bắn.

## Điều khiển trực tiếp (không cần internet)

Firmware chứa sẵn bản web giống hệt bản MQTT, chạy thẳng trên ESP32. Lệnh đi từ điện thoại tới máy qua HTTP, không qua broker, nên không bị ảnh hưởng khi mạng chặn MQTT hoặc broker công cộng chập chờn.

- **Cùng mạng WiFi với máy:** mở `http://<IP của máy>/app/` (IP hiện trên LCD và Serial) hoặc `http://pingpong.local/app/`.
- **Không có WiFi** (ví dụ ở sân tập): khi không vào được mạng đã lưu, máy phát WiFi `PingPong-XXXXXX` (mật khẩu `pingpong`). Kết nối vào, trên trang cài đặt bấm **Điều khiển trực tiếp**, hoặc mở `http://192.168.4.1/app/`. Điện thoại có thể báo "không có internet", hãy chọn giữ kết nối.

Bản nội bộ dùng cùng cơ chế xác nhận (ack + `rev`) với bản MQTT. Phần cập nhật firmware bị ẩn vì cần internet. Web MQTT, web nội bộ và nút vật lý dùng song song được.

Sau khi sửa code web, sinh lại bản nhúng rồi nạp lại firmware:

```bash
cd web && npm run build:device && cd ..
python3 firmware/tools/build_webapp.py
```

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
| `ppl/<id>/ack` | máy → web | `{id, rev, boot, result}` cho từng lệnh, không retain |

Các khóa lệnh: `power`, `sb1`, `sb2`, `sbAll`, `mode` (`sweep|random|fixed`), `range`, `angle`, `feed`, `get`, `ota` (`{url, sha256, version}`).

Topic `ppl/<id>/ota` (không retain) báo tiến trình cập nhật: `queued`, `downloading` (kèm %), `rebooting`, `error`.

### Chống trùng lặp và đồng bộ hai chiều

Phía web xử lý bằng `web/src/sync/syncEngine.ts`, phía firmware bằng `AckQueue`.

1. **Máy là nguồn sự thật.** Giá trị bạn vừa chỉnh được lưu riêng như một thay đổi đang chờ và hiển thị đè lên trạng thái máy gửi về. Nhờ vậy thanh trượt không bị nhảy về số cũ khi state cũ đến muộn.
2. **Gom lệnh.** Mỗi 120ms, các thay đổi chưa gửi được gom thành một lệnh có `id` riêng. Thanh "chỉnh chung" gửi `sb1` và `sb2` trong cùng một lệnh.
3. **Máy xác nhận.** Với mỗi lệnh nhận được, firmware gửi `ppl/<id>/ack` gồm `{id, rev, boot, result}`, kể cả khi lệnh bị trùng hoặc không làm thay đổi gì.
4. **Khi nào coi là xong.** Một thay đổi chỉ được coi là đã đồng bộ khi web nhận được ack của đúng `id` đó, **và** sau đó nhận được state có `rev` lớn hơn hoặc bằng `rev` trong ack (cùng `boot`). Lúc đó web chuyển sang hiển thị số của máy.
5. **Gửi lại.** Sau 2.5 giây không thấy ack, web gửi lại lệnh với **cùng `id`**. Firmware nhận ra trùng `id` nên không áp dụng lần hai, chỉ gửi ack lại. Sau 3 lần mà vẫn không có ack, web bỏ thay đổi đó và hiện cảnh báo.
6. **Kéo liên tục.** Khi bạn đang kéo thanh trượt, ack và state của các lệnh cũ bị bỏ qua, chỉ giá trị cuối cùng được chờ xác nhận.
7. **Bỏ state cũ.** State có `rev` nhỏ hơn state mới nhất (cùng `boot`) bị bỏ qua. Khi máy khởi động lại (`boot` đổi), web nhận state mới từ đầu.
8. **Dấu chấm cam** cạnh giá trị nghĩa là máy chưa xác nhận giá trị đó.

Lưu ý: broker công cộng ai cũng đọc được. Chỉ biết mã thiết bị là gửi lệnh được, nên đừng dùng cho việc gì cần bảo mật.

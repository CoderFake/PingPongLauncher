# Sơ đồ máy bắn bóng bàn

## 1. Đấu nối chân

```mermaid
flowchart LR
  PSU["Nguồn 5V 20A"]
  BST["Boost 5V → 12V<br/>≥5A, tụ 470–1000µF đầu ra"]
  ESP["ESP32 DevKit"]
  A1["AOD4184 SB1"]
  A2["AOD4184 SB2"]
  M1["ZWF-30 bánh trên<br/>tụ 1000µF + SS34"]
  M2["ZWF-30 bánh dưới<br/>tụ 1000µF + SS34"]
  S1["MG996R nâng bóng<br/>tụ 1000µF"]
  S2["MG996R cổ nòng<br/>tụ 1000µF"]
  KEYS["4 nút nhấn<br/>POWER · MENU · − · +<br/>chân kia xuống GND"]
  BTN["Nút BOOT"]
  LED["LED onboard"]
  BZ["Còi thụ động MH-FMD<br/>kích mức thấp, VCC 3.3V"]
  LCD["LCD 16x2 I2C 0x27<br/>VCC 5V, tùy chọn"]
  GND["GND chung: nguồn, boost, ESP32, VIN− AOD4184,<br/>dây nâu servo, nút nhấn, GND còi, GND LCD"]

  PSU -->|"5V"| BST
  PSU -->|"5V → VIN"| ESP
  PSU -->|"5V → dây đỏ"| S1
  PSU -->|"5V → dây đỏ"| S2
  PSU -->|"5V → VCC"| LCD
  BST -->|"12V → VIN+"| A1
  BST -->|"12V → VIN+"| A2
  ESP -->|"GPIO18 → PWM"| A1
  ESP -->|"GPIO19 → PWM"| A2
  A1 -.->|"chân PWM → GPIO16<br/>(đọc lại)"| ESP
  A2 -.->|"chân PWM → GPIO17<br/>(đọc lại)"| ESP
  A1 -->|"OUT+ → đỏ+xanh<br/>OUT− → đen+vàng"| M1
  A2 -->|"OUT+ → đỏ+xanh<br/>OUT− → đen+vàng"| M2
  ESP -->|"GPIO25 → dây cam"| S1
  ESP -->|"GPIO26 → dây cam"| S2
  KEYS -->|"GPIO32 POWER<br/>GPIO33 MENU<br/>GPIO13 −<br/>GPIO14 +"| ESP
  BTN -->|"GPIO0"| ESP
  ESP -->|"GPIO2"| LED
  ESP -->|"GPIO27 → I/O<br/>3V3 → VCC"| BZ
  ESP -->|"GPIO21 → SDA<br/>GPIO22 → SCL"| LCD
```

## 2. Khởi động và WiFi

Dòng bắt đầu bằng "LCD:" là nội dung màn hình (tiếng Anh), 🔔 là tiếng còi.

```mermaid
flowchart TD
  A([Cấp nguồn]) --> G["Kiểm tra OTA thử nghiệm<br/>quá 3 lần khởi động thì quay về bản cũ"]
  G --> B["Tạo mã thiết bị từ MAC<br/>bánh đà tắt, servo về vị trí an toàn<br/>dò LCD ở 0x27"]
  B --> SPL["LCD: PingPong Launch / v1.0.0 A1B2C3<br/>🔔 2 nốt lên"]
  SPL --> C{"Có WiFi đã lưu?"}
  C -- Không --> P["Bật AP PingPong-XXXXXX, trang 192.168.4.1<br/>LCD: Setup: join WiFi ↔ Pass: pingpong<br/>🔔 2 bíp"]
  C -- Có --> D["STA: kết nối mạng đã lưu<br/>LCD: Connecting WiFi"]
  D --> E{"Kết nối được trong 15s?"}
  E -- Có --> ON["Online, chế độ STA<br/>🔔 3 nốt lên"]
  E -- Không --> P
  P --> Q["Quét mạng, chọn mạng, nhập mật khẩu"]
  Q --> R["AP+STA: thử kết nối, DHCP cấp IP<br/>LCD: Joining WiFi..."]
  R --> S{"Thành công?"}
  S -- Không --> F["LCD: WiFi failed / Wrong password<br/>🔔 1 tiếng dài"]
  F --> P
  S -- Có --> T["Lưu NVS, web hiện IP và mã thiết bị<br/>LCD: WiFi connected / IP<br/>🔔 3 nốt lên"]
  T --> U["Chờ 8s, tắt AP, chuyển STA"]
  U --> ON
  ON --> M["Kết nối MQTT broker.emqx.io:1883<br/>LCD: Cloud connecting"]
  M --> N["status = online, subscribe cmd, publish state<br/>xác nhận firmware chạy tốt<br/>🔔 bíp cao"]
  N --> L["Vòng lặp điều khiển"]
  BTN(["Giữ BOOT"]) -.-> H["LCD: Hold to reset / WiFi in 2s...<br/>🔔 tích mỗi giây"]
  H -.->|"đủ 3s"| X["LCD: WiFi reset / Rebooting...<br/>🔔 bíp dài, xóa WiFi"]
  X -.-> A
```

## 3. Vòng lặp bắn

```mermaid
flowchart TD
  L["Vòng lặp"] --> PWR{"power?"}
  PWR -- Tắt --> OFF["Bánh đà giảm dần về 0<br/>cổ nòng về giữa, piston dừng<br/>LCD: OFF ID A1B2C3"]
  OFF --> L
  PWR -- Bật --> RAMP["Bánh đà tăng dần tới SB1 / SB2<br/>LCD: Spinning up..."]
  RAMP --> RDY{"Đủ tốc và ổn định 1.5s?"}
  RDY -- Chưa --> AIM
  RDY -- Rồi --> FIRE["LCD: FIRING 42<br/>🔔 2 bíp cao (lần đầu)"]
  FIRE --> FEED{"Đến nhịp lên bóng?"}
  FEED -- Chưa --> AIM
  FEED -- Rồi --> SHOT["Piston nâng ~0.7s, hạ ~0.85s (hành trình 2°→178°)<br/>quả trên cùng vào bánh đà"]
  SHOT --> CNT["shots + 1, publish state"]
  CNT --> RND{"mode = random?"}
  RND -- Có --> NEWT["Chọn góc ngẫu nhiên trong ±range"]
  RND -- Không --> AIM
  NEWT --> AIM["Cổ nòng: quét / ngẫu nhiên / cố định"]
  AIM --> L
```

## 4. Đồng bộ MQTT hai chiều

```mermaid
sequenceDiagram
  participant W as Web (SyncEngine)
  participant B as Broker EMQX
  participant E as ESP32
  E->>B: state {rev: 7, boot, sb1: 80} (retain)
  B-->>W: state rev 7
  Note over W: Kéo SB1 → 60: hiện 60 ngay (đang chờ)
  W->>B: cmd {"id":"a1","sb1":60} QoS 1
  B-->>W: state cũ rev 7 tới muộn
  Note over W: Vẫn hiện 60, không giật về 80
  B->>E: cmd a1
  E->>E: Áp dụng, rev 8
  E->>B: ack {id: a1, rev: 8}
  E->>B: state {rev: 8, sb1: 60}
  B-->>W: ack a1 (rev 8) + state rev 8
  Note over W: ack khớp id và state.rev ≥ 8 → đã đồng bộ
  W->>B: cmd {"id":"b2","feed":50}
  Note over W: 2.5s không có ack → gửi lại cùng id b2
  W->>B: cmd {"id":"b2","feed":50}
  B->>E: cmd b2 (lần 1 và lần 2)
  Note over E: Lần 2 trùng id → không áp dụng lại, chỉ gửi ack
  E->>B: ack {id: b2, result: duplicate}
```

## 5. Cập nhật firmware qua OTA

```mermaid
sequenceDiagram
  participant D as Dev
  participant GH as GitHub Actions + Releases
  participant W as Web React
  participant B as Broker EMQX
  participant E as ESP32
  D->>GH: git push tag v1.1.0
  GH->>GH: Build .bin, tạo Release (digest SHA-256)
  W->>GH: GET releases/latest
  GH-->>W: url .bin + sha256 + version
  Note over W: Máy phải đang tắt nguồn bắn
  W->>B: cmd {"ota":{url, sha256, version}}
  B->>E: cmd ota
  E->>E: Kiểm tra link thuộc repo, SHA-256 hợp lệ
  E->>B: ota {state: downloading}
  Note over E: LCD: Updating fw... + thanh %<br/>🔔 2 bíp
  E->>GH: HTTPS tải .bin, ghi vào vùng OTA trống
  E->>B: ota {downloading, 5%...100%}
  B-->>W: Thanh tiến trình
  alt SHA-256 khớp
    E->>B: ota {state: rebooting}
    Note over E: LCD: Update done<br/>🔔 4 nốt lên
    E->>E: Khởi động bản mới (chế độ thử)
    E->>B: state {fw: 1.1.0} → xác nhận bản mới
  else Lỗi
    E->>B: ota {state: error, error: sha_mismatch}
    Note over E: LCD: Update failed<br/>🔔 3 bíp dài, giữ bản cũ
  end
```

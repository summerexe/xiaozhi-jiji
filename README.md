# XiaoZhi AI Chatbot – Custom Build with Servo & Face Animation

This is a customized fork of the XiaoZhi ESP32 project with **servo motors** and **JijiFace animated expressions** wired to AI emotions. These additions work **only** with the **compact_wifi_board_s3cam** board (`bread-compact-wifi-s3cam`).

## Original Project & Documentation

- **Original XiaoZhi ESP32 repo:** [78/xiaozhi-esp32](https://github.com/78/xiaozhi-esp32)

---

## What’s Different in This Fork

| Feature                  | Status                                                                                                             |
| ------------------------ | ------------------------------------------------------------------------------------------------------------------ |
| **JijiFace animation**   | Animated robot face that reacts to emotions (happy, sad, angry, etc.)                                              |
| **Servo motors**         | Two servos driven by emotions via `SERVO1_GPIO` and `SERVO2_GPIO`                                                  |
| **Touch button → happy** | Touch → happy face + servos; after release, smile holds for `TOUCH_SMILE_HOLD_MS` then smoothly returns to neutral |

These features are **implemented only for the Bread Compact WiFi + LCD + Camera** board. Other boards in the repo do not define the servo controller, so they will not have emotion-driven servos or the full JijiFace integration.

---

## Hardware (This Fork)

Full BOM in my Patreon page: https://www.patreon.com/posts/jiji-robot-bill-147615515

- **Board:** ESP32S3CAM (Bread Compact WiFi + LCD + Camera)
- **Camera:** OV2640
- **Display:** SPI LCD (GC9A01 240×240) – match your hardware in menuconfig
- **Servos:** 2 servos on GPIO 1 and GPIO 2 (configurable in `main/boards/bread-compact-wifi-s3cam/config.h`)
- **Audio:** INMP441 (I2S mic) + MAX98357 (I2S speaker), Duplex I2S
- **Touch Sensor:** TTP223
- **Power:** Powered by 5V output power bank

---

## Build Configuration

Use menuconfig to match the reference settings so the firmware works correctly.

### 1. ESP-IDF commands

Use these commands during development:

1. Activate the ESP-IDF environment:

```bash
source activate_esp_idf.sh
```

2. Open the configuration menu:

```bash
idf.py menuconfig
```

3. Build the firmware:

```bash
idf.py build
```

4. Flash the device:

```bash
idf.py flash
```

5. After code changes, rebuild and flash in one step:

```bash
idf.py build flash
```

6. Monitor serial output:

```bash
idf.py monitor
```

To list available serial ports on macOS:

```bash
ls /dev/cu.*
```

### 2. Set target and open menuconfig

```bash
source activate_esp_idf.sh
idf.py menuconfig
```

### 3. Board and display

1. Go to **Xiaozhi Assistant → Board Type**
2. Select **Bread Compact WiFi + LCD + Camera (面包板)**
3. Go to **Xiaozhi Assistant → LCD Type**
4. Choose the LCD that matches your display, e.g.:
   - **GC9A01 240\*240 Circle** (round display)
   - Or the correct option for your panel

### 4. Camera

1. Go to **Component config → Espressif Camera Sensors Configurations → Camera Sensor Configuration**
2. Set **Select and Set Camera Sensor** to **OV2640**
3. Enter the OV2640 submenu:
   - Enable **Auto detect**
   - Set **default output format** to **YUV422**
   - Pick an appropriate resolution

### 5. Custom assets (wake words, fonts, theme)

Use the **Custom Assets Generator** to configure board type, screen size, wake words, fonts, emojis, and chat background:

👉 **[xiaozhi-assets-generator](https://github.com/78/xiaozhi-assets-generator)** — clone and run it locally, or use the hosted version if available (check the repo).

Configure your board type, screen size, and custom wake words there. **We recommend setting up your own wake words** for best results.

#### Reference configuration (what this fork uses)

| Setting          | Value     |
| ---------------- | --------- |
| Board            | ESP32-S3  |
| Screen size      | 240×240   |
| Fonts            | None      |
| Emoji pack       | None      |
| Background color | `#000000` |
| Theme mode       | Dark      |

#### Using the pre-built assets.bin (optional)

A pre-built `assets.bin` with the above settings is included in `main/assets.bin` for quick start. **For best results, we recommend creating your own assets with custom wake words** using the generator above.

To use the pre-built (or your own) `assets.bin`:

1. In **menuconfig** (`idf.py menuconfig`), go to **Xiaozhi Assistant → Flash Assets**
2. Select **Flash Custom Assets**
3. Set **Custom Assets File** to `assets.bin` (the default; the file lives in `main/`)
4. Save and build

If you put your own `assets.bin` elsewhere, set **Custom Assets File** to the path relative to the project (e.g. `main/assets.bin`).

---

### 6. Save and build

1. Save (S) and exit (Q)
2. Build and flash:

```bash
idf.py build flash
```

---

## Servo & Face Behavior

- **Servo GPIOs:** defined in `main/boards/bread-compact-wifi-s3cam/config.h` as `SERVO1_GPIO` and `SERVO2_GPIO` (default GPIO 1 and 2).
- Set either to `GPIO_NUM_NC` in `config.h` to disable that servo.
- **Touch button (GPIO 14):** press → happy emotion (face + servos). On release, the smile and servo pose **stay for a short hold time**, then the face and servos **smoothly return to neutral** (interpolated servo movement).

### `TOUCH_SMILE_HOLD_MS` (smile duration after touch release)

In `main/boards/bread-compact-wifi-s3cam/compact_wifi_board_s3cam.cc`, the macro **`TOUCH_SMILE_HOLD_MS`** sets how many **milliseconds** the device keeps showing the happy expression and servo “smile” pose **after** you lift your finger from the touch pad, before returning to neutral (servos use smooth interpolation).

| Value                    | Effect                                     |
| ------------------------ | ------------------------------------------ |
| **Lower** (e.g. `600`)   | Returns to neutral sooner after release.   |
| **Higher** (e.g. `2000`) | Keeps the happy look longer after release. |
| **Default**              | `1400` ms                                  |

Change the number, rebuild, and flash. Touching again before the hold ends cancels the pending return and keeps the happy state (timer restarts on the next release).

---

## Project Structure

- `main/boards/bread-compact-wifi-s3cam/` – board-specific code (servo init, camera, display)
- `main/boards/common/servo_controller.h` – shared servo logic
- `main/display/lvgl_display/jiji_face.cc` – JijiFace implementation
- `main/display/lcd_display.cc` – display + emotion → servo wiring

# xiaozhi-jiji
